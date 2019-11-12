#include "search.hpp"

/*
Early stop conditions (or decisions) to implement:
-Quit if one of the input files or a log file is missing. Missing certain log files is fine, and we can simply create new ones (as long as the folder is present).
-Quit if the initial operator cost is set to the default value of -1.
-If the initial flow vector is missing, default to the zero flow vector.
*/

/**
Signal handler for safe stop request.

Executes whenever the user presses [Ctrl]+[C] on the keyboard (or with the "raise(SIGINT)" command).

Sets the global stop request variable to True, which causes the main search loop to end after it completes its current iteration.
*/
void stop_request(int signum)
{
	cout << "\n****************************************" << endl;
	cout << "************ STOP REQUESTED ************" << endl;
	cout << "****************************************\n" << endl;
	cout << "Program will safely exit at end of current loop (which may take a while)." << endl;
	cout << "Do not close or data may be corrupted!\n" << endl;

	stopping = true;
}
