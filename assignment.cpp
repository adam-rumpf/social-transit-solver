#include "assignment.hpp"

/// Constant-cost assignment constructor sets network pointer.
ConstantAssignment::ConstantAssignment(Network * net_in)
{
	Net = net_in;
	stop_size = Net->stop_nodes.size();
}

/**
Constant-cost assignment model evaluation for a given solution.

Requires a fleet size vector and nonlinear cost vector.

Returns a pair containing a vector of flow values and a waiting time scalar.

This model comes from the linear program formulation of the common line problem, which can be solved using a Dijkstra-like label setting algorithm. This must be done separately for every sink node, but each of these problems is independent and may be parallelized. The final result is the sum of these individual results.
*/
pair<vector<double>, double> ConstantAssignment::calculate(const vector<int> &fleet, const vector<double> &arc_costs)
{
	// Generate a vector of line frequencies based on the fleet sizes
	vector<double> line_freq(Net->lines.size());
	for (int i = 0; i < line_freq.size(); i++)
		line_freq[i] = Net->lines[i]->frequency(fleet[i]);

	// Use the line frequencies to generate arc frequencies
	vector<double> freq(Net->core_arcs.size(), INFINITY);
	for (int i = 0; i < Net->lines.size(); i++)
		for (int j = 0; j < Net->lines[i]->boarding.size(); j++)
			freq[Net->lines[i]->boarding[j]->id] = line_freq[i];

	// Initialize reader/writer locks for incrementing the flow and waiting variables for each hyperpath in parallel
	reader_writer_lock flow_lock; // reader/writer lock for arc flow variables
	reader_writer_lock wait_lock; // reader/writer lock for total waiting time variable

	// Solve single-destination model in parallel for all sinks and add all results
	vector<double> flows(Net->core_arcs.size(), 0.0); // total flow vector over all destinations
	double waiting = 0.0; // total waiting time over all destinations
	cout << "Solving single-sink models in parallel:\n|";
	for (int i = 0; i < stop_size; i++)
		cout << '-'; // length of "progress bar"
	cout << "|\n|";
	parallel_for_each(Net->stop_nodes.begin(), Net->stop_nodes.end(), [&](Node * s)
	{
		cout << '*'; // shows progress
		flows_to_destination(s->id, flows, waiting, freq, arc_costs, &flow_lock, &wait_lock);
	});
	cout << '|' << endl;

	return make_pair(flows, waiting);
}

