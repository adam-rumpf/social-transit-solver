#include "search.hpp"

/*
Early stop conditions (or decisions) to implement:
-Quit if one of the input files or a log file is missing. Missing certain log files is fine, and we can simply create new ones (as long as the folder is present).
-Quit if the initial operator cost is set to the default value of -1.
-If the initial flow vector is missing, default to the zero flow vector.
*/

///////////////// Include optional arguments for the main solver.

/// Search constructor initializes Network, Objective, and Constraint objects, and sets the static stopping variable.
Search::Search()
{
	Net = new Network(NODE_FILE, ARC_FILE, OD_FILE, TRANSIT_FILE, VEHICLE_FILE, PROBLEM_FILE); // network object
	Obj = new Objective(OBJECTIVE_FILE, Net); // objective function object
	Con = new Constraint(USER_COST_FILE, OPERATOR_COST_FILE, ASSIGNMENT_FILE, Net); // constraint function object
}

/// Main driver of the solution algorithm.
void Search::solve()
{
	cout << "Solution algorithm initialized." << endl;

	if (keyboard_halt == true)
	{
		cout << "keyboard_halt is true" << endl;
		exit(KEYBOARD_HALT);
	}
	else
		cout << "keyboard_halt is false" << endl;
}
