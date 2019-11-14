#include "logger.hpp"

/**
Solution log constructor reads the solution log file into the solution log unordered map.

Accepts a boolean argument (default true) to specify whether to begin by loading the existing log files. If true, the existing solution log is loaded. If false, the existing log file is wiped clean except for the initial solution row.
*/
SolutionLog::SolutionLog(bool pickup=true)
{
	////////////////////////////////////////

	//////////// When loading the initial solution log, try to open the solution log file. If missing, exit(FILE_NOT_FOUND). If empty, exit(INCORRECT_FILE).
	// read INPUT_SOLUTION_LOG_FILE
}

/// Solution log destructor automatically calls the writing method to export the unordered map to a file.
SolutionLog::~SolutionLog()
{
	write_solution();
}

/// Writes the current contents of the solution log to the solution log output file.
void SolutionLog::write_solution()
{
	//////////////////////////////////////////
	// write to OUTPUT_SOLUTION_LOG_FILE
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
vector<int> SolutionLog::solution_string_vector(const string &sol)
{
	vector<int> out;
	stringstream sol_stream(sol);
	string element;

	while (getline(sol_stream, element, DELIMITER))
		out.push_back(stoi(element));

	return out;
}
