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

#include <iostream>
#include <fstream>
#include <string>

using namespace std;

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

	cin.get(); ///////////////// remove later

	return 0;
}
