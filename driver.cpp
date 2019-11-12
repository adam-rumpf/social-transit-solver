/**
Main driver for setting up the TS/SA solution algorithm.

Contains this project's main function, which is mostly just used to load the data files, initialize objects, and then start the search algorithm.

Also contains a signal handler for safely terminating the search on keyboard command.
*/

/*
Early stop conditions (or decisions) to implement:
-Quit if one of the input files or a log file is missing. Missing certain log files is fine, and we can simply create new ones (as long as the folder is present).
-Quit if the initial operator cost is set to the default value of -1.
-If the initial flow vector is missing, default to the zero flow vector.
*/

#include <csignal>
#include <iostream>
#include <fstream>
#include <string>

// Define input file names
#define NODE_FILE "data/node_data.txt"
#define ARC_FILE "data/arc_data.txt"
#define OD_FILE "data/od_data.txt"
#define TRANSIT_FILE "data/transit_data.txt"
#define VEHICLE_FILE "data/vehicle_data.txt"
#define OBJECTIVE_FILE "data/objective_data.txt"
#define PROBLEM_FILE "data/problem_data.txt"
#define USER_COST_FILE "data/user_cost_data.txt"
#define OPERATOR_COST_FILE "data/operator_cost_data.txt"
#define ASSIGNMENT_FILE "data/assignment_data.txt"
#define FLOW_FILE "data/initial_flows.txt"
#define SEARCH_FILE "data/search_parameters.txt"

// Define output file names
#define METRIC_FILE "log/metrics.txt"
#define SOLUTION_LOG_FILE "log/solution.txt"
#define EVENT_LOG_FILE "log/event.txt"
#define MEMORY_LOG_FILE "log/memory.txt"
#define FINAL_SOLUTION_FILE "log/final.txt"
#define OBJECTIVE_LOG_FILE "log/objective.txt"

using namespace std;

// Global variables
bool stopping = false; // whether a stop has been requested via keyboard

// Function prototypes
void stop_request(int signum);

/// Main driver.
int main(int argc, char *argv[])
{
	// Command line argument test
	if (argc > 1)
	{
		// First argument is always the program's own path, so only pay attention to the additional arguments.
		cout << "Command line arguments:" << endl;
		for (int i = 1; i < argc; i++)
			cout << argv[i] << endl;
		cout << endl;
	}
	else
		cout << "No command line arguments read." << endl << endl;

	// Register event handler for keyboard stop request ([Ctrl]+[C])
	signal(SIGINT, stop_request);

	string line;
	ifstream vehicle_data("data/vehicle_data.txt");	
	if (vehicle_data.is_open())
	{
		while (getline(vehicle_data, line))
			cout << line << endl;
		vehicle_data.close();
	}
	else
		cout << "Unable to open file." << endl;

	// Test of keyboard halt.
	int count = 0;
	while (stopping == false)
	{
		count++;
		cout << "Infinite loop! Press [Ctrl]+[C]." << endl;
		if (count > 1000)
			raise(SIGINT);
	}

	cin.get(); ///////////////// remove later

	// Return appropriate exit code
	if (stopping == true)
		return 2;
	else
		return 0;
}

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
