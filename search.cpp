#include "search.hpp"

/// Search constructor initializes Network, Objective, Constraint, and logger objects.
Search::Search()
{
	Net = new Network(); // network object
	Obj = new Objective(Net); // objective function object
	Con = new Constraint(Net); // constraint function object
	ELog = new EventLog(); // event log object
	MLog = new MemoryLog(); // memory log object
	SLog = new SolutionLog(); // solution log object
}

/// Search constructor deletes Network, Objective, and Constraint objects created by the constructor.
Search::~Search()
{
	delete Net;
	delete Obj;
	delete Con;
	delete ELog;
	delete MLog;
	delete SLog;
}

/// Main driver of the solution algorithm.
void Search::solve()
{
	///////////////// Include optional arguments for the main solver.

	///////////////// Test a variety of circumstances, including saving/loading.

	cout << "Search initialized." << endl;




	/////// Note: If we end a loop due to stopping == true, we should safely quit with exit(KEYBOARD_HALT).
}
