#include "logger.hpp"

/**
Solution log constructor reads the solution log file into the solution log unordered map.

Accepts a boolean argument (default true) to specify whether to begin by loading the existing log files. If true, the existing solution log is loaded. If false, the existing log file is overwritten.
*/
SolutionLog::SolutionLog(bool pickup=true)
{
	if (pickup == true)
		// If continuing from a previous run, read in the existing log
		read_solution(OUTPUT_SOLUTION_LOG_FILE);
	else
		// If starting a new run, read in only the initial solution log
		read_solution(INPUT_SOLUTION_LOG_FILE);
}

/// Solution log destructor automatically calls the writing method to export the unordered map to a file.
SolutionLog::~SolutionLog()
{
	write_solution();
}

/// Reads a given solution log file into the solution dictionary.
void SolutionLog::read_solution(string in_file)
{
	// Read specified file
	ifstream log_file;
	log_file.open(in_file);
	if (log_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(log_file, line); // skip comment line

		while (log_file.eof() == false)
		{
			// Get whole line as a string stream
			getline(log_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);

			// Initialize containers to temporarily hold row contents
			string row_sol;
			int row_feas;
			vector<double> row_uc(UC_COMPONENTS);
			double row_con_time;
			double row_obj;
			double row_obj_time;

			// Go through each piece of the line
			getline(stream, piece, '\t'); // Solution
			row_sol = piece;
			getline(stream, piece, '\t'); // Feasible
			row_feas = stoi(piece);
			for (int i = 0; i < UC_COMPONENTS; i++)
			{
				// User cost components
				getline(stream, piece, '\t');
				row_uc[i] = stod(piece);
			}
			getline(stream, piece, '\t'); // Constraint time
			row_con_time = stod(piece);
			getline(stream, piece, '\t'); // Objective
			row_obj = stod(piece);
			getline(stream, piece, '\t'); // Objective time
			row_obj_time = stod(piece);

			// Create dictionary entry
			sol_log[row_sol] = make_tuple(row_feas, row_uc, row_con_time, row_obj, row_obj_time);
		}

		log_file.close();
	}
	else
	{
		cout << "Solution log file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}
}

/// Writes the current contents of the solution log to the solution log output file.
void SolutionLog::write_solution()
{
	ofstream log_file(OUTPUT_SOLUTION_LOG_FILE);

	if (log_file.is_open())
	{
		// Write comment line
		log_file << "Solution\tFeasible\tUC_Riding\tUC_Walking\tUC_Waiting\tCon_Time\tObjective\tObj_Time" << fixed << setprecision(15) << endl;

		// Write rows by iterating through dictionary (order is arbitrary)
		for (auto it = sol_log.begin(); it != sol_log.end(); it++)
		{
			log_file << it->first << '\t' << get<SOL_LOG_FEAS>(it->second) << '\t';
			for (int i = 0; i < UC_COMPONENTS; i++)
				log_file << get<SOL_LOG_UC>(it->second)[i] << '\t';
			log_file << get<SOL_LOG_CON_TIME>(it->second) << '\t' << get<SOL_LOG_OBJ>(it->second) << '\t' << get<SOL_LOG_OBJ_TIME>(it->second) << endl;
		}

		log_file.close();
		cout << "Successfully recorded solution log." << endl;
	}
	else
		cout << "Failed to write to solution log." << endl;
}

/// Converts a solution vector to a string by simply concatenating its digits separated by underscores.
string SolutionLog::solution_string(const vector<int> &sol)
{
	string out = "";

	for (int i = 0; i < sol.size(); i++)
		out += to_string(sol[i]) + DELIMITER;
	out.pop_back();

	return out;
}

/// Converts a solution string back into an integer solution vector.
vector<int> SolutionLog::solution_string_vector(string sol)
{
	vector<int> out;
	stringstream sol_stream(sol);
	string element;

	while (getline(sol_stream, element, DELIMITER))
		out.push_back(stoi(element));

	return out;
}
