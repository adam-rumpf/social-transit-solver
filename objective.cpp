#include "objective.hpp"

/**
Objective object constructor that loads objective file input and sets a network object pointer.

Requires the name of the objective data input file and a pointer to the network object.
*/
Objective::Objective(Network * net_in)
{
	Net = net_in;
	pop_size = Net->population_nodes.size();
	fac_size = Net->facility_nodes.size();

	// Read objective data
	ifstream obj_file;
	obj_file.open(OBJECTIVE_FILE);
	if (obj_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(obj_file, line); // skip comment line
		int count = 0;

		while (obj_file.eof() == false)
		{
			count++;

			// Get whole line as a string stream
			getline(obj_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);

			// Go through each piece of the line
			getline(stream, piece, '\t'); // Label
			getline(stream, piece, '\t'); // Value
			string value = piece;

			// Expected data
			if (count == 2)
				lowest_metrics = stoi(value);
			if (count == 3)
				gravity_exponent = stod(value);
			if (count == 4)
				multiplier = stod(value);
		}

		obj_file.close();
	}
	else
	{
		cout << "Objective file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}
}

/**
Calculates objective value.

Requires a solution vector, which is passed directly to the all_metrics() method for use in calculating all population center gravity metrics. The objective value is the sum of the lowest few of these metrics (specifically, the number stored in "lowest_metrics"). Because the TS/SA algorithm is written to minimize its objective, we actually return the negative of this value.
*/
double Objective::calculate(const vector<int> &fleet)
{
	vector<double> metrics = all_metrics(fleet); // calculate all metrics
	sort(metrics.begin(), metrics.end()); // sort metrics in ascending order

	double sum = 0; // sum lowest metrics
	for (int i = 0; i < lowest_metrics; i++)
		sum += metrics[i];

	return -sum; // return negative sum
}

/**
Calculates gravity metrics for all population centers.

Requires a solution vector, which is then used to calculate the gravity metrics for each population center.

Returns a vector of gravity metrics for each population center, ordered in the same way as the population center list.
*/
vector<double> Objective::all_metrics(const vector<int> &fleet)
{
	// Generate a vector of line headways based on the fleet sizes
	vector<double> headways(Net->lines.size());
	for (int i = 0; i < headways.size(); i++)
		headways[i] = Net->lines[i]->headway(fleet[i]);

	// Initialize a population center-to-facility distance matrix
	vector<vector<double>> distance(pop_size);
	for (int i = 0; i < pop_size; i++)
		distance[i].resize(fac_size);

	// Calculate distances row-by-row using single-source Dijkstra in parallel over all sources
	parallel_for(0, pop_size, [&](int i)
	{
		population_to_all_facilities(i, distance[i]);
	});

	// Calculate facility metrics
	vector<double> fac_met(fac_size);
	for (int i = 0; i < fac_size; i++)
		fac_met[i] = facility_metric(i, distance);

	// Do same for all population centers to get the gravity metrics
	vector<double> pop_met(pop_size);
	for (int i = 0; i < pop_size; i++)
		pop_met[i] = population_metric(i, distance, fac_met);

	return pop_met;
}

/**
Calculates the distance from a given population center to all primary care facilities.

Requires the index of a population center (as a position in the population center list) and a reference to a distance matrix row.

Returns nothing, but updates the referenced row with all distances.

Distance calculations are accomplished with a priority queue implementation of single-sink Dijkstra. Note that this method will be run in parallel for all population centers, and so must rely on mostly local variables, treating all other data as read-only.
*/
void Objective::population_to_all_facilities(int source, vector<double> &row)
{
	/*
	To explain some of the technical details, the standard C++ priority queue container does not easily allow changing the priorities of its entries. This makes the tentative distance reduction step of Dijkstra's algorithm more difficult since we cannot simply reduce priorities (distances) in the queue.
	
	As a workaround, whenever we need to reduce a tentative distance, we just add another copy of that node to the queue along with its new (smaller) distance value. Whenever we do this we also reduce that node's current distance in a distance vector.
	
	If we pop a node out of the queue and its distance matches the distance from the vector, then we must be looking at the most recent copy, which will also be the correct one. If they do not match, then we must be looking at an old copy, and we can ignore it.
	*/

	// Initialize Dijkstra data structures
	vector<double> dist(Net->nodes.size(), INFINITY); // tentative distance to every node (all initially infinite)
	dist[Net->population_nodes[source]->id] = 0.0; // distance from source to self is 0
	unordered_set<int> unsearched_sinks; // set of facility node IDs, to be removed as they are searched as a stopping criterion
	for (int i = 0; i < fac_size; i++)
		unsearched_sinks.insert(Net->facility_nodes[i]->id);
	priority_queue<dist_pair, vector<dist_pair>, greater<dist_pair>> dist_queue; // min-priority queue to hold distance/ID pairs sorted by distance
	dist_queue.push(make_pair(0.0, Net->population_nodes[source]->id)); // initialize queue with only the source node and its distance of zero

	// Main Dijkstra loop
	while (unsearched_sinks.empty() == false)
	{
		// Get current minimum distance
		double chosen_dist = dist_queue.top().first; // lowest distance
		int chosen_node = dist_queue.top().second; // lowest-distance node ID
		dist_queue.pop(); // remove entry from queue

		// Only proceed if we can verify that this is the most recent copy of the node in the priority queue
		if (dist[chosen_node] < chosen_dist)
			continue;

		// Try to remove node from unprocessed sink list (if it is not in the list, nothing will happen)
		unsearched_sinks.erase(chosen_node);

		// Search core out-neighborhood for distance reductions
		for (int i = 0; i < Net->nodes[chosen_node]->core_out.size(); i++)
		{
			int head = Net->nodes[chosen_node]->core_out[i]->head->id; // current out-neighbor
			double new_dist = dist[chosen_node] + Net->nodes[chosen_node]->core_out[i]->cost; // own distance plus outgoing arc's cost
			if (new_dist < dist[head])
			{
				// If the new distance is an improvement, update the out-neighbor's distance and add a new copy to the queue
				dist[head] = new_dist;
				dist_queue.push(make_pair(new_dist, head));
			}
		}

		// Repeat search for access out-neighborhood
		for (int i = 0; i < Net->nodes[chosen_node]->access_out.size(); i++)
		{
			int head = Net->nodes[chosen_node]->access_out[i]->head->id; // current out-neighbor
			double new_dist = dist[chosen_node] + Net->nodes[chosen_node]->access_out[i]->cost; // own distance plus outgoing arc's cost
			if (new_dist < dist[head])
			{
				// If the new distance is an improvement, update the out-neighbor's distance and add a new copy to the queue
				dist[head] = new_dist;
				dist_queue.push(make_pair(new_dist, head));
			}
		}
	}

	// Use distances to update given vector object with source-to-facility distances
	for (int i = 0; i < Net->facility_nodes.size(); i++)
		row[i] = dist[Net->facility_nodes[i]->id];
}

/**
Calculates the gravity metric for a given facility.

Requires the index of a facility (as a position in the facility list) and a reference to the entire distance matrix.

The facility gravity metric for a facility j is defined by
	V_j = sum_k P_k d_kj^(-beta)
where the sum is over all population centers k, P_k is the population at center k, d_kj is the distance from center k to facility j, and beta is the gravity model exponent.
*/
double Objective::facility_metric(int fac, vector<vector<double>> &distance)
{
	double sum = 0.0; // running total

	// Calculate each term of the sum
	for (int i = 0; i < pop_size; i++)
		sum += Net->population_nodes[i]->value * pow(distance[i][fac], -gravity_exponent);

	return sum;
}

/**
Calculates the gravity metric for a given population center.

Requires the index of a population center (as a position in the population center list), a reference to the entire distance matrix, and a reference to the facility metric vector.

The population gravity metric for a population center i is defined by
	A_i = sum_j (S_j d_ij^(-beta))/V_j
where the sum is over all facilities j, S_j is the capacity (or quality) of facility j, and d_ij, beta, and V_j all mean the same thing as for the facility metric.
*/
double Objective::population_metric(int pop, vector<vector<double>> &distance, vector<double> &fac_metric)
{
	double sum = 0.0; // running total

	// Calculate each term of the sum
	for (int i = 0; i < fac_size; i++)
		sum += (Net->facility_nodes[i]->value * pow(distance[pop][i], -gravity_exponent)) / fac_metric[i];

	return multiplier * sum; // apply multiplication factor to result
}
