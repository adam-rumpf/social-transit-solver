#include "logger.hpp"

/// Converts a solution vector to a string by simply concatenating its digits separated by underscores.
string solution_string(const vector<int> &sol)
{
	string out = "";

	for (int i = 0; i < sol.size(); i++)
		out += to_string(sol[i]) + '_';
	out.pop_back();

	return out;
}

//////////// When loading the initial solution log, try to open the solution log file. If missing, exit(FILE_NOT_FOUND). If empty, exit(INCORRECT_FILE).