/**
Calculates the flow vector to a given sink.

Requires the sink index (as a position in the stop node list), flow vector, waiting time scalar, line frequency vector, arc cost vector, and pointers to the arc flow reader/writer lock and waiting time reader/writer lock, respectively.

The flow vector and waiting time are passed by reference and automatically incremented according to the results of this function.

The algorithm here solves the constant-cost, single-destination version of the common lines problem, which is a LP similar to min-cost flow and is solvable with a Dijkstra-like label setting algorithm. This process can be parallelized over all destinations, and so should rely only on local variables.
*/
void ConstantAssignment::flows_to_destination(int dest, vector<double> &flows, double &waiting, const vector<double> &freq, const vector<double> &arc_costs, reader_writer_lock *flow_lock, reader_writer_lock *wait_lock)
{
	/*
	To explain a few technical details, the label setting algorithm involves updating a distance label for each node. In each iteration, we choose the unprocessed arc with the minimum value of its own cost plus its head's label. In order to speed up that search, we store all of those values in a min-priority queue. As with Dijkstra's algorithm, to get around the inability to update priorities, we just add extra copies to the queue whenever they are updated. We also store a master list of those values, which should always decrease as the algorithm moves forward, as a comparison every time we pop something out of the queue to ensure that we have the latest version.

	The arc loading algorithm involves processing all of the selected attractive arcs in descending order of their cost-plus-head-label from the label setting algorithm. This is accomplished in a similar way, with a copy of the cost-plus-head-label being added to a max-priority queue each time the tail label is updated.
	*/

	// Initialize variables
	double chosen_label; // cost-plus-head-label value chosen for current loop iteration
	int chosen_arc; // arc ID chosen for current loop iteration
	int chosen_tail; // tail node ID chosen for current loop iteration
	int chosen_head; // head node ID chosen for current loop iteration
	int updated_arc; // arc ID for label setting updates
	double updated_label; // updated label value
	double added_flow; // chosen arc's added flow volume

	// Initialize containers
	vector<double> node_label(Net->core_nodes.size(), INFINITY); // tentative distances from every node to the destination
	node_label[Net->stop_nodes[dest]->id] = 0.0; // distance from destination to self is 0
	vector<double> node_freq(Net->core_nodes.size(), 0.0); // total frequency of all attractive arcs leaving a node
	vector<double> node_vol(Net->core_nodes.size(), 0.0); // total flow leaving a node
	for (int i = 0; i < Net->stop_nodes.size(); i++)
		// Initialize travel volumes for stop nodes based on demand for destination
		node_vol[Net->stop_nodes[i]->id] = Net->stop_nodes[dest]->incoming_demand[i];
	vector<double> node_wait(Net->core_nodes.size(), 0.0); // expected waiting time at each node
	unordered_set<int> unprocessed_arcs; // arcs not yet chosen in the main label setting loop, in order to ensure that each arc is processed only once
	for (int i = 0; i < Net->core_arcs.size(); i++)
		// All arcs are initially unprocessed
		unprocessed_arcs.insert(Net->core_arcs[i]->id);
	priority_queue<arc_cost_pair, vector<arc_cost_pair>, greater<arc_cost_pair>> arc_queue; // min-priority queue to quickly access the unprocessed arc with the minimum cost-plus-head-distance value
	for (int i = 0; i < Net->stop_nodes[dest]->core_in.size(); i++)
		// Set all non-infinite arc labels (which will include only the sink node's incoming arcs)
		arc_queue.push(make_pair(arc_costs[Net->stop_nodes[dest]->core_in[i]->id], Net->stop_nodes[dest]->core_in[i]->id));
	unordered_set<int> attractive_arcs; // set of attractive arcs
	priority_queue<arc_cost_pair, vector<arc_cost_pair>, less<arc_cost_pair>> load_queue; // max-priority queue to process attractive arcs in reverse order
	stack<arc_cost_pair> nonzero_flows; // stack of flow increase/arc ID pairs for quickly processing only the nonzero updates

	// Main label setting loop

	while ((unprocessed_arcs.empty() == false) && (arc_queue.empty() == false))
	{
		// Find the arc that minimizes the sum of its head's label and its own cost
		chosen_label = arc_queue.top().first;
		chosen_arc = arc_queue.top().second;
		arc_queue.pop();

		// Only proceed for unprocessed arcs
		if (unprocessed_arcs.count(chosen_arc) == 0)
			continue;

		// Mark arc as processed and get its tail
		unprocessed_arcs.erase(chosen_arc);
		chosen_tail = Net->core_arcs[chosen_arc]->tail->id;

		// Skip arcs with zero frequency (can occur for boarding arcs on lines with no vehicles)
		if (freq[chosen_arc] == 0)
			continue;

		// Update the node label of the chosen arc's tail
		if (node_label[chosen_tail] >= chosen_label)
		{
			// Check whether the attractive arc has infinite frequency
			if (freq[chosen_arc] < INFINITY)
			{
				// Finite-frequency attractive arc (should include only boarding arcs)

				// Update tail label
				if (node_label[chosen_tail] < INFINITY)
					// Standard update
					node_label[chosen_tail] = (node_freq[chosen_tail] * node_label[chosen_tail] + freq[chosen_arc] * chosen_label) / (node_freq[chosen_tail] + freq[chosen_arc]);
				else
					// First-time update (from initially-infinite label)
					node_label[chosen_tail] = (1 / freq[chosen_arc]) + chosen_label;

				// Update tail frequency
				node_freq[chosen_tail] += freq[chosen_arc];
			}
			else
			{
				// Infinite-frequency attractive arc

				// Update tail label and frequency
				node_label[chosen_tail] = chosen_label;
				node_freq[chosen_tail] = INFINITY;

				// Remove all other attractive arcs leaving the tail
				for (int i = 0; i < Net->core_nodes[chosen_tail]->core_out.size(); i++)
					attractive_arcs.erase(Net->core_nodes[chosen_tail]->core_out[i]->id);
			}

			// Add arc to attractive arc set
			attractive_arcs.insert(chosen_arc);

			// Update arc labels that are affected by the updated tail node
			for (int i = 0; i < Net->core_nodes[chosen_tail]->core_in.size(); i++)
			{
				// Find arcs to update, recalculate labels, and push updates into priority queue
				updated_arc = Net->core_nodes[chosen_tail]->core_in[i]->id;
				updated_label = arc_costs[Net->core_arcs[updated_arc]->id] + node_label[chosen_tail];
				arc_queue.push(make_pair(updated_label, updated_arc));
			}
		}
	}

	// Build updated max-priority queue for attractive arc set

	for (auto a = attractive_arcs.begin(); a != attractive_arcs.end(); a++)
	{
		// Recalculate the cost-plus-head label for each attractive arc and place in a max-priority queue
		load_queue.push(make_pair(node_label[Net->core_arcs[*a]->head->id] + arc_costs[Net->core_arcs[*a]->id], *a));
	}

	vector<double>().swap(node_label); // clear node label vector, which is no longer needed

	// Main arc loading loop

	while (load_queue.empty() == false)
	{
		// Process attractive arcs in descending order of cost-plus-head-label value

		// Get next arc's properties and remove from queue
		chosen_arc = load_queue.top().second;
		load_queue.pop();
		chosen_tail = Net->core_arcs[chosen_arc]->tail->id;
		chosen_head = Net->core_arcs[chosen_arc]->head->id;

		// Distribute volume from tail
		if (freq[chosen_arc] < INFINITY)
		{
			// Finite-frequency arc
			added_flow = (freq[chosen_arc] / node_freq[chosen_tail]) * node_vol[chosen_tail]; // distribute flow proportionally according to frequency
			node_wait[chosen_tail] = max(node_wait[chosen_tail], added_flow / freq[chosen_arc]); // update waiting time to be bounded below by all outgoing flow:frequency ratios
		}
		else
			// Infinite-frequency arc
			added_flow = node_vol[chosen_tail]; // all flow goes to single outgoing arc

		// If this results in a nonzero flow increase, update the head and add the change to an update stack
		if (added_flow > 0)
		{
			node_vol[chosen_head] += added_flow;
			nonzero_flows.push(make_pair(added_flow, chosen_arc));
		}
	}

	// Sum all waiting times
	double total_wait = 0.0;
	for (int i = 0; i < node_wait.size(); i++)
		total_wait += node_wait[i];

	// Process nonzero flow queue while reader/writer lock is engaged
	flow_lock->lock();
	while (nonzero_flows.empty() == false)
	{
		chosen_arc = nonzero_flows.top().second;
		added_flow = nonzero_flows.top().first;
		flows[chosen_arc] += added_flow; // apply nonzero flow increase from stack
		nonzero_flows.pop();
	}
	flow_lock->unlock();

	// Increment total waiting time while reader/writer lock is engaged
	wait_lock->lock();
	waiting += total_wait;
	wait_lock->unlock();
}

