#include "search.hpp"

/// Search constructor initializes Network, Objective, and Constraint objects, and sets the static stopping variable.
Search::Search()
{
	Net = new Network(); // network object
	Obj = new Objective(Net); // objective function object
	Con = new Constraint(Net); // constraint function object
}

/// Main driver of the solution algorithm.
void Search::solve()
{
	///////////////// Include optional arguments for the main solver.

	cout << "Search initialized." << endl;

	///////////// As an initial test before we move forward, manually call the objective and constraint functions a couple of times to see that they are working. Look at the single solver to see some of the commands it uses.

	///////////// Then try loading data under a variety of circumstances, specifically related to the initial solution (depending on the contents of the solution log and the memory log, and whether or not we are starting fresh).




	/////// Note: If we end a loop due to stopping == true, we should safely quit with exit(KEYBOARD_HALT).
}
