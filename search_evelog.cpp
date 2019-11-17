/// Event log class methods.

#include "search.hpp"

/**
Event log constructor writes headers of event log files and clears if necessary.

Requires a boolean argument to specify whether to continue or restart the log files. If true, the existing files are appended to. If false, the existing files are overwritten.

Also reads the search parameter file to obtain the maximum number of iterations.
*/
EventLog::EventLog(bool pickup)
{
	string event_header; // event log header

	if (pickup == true)
	{
		// If continuing from a previous run, set mode to append and create a longer event header
		mode = ofstream::app;
		event_header = "\n############################################################\nResuming Session\n############################################################\n";
	}
	else
	{
		// If starting a new run, set mode to truncate, create a fresh event header, and rewrite the objective log header
		mode = ofstream::trunc;
		double obj_init = get_initial_solution().second;
		event_header = "New search initialized.\nInitial objective value: " + to_string(obj_init) + '\n';

		// Write objective log header and initial value
		ofstream obj_file(OBJECTIVE_LOG_FILE, mode);
		if (obj_file.is_open())
		{
			obj_file << "Iteration\tObj_Current\tObj_Best\n0\t" << fixed << setprecision(15);
			obj_file << obj_init << '\t' << obj_init << endl;
			obj_file.close();
		}
	}

	// Always write event log header
	ofstream event_file(EVENT_LOG_FILE, mode);
	if (event_file.is_open())
	{
		event_file << event_header;
		event_file.close();
	}

	// After header creation we should always append
	mode = ofstream::app;

	// Read search parameter file
	ifstream param_file;
	param_file.open(SEARCH_FILE);
	if (param_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(param_file, line); // skip comment line

		int count = 0;

		while (param_file.eof() == false)
		{
			count++;

			// Get whole line as a string stream
			getline(param_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);

			// Go through each piece of the line
			getline(stream, piece, '\t'); // Label
			getline(stream, piece, '\t'); // Value

										  // Expected data
			if (count == 2)
				max_iterations = stoi(piece);
		}

		param_file.close();
	}
	else
		max_iterations = -1;
}

/**
Appends an iteration summary to the event log file.

Requires the following arguments, respectively:
	current iteration number
	current objective value
	best objective value
*/
void EventLog::log_iteration(int iteration, double obj_current, double obj_best)
{
	// Write to event log file
	ofstream event_file(EVENT_LOG_FILE, mode);
	if (event_file.is_open())
	{
		// Write iteration header
		event_file << "\n==================================================\nIteration " << iteration << " / " << max_iterations << "\n==================================================\n\n";

		///////////////////////////////////////////////////////////////////

		event_file.close();
	}

	// Write to objective log file
	ofstream obj_file(OBJECTIVE_LOG_FILE, mode);
	if (obj_file.is_open())
	{
		obj_file << fixed << setprecision(15);
		obj_file << iteration << '\t' << obj_current << '\t' << obj_best << fixed << setprecision(15) << endl;
		obj_file.close();
	}
}