/// Nonlinear assignment constructor reads in model data from file and sets network pointer.
NonlinearAssignment::NonlinearAssignment(string input_file, Network * net_in)
{
	Net = net_in;

	// Initialize submodel object
	Submodel = new ConstantAssignment(net_in);

	// Read assignment model data
	cout << "Reading assignment model data..." << endl;
	ifstream a_file;
	a_file.open(input_file);
	if (a_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(a_file, line); // skip comment line
		int count = 0;

		while (a_file.eof() == false)
		{
			count++;

			// Get whole line as a string stream
			getline(a_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);

			// Go through each piece of the line
			getline(stream, piece, '\t'); // Label
			getline(stream, piece, '\t'); // Value
			string value = piece;

			// Expected data
			if (count == 1)
				error_tol = stod(value);
			if (count == 2)
				change_tol = stod(value);
			if (count == 3)
				max_iterations = stoi(value);
			if (count == 5)
				conical_alpha = stod(value);
			if (count == 6)
				conical_beta = stod(value);
		}

		a_file.close();
	}
	else
		cout << "Assignment file file failed to open." << endl;

	cout << "Assignment data read." << endl;
}

/**
Nonlinear cost assignment model evaluation for a given solution.

Requires a fleet size vector and an initial solution, which takes the form of a pair made up of a flow vector and a waiting time scalar.

Returns a pair containing a vector of flow values and a waiting time scalar.

The solution vector is used to determine the frequency and capacity of each line. Frequencies contribute to boarding arc costs for the common lines problem, while capacities contribute to an overcrowding penalty to model congestion.

The overall process being used here is the Frank-Wolfe algorithm, which iteratively solves the linear approximation of the nonlinear cost quadratic program. That linear approximation happens to be an instance of the constant-cost LP whose costs are based on the current solution.
*/
pair<vector<double>, double> NonlinearAssignment::calculate(vector<int> &fleet, pair<vector<double>, double> initial_sol)
{
	// Initialize variables
	pair<vector<double>, double> sol_next = initial_sol; // flow/waiting pair calculated as the linearized submodel solution
	pair<vector<double>, double> sol_previous = initial_sol; // flow/waiting pair for the previous solution
	vector<double> arc_costs(Net->core_arcs.size()); // arc costs based on current flow
	int iteration = 0; // current iteration number
	double error = INFINITY; // current solution error bound
	double change = INFINITY; // maximum elementwise difference between consecutive solutions

	// Calculate line arc capacities
	vector<double> capacities(Net->core_arcs.size(), INFINITY);
	for_each(Net->line_arcs.begin(), Net->line_arcs.end(), [&](Arc * a)
	{
		capacities[a->id] = Net->lines[a->line]->capacity(fleet[a->line]);
	});

	// Main Frank-Wolfe loop

	cout << "\n========================================\n\n";
	while ((iteration < max_iterations) && (error > error_tol) && (change > change_tol))
	{
		// Loop continues until achieving sufficiently low error or reaching an iteration cutoff
		iteration++;

		cout << "----------------------------------------" << endl;
		cout << "Frank-Wolfe algorithm iteration " << iteration << endl << endl;

		// Update all arc costs based on the current flow
		for_each(Net->core_arcs.begin(), Net->core_arcs.end(), [&](Arc * a)
		{
			arc_costs[a->id] = arc_cost(a->id, sol_previous.first[a->id], capacities[a->id]);
		});

		// Solve constant-cost model for given cost vector
		sol_next = Submodel->calculate(fleet, arc_costs);

		// Calculate new error bound
		error = obj_error(capacities, sol_previous.first, sol_previous.second, sol_next.first, sol_next.second);
		cout << "Current error bound = " << error << endl;

		cout << "lambda = " << 1 - (1.0 / iteration) << endl;

		// Update solution as successive average of consecutive solutions and get maximum elementwise difference
		change = solution_update(1 - (1.0 / iteration), sol_previous.first, sol_previous.second, sol_next.first, sol_next.second);
		cout << "Maximum change = " << change << endl;
	}

	cout << "\n========================================\n";
	cout << "Frank-Wolfe ended." << endl;
	if (error <= error_tol)
		cout << "Error bound achieved at " << error << endl;
	if (change <= change_tol)
		cout << "Change bound achieved at " << change << endl;
	if ((error > error_tol) && (change > change_tol))
		cout << "Iteration cutoff reached at " << iteration << " with error " << error << endl;

	return sol_previous;
}

