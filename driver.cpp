/**
The main function for the TS/SA hybrid search algorithm.

Responsible for reading input data, initializing objects, and finally calling the search function, which is where most of the algorithm is actually conducted.

Includes a signal handler to allow for keyboard halt via [Ctrl]+[C].

The exit code should correspond to the circumstances of the exit.
*/

#include <csignal>
#include <iostream>
#include "DEFINITIONS.hpp"
#include "search.hpp"

using namespace std;

// Global search object pointer
Search * Solver;

// Global keyboard stop signal handler
void STOP_REQUEST(int);

/// Main driver.
int main()
{
	// Register signal handler for stop request
	signal(SIGINT, STOP_REQUEST);

	// Initialize search object
	Solver = new Search();

	// Call main solver
	Solver->solve();

	// Delete solver to automate shutdown process
	delete Solver;

	cin.get();////////////////////////////////////// remove later

	return SUCCESSFUL_EXIT;
}

/**
Signal handler for safe stop request.

Executes whenever the user presses [Ctrl]+[C] on the keyboard (or with the "raise(SIGINT)" command).

Sets the global solver's stop request variable to true, which causes the main search loop to end after it completes its current iteration.
*/
void STOP_REQUEST(int signum)
{
	cout << "\n\n****************************************" << endl;
	cout << "************ STOP REQUESTED ************" << endl;
	cout << "****************************************\n" << endl;
	cout << "Program will safely exit at end of current loop (which may take a while)." << endl;
	cout << "Do not close or data may be corrupted!" << endl;
	cout << "\n****************************************" << endl;
	cout << "************ STOP REQUESTED ************" << endl;
	cout << "****************************************\n" << endl;

	Solver->keyboard_halt = true;
}
