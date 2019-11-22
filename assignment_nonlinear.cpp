/// Nonlinear cost assignment model class methods.

#include "assignment.hpp"

/// Nonlinear assignment constructor reads in model data from file and sets network pointer.
NonlinearAssignment::NonlinearAssignment(Network * net_in)
{
	Net = net_in;

	// Initialize submodel object
	Submodel = new ConstantAssignment(net_in);

	// Read assignment model data
	ifstream a_file;
	a_file.open(ASSIGNMENT_FILE);
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
				flow_tol = stod(value);
			if (count == 3)
				waiting_tol = stod(value);
			if (count == 4)
				max_iterations = stoi(value);
			if (count == 6)
				conical_alpha = stod(value);
			if (count == 7)
				conical_beta = stod(value);
		}

		a_file.close();
	}
	else
	{
		cout << "Assignment file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}
}

/// Nonlinear assignment destructor deletes the submodel created by the constructor.
NonlinearAssignment::~NonlinearAssignment()
{
	delete Submodel;
}

/**
Nonlinear cost assignment model evaluation for a given solution.

Requires a fleet size vector and an initial solution, which takes the form of a pair made up of a flow vector and a waiting time scalar.

Returns a pair containing a vector of flow values and a waiting time scalar.

The solution vector is used to determine the frequency and capacity of each line. Frequencies contribute to boarding arc costs for the common lines problem, while capacities contribute to an overcrowding penalty to model congestion.

The overall process being used here is the Frank-Wolfe algorithm, which iteratively solves the linear approximation of the nonlinear cost quadratic program. That linear approximation happens to be an instance of the constant-cost LP whose costs are based on the current solution.
*/
pair<vector<double>, double> NonlinearAssignment::calculate(const vector<int> &fleet, const pair<vector<double>, double> &initial_sol)
{
	// Initialize variables
	pair<vector<double>, double> sol_next; // flow/waiting pair calculated as the linearized submodel solution
	pair<vector<double>, double> sol_previous; // flow/waiting pair for the previous solution
	int iteration = 0; // current iteration number
	double error = INFINITY; // current solution error bound
	pair<double, double> change = make_pair(INFINITY, INFINITY); // flow/waiting time differences between consecutive solutions

	// Calculate line arc capacities
	vector<double> capacities(Net->core_arcs.size(), INFINITY);
	for_each(Net->line_arcs.begin(), Net->line_arcs.end(), [&](Arc * a)
	{
		capacities[a->id] = Net->lines[a->line]->capacity(fleet[a->line]);
	});

	// Calculate arc costs based on initial flow
	vector<double> arc_costs(Net->core_arcs.size());
	for_each(Net->core_arcs.begin(), Net->core_arcs.end(), [&](Arc * a)
	{
		arc_costs[a->id] = arc_cost(a->id, initial_sol.first[a->id], capacities[a->id]);
	});

	// Solve constant-cost model once to obtain an initial solution
	sol_previous = Submodel->calculate(fleet, arc_costs);

	// Main Frank-Wolfe loop

	while ((iteration < max_iterations) && (error > error_tol) && ((change.first > flow_tol) || (change.second > waiting_tol)))
	{
		// Loop continues until achieving sufficiently low error or reaching an iteration cutoff
		iteration++;
		cout << '.';

		// Update all arc costs based on the current flow
		for_each(Net->core_arcs.begin(), Net->core_arcs.end(), [&](Arc * a)
		{
			arc_costs[a->id] = arc_cost(a->id, sol_previous.first[a->id], capacities[a->id]);
		});

		// Solve constant-cost model for given cost vector
		sol_next = Submodel->calculate(fleet, arc_costs);

		// Calculate new error bound
		error = obj_error(capacities, sol_previous.first, sol_previous.second, sol_next.first, sol_next.second);

		// Update solution as successive average of consecutive solutions and get maximum elementwise difference
		change = solution_update(1 - (1.0 / iteration), sol_previous.first, sol_previous.second, sol_next.first, sol_next.second);
	}

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

Updates the current solution in place as a convex combination of the two vectors.

Also returns a pair containing the maximum elementwise flow vector change and the waiting time change, respectively.
*/
pair<double, double> NonlinearAssignment::solution_update(double lambda, vector<double> &flows_current, double &waiting_current, const vector<double> &flows_next, double waiting_next)
{
	double max_flow_diff = 0.0; // maximum elementwise flow difference
	double waiting_diff; // waiting time difference
	double element; // temporary variable for the updated element

	// Update waiting time
	element = lambda*waiting_current + (1 - lambda)*waiting_next;
	waiting_diff = abs(waiting_current - element);
	waiting_current = element;

	// Update each flow variable
	for (int i = 0; i < flows_current.size(); i++)
	{
		element = lambda*flows_current[i] + (1 - lambda)*flows_next[i];
		max_flow_diff = max(abs(flows_current[i] - element), max_flow_diff);
		flows_current[i] = element;
	}

	return make_pair(max_flow_diff, waiting_diff);
}