/**
Calculates the nonlinear cost function for a given arc.

Requires the arc ID, arc flow, and arc capacity.

Returns the arc's cost according to the conical congestion function.
*/
double NonlinearAssignment::arc_cost(int id, double flow, double capacity)
{
	// Return infinite cost for zero-capacity arcs
	if (capacity == 0)
		return INFINITY;

	// Return only the arc's base cost for infinite-capacity or zero-flow arcs
	if ((capacity >= INFINITY) || (flow == 0))
		return Net->core_arcs[id]->cost;

	/*
	Otherwise, evaluate the conical congestion function, which is defined as:
		c(x) = c * (2 + sqrt((alpha * (1 - x/u))^2 + beta^2) - alpha * (1 - x/u) - beta)
	where c(x) is the nonlinear cost, x is the arc's flow, c is the arc's base cost, u is the arc's capacity, and alpha and beta are parameters.
	*/
	double ratio = 1 - (flow / capacity);
	return Net->core_arcs[id]->cost * (2 + sqrt(pow(conical_alpha*ratio, 2) + pow(conical_beta, 2)) - (conical_alpha * ratio) - conical_beta);
}

/**
Calculates an error bound for the current objective value based on the difference between consecutive solutions.

Requires references to the capacity vector, the current flow vector, the current waiting time, the next flow vector, and the next waiting time, respectively.

Returns an upper bound for the absolute error in the current solution.

The Frank-Wolfe algorithm includes a means for bounding the absolute error of the current solution based on the objective values of the previous solutions. Since our algorithm never explicitly evaluates the objective value (only values of the linearized objective), we instead use a looser but more easily calculated bound that involves the difference between consecutive linearized objective values.
*/
double NonlinearAssignment::obj_error(const vector<double> &capacities, const vector<double> &flows_old, double waiting_old, const vector<double> &flows_new, double waiting_new)
{
	// Calculate error term-by-term
	double total = waiting_old - waiting_new;
	for (int i = 0; i < Net->core_arcs.size(); i++)
		total += arc_cost(Net->core_arcs[i]->id, flows_old[i], capacities[i]) * (flows_old[i] - flows_new[i]);

	return abs(total);
}

/**
Updates the solution according to the convex combination found from the line search.

Requires a value for the convex parameter, followed by references to the current flow vector, the current waiting time, the next flow vector, and the next waiting time, respectively.

Returns the maximum elementwise difference between the current and updated solutions, and also updates the current solution in place as a convex combination of the two vectors.
*/
double NonlinearAssignment::solution_update(double lambda, vector<double> &flows_current, double &waiting_current, const vector<double> &flows_next, double waiting_next)
{
	double max_diff; // maximum elementwise difference
	double element; // temporary variable for the updated element

	// Update waiting time
	element = lambda*waiting_current + (1 - lambda)*waiting_next;
	max_diff = abs(waiting_current - element);
	waiting_current = element;

	// Update each flow variable
	for (int i = 0; i < flows_current.size(); i++)
	{
		element = lambda*flows_current[i] + (1 - lambda)*flows_next[i];
		max_diff = max(abs(flows_current[i] - element), max_diff);
		flows_current[i] = element;
	}

	return max_diff;
}
