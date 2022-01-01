/// Functions common to all search and logger classes.

#include "search.hpp"

/**
Reads the initial solution vector and objective value from the initial solution log file.

Returns a pair consisting of the initial fleet size vector along with its objective value, respectively.

Normally the current and best solutions along with their objective values are stored in the memory log file, and can be
passed to the search object in order to continue a search. If we are starting with a new search, however, the initial
solution information must be (re-)acquired from the initial solution log file.
*/
pair<vector<int>, double> get_initial_solution()
{
	// Initialize containers to temporarily hold row contents
	string row_sol;
	double row_obj;

	// Read specified file
	ifstream log_file;
	log_file.open(FILE_BASE + INPUT_SOLUTION_LOG_FILE);
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

			// Go through each piece of the line
			getline(stream, piece, '\t'); // Solution
			row_sol = piece;
			getline(stream, piece, '\t'); // Feasible
			for (int i = 0; i < UC_COMPONENTS; i++)
				// User cost components
				getline(stream, piece, '\t');
			getline(stream, piece, '\t'); // Constraint time
			getline(stream, piece, '\t'); // Objective
			row_obj = stod(piece);
			getline(stream, piece, '\t'); // Objective time
		}

		log_file.close();
	}
	else
	{
		cout << "Solution log file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}

	// Return solution pair
	return make_pair(str2vec(row_sol), row_obj);
}

/// Converts a solution vector to a string by simply concatenating its digits separated by underscores.
string vec2str(const vector<int> &sol)
{
	string out = "";

	for (int i = 0; i < sol.size(); i++)
		out += to_string(sol[i]) + DELIMITER;
	out.pop_back();

	return out;
}

/// Converts a solution string back into an integer solution vector.
vector<int> str2vec(string sol)
{
	vector<int> out;
	stringstream sol_stream(sol);
	string element;

	while (getline(sol_stream, element, DELIMITER))
		out.push_back(stoi(element));

	return out;
}
