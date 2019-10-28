/**
Main driver for setting up the TS/SA solution algorithm.

Contains this project's main function, which is mostly just used to load the data files, initialize objects, and then start the search algorithm.

Also contains a signal handler for safely terminating the search on keyboard command.
*/

#include <iostream>
#include <fstream>
#include <string>
using namespace std;

int main()
{
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

	cin.get();

	return 0;
}
