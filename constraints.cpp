#include "constraints.hpp"

/**
Constraint object constructor that loads constraint file input and sets a network object pointer.

Requires the names of the user cost file, operator cost file, assignment model file, and a network object pointer.
*/
Constraint::Constraint(string us_file_name, string op_file_name, string assignment_file_name, Network * net_in)
{
	Net = net_in;
	stop_size = Net->stop_nodes.size();
	sol_pair.first.resize(Net->core_arcs.size(), 0.0);

	// Initialize assignment model object
	Assignment = new NonlinearAssignment(assignment_file_name, net_in);

	// Read constraint data
	ifstream us_file;
	us_file.open(us_file_name);
	if (us_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(us_file, line); // skip comment line
		int count = 0;

		while (us_file.eof() == false)
		{
			count++;

			// Get whole line as a string stream
			getline(us_file, line);
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
				initial_user_cost = stod(value);
			if (count == 2)
				uc_percent_increase = stod(value);
			if (count == 4)
				riding_weight = stod(value);
			if (count == 5)
				walking_weight = stod(value);
			if (count == 6)
				waiting_weight = stod(value);
		}

		us_file.close();
	}
	else
	{
		cout << "Constraint file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}
}

/**
Evaluates the constraint functions for a given solution.

Requires a solution vector.

Returns a pair whose first element is the feasibility result (1 for feasible, 0 for infeasible) and whose second element is a vector of constraint function elements, ordered in the same way as the solution log file.

All of the constraint functions are evaluated using either the solution vector directly, or using the flow vector produced by the assignment model.
*/
pair<int, vector<double>> Constraint::calculate(vector<int> &sol)
{
	// Feed solution to assignment model to calculate flow vector
	sol_pair = Assignment->calculate(sol, sol_pair);

	// Calculate user cost components
	vector<double> ucc = user_cost_components();

	// Calculate total user cost and compare to the bound to determine feasibility
	double total_user_cost = riding_weight*ucc[0] + walking_weight*ucc[1] + waiting_weight*ucc[2];
	int feas = 1;
	if (total_user_cost > (1 + uc_percent_increase)*initial_user_cost)
		feas = 0;

	return make_pair(feas, ucc);
}

/**
Converts user flow vector and waiting time scalar into a vector of the user cost components.

Returns a vector of the user cost components, in the order of the solution log columns.
*/
vector<double> Constraint::user_cost_components()
{
	vector<double> uc(3, 0);
	uc[2] = sol_pair.second;

	// In-vehicle riding time
	for (int i = 0; i < Net->line_arcs.size(); i++)
		uc[0] += sol_pair.first[Net->line_arcs[i]->id] * Net->line_arcs[i]->cost;

	// Walking time
	for (int i = 0; i < Net->walking_arcs.size(); i++)
		uc[1] += sol_pair.first[Net->walking_arcs[i]->id] * Net->walking_arcs[i]->cost;

	return uc;
}
