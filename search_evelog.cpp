/// Event log class methods.

#include "search.hpp"

/**
Event log constructor writes headers of event log files and clears if necessary.

Requires a boolean argument to specify whether to continue or restart the log files. If true, the existing files are appended to. If false, the existing files are overwritten.

Also sets parameters to default values.
*/
EventLog::EventLog(bool pickup)
{
	if (pickup == true)
		// If continuing from a previous run, set mode to append
		mode = ofstream::app;
	else
	{
		// If starting a new run, set mode to truncate and write a comment line
		mode = ofstream::trunc;

		// Get initial solution for first row
		pair<vector<int>, double> init_sol = get_initial_solution();

		// Load search parameters
		ifstream parameter_file;
		parameter_file.open(SEARCH_FILE);
		if (parameter_file.is_open())
		{
			string line, piece; // whole line and line element being read
			getline(parameter_file, line); // skip comment line
			int count = 0;

			while (parameter_file.eof() == false)
			{
				count++;

				// Get whole line as a string stream
				getline(parameter_file, line);
				if (line.size() == 0)
					// Break for blank line at file end
					break;
				stringstream stream(line);

				// Go through each piece of the line
				getline(stream, piece, '\t'); // Label
				getline(stream, piece, '\t'); // Value

				// Expected data
				if (count == 3)
					temperature = stod(piece);
				if (count == 11)
					tenure = stod(piece);
			}

			parameter_file.close();
		}

		// Write to event log file
		ofstream event_file(EVENT_LOG_FILE, mode);
		if (event_file.is_open())
		{
			// Comment line and formatting
			event_file << "Iteration\tObj_Current\tObj_Best\tNew_Best\tCase\tSA_Prob\tJump\tNonimp_Int\tNonimp_Out\tTenure\tTemperature\tADD\tDROP\tObj_Lookups\tCon_Lookups\tObj_Evals\tCon_Evals\tADD_First\tDROP_First\tADD_Second\tDROP_Second\tSWAPs\tTotal_Time\tSolution" << fixed << setprecision(15) << endl;

			// Initial solution row
			event_file << 0 << '\t'; // Iteration
			event_file << init_sol.second << '\t'; // Obj_Current
			event_file << init_sol.second << '\t'; // Obj_Best
			event_file << 1 << '\t'; // New_Best
			event_file << NO_ID << '\t'; // Case
			event_file << NO_ID << '\t'; // SA_Prob
			event_file << 0 << '\t'; // Jump
			event_file << 0 << '\t'; // Nonimp_In
			event_file << 0 << '\t'; // Nonimp_Out
			event_file << tenure << '\t'; // Tenure
			event_file << temperature << '\t'; // Temperature
			event_file << NO_ID << '\t'; // ADD
			event_file << NO_ID << '\t'; // DROP
			event_file << 0 << '\t'; // Obj_Lookups
			event_file << 0 << '\t'; // Con_Lookups
			event_file << 0 << '\t'; // Obj_Evals
			event_file << 0 << '\t'; // Con_Evals
			event_file << 0 << '\t'; // ADD_First
			event_file << 0 << '\t'; // DROP_First
			event_file << 0 << '\t'; // ADD_Second
			event_file << 0 << '\t'; // DROP_Second
			event_file << 0 << '\t'; // SWAPs
			event_file << 0.0 << '\t'; // Total_Time
			event_file << vec2str(init_sol.first); // Solution
			event_file << endl;

			event_file.close();
		}
	}

	// After header creation we should always append
	mode = ofstream::app;

	// Reset memory
	reset();
}

/// Appends an iteration row to the event log file for a given solution and resets the event log object's memory.
void EventLog::log_iteration(const vector<int> &sol)
{
	// Write to event log file
	ofstream event_file(EVENT_LOG_FILE, mode);
	if (event_file.is_open())
	{
		// Write event log parameters
		event_file << fixed << setprecision(15);
		event_file << iteration << '\t'; // Iteration
		event_file << obj_current << '\t'; // Obj_Current
		event_file << obj_best << '\t'; // Obj_Best
		event_file << new_best << '\t'; // New_Best
		event_file << event_case << '\t'; // Case
		event_file << sa_prob << '\t'; // SA_Prob
		event_file << jump << '\t'; // Jump
		event_file << nonimp_in << '\t'; // Nonimp_In
		event_file << nonimp_out << '\t'; // Nonimp_Out
		event_file << tenure << '\t'; // Tenure
		event_file << temperature << '\t'; // Temperature
		event_file << add_id << '\t'; // ADD
		event_file << drop_id << '\t'; // DROP
		event_file << obj_lookups << '\t'; // Obj_Lookups
		event_file << con_lookups << '\t'; // Con_Lookups
		event_file << obj_evals << '\t'; // Obj_Evals
		event_file << con_evals << '\t'; // Con_Evals
		event_file << add_first << '\t'; // ADD_First
		event_file << drop_first << '\t'; // DROP_First
		event_file << add_second << '\t'; // ADD_Second
		event_file << drop_second << '\t'; // DROP_Second
		event_file << swaps << '\t'; // SWAPs
		event_file << total_time << '\t'; // Total_Time
		event_file << vec2str(sol); // Solution
		event_file << endl;

		event_file.close();
	}

	// Reset memory
	reset();
}

/// Resets the event log object's internal memory to default values.
void EventLog::reset()
{
	new_best = 0;
	event_case = NO_ID;
	sa_prob = NO_ID;
	jump = 0;
	nonimp_in = 0;
	nonimp_out = 0;
	tenure = NO_ID;
	temperature = NO_ID;
	add_id = NO_ID;
	drop_id = NO_ID;
	obj_lookups = 0;
	con_lookups = 0;
	obj_evals = 0;
	con_evals = 0;
	add_first = 0;
	drop_first = 0;
	add_second = 0;
	drop_second = 0;
	swaps = 0;
}

/// Writes a row of -1 to the solution log in order to indicate a keyboard halt.
void EventLog::halt()
{
	// Write to event log file
	ofstream event_file(EVENT_LOG_FILE, mode);
	if (event_file.is_open())
	{
		for (int i = 0; i < EVENT_LOG_COLUMNS - 1; i++)
			event_file << "-1\t";
		event_file << "-1" << endl;

		event_file.close();
	}
}
