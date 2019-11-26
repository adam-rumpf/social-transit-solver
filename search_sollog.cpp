/// Solution log class methods.

#include "search.hpp"

/**
Solution log constructor reads the solution log file into the solution log unordered map.

Requires a boolean argument to specify whether to begin by loading the existing log files. If true, the existing solution log is loaded. If false, the existing log file is overwritten.
*/
SolutionLog::SolutionLog(bool pickup)
{
	if (pickup == true)
		// If continuing from a previous run, read in the existing log
		load_solution(FILE_BASE + OUTPUT_SOLUTION_LOG_FILE);
	else
		// If starting a new run, read in only the initial solution log
		load_solution(FILE_BASE + INPUT_SOLUTION_LOG_FILE);
}

/// Reads a given solution log file into the solution dictionary.
void SolutionLog::load_solution(string in_file)
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
void SolutionLog::save_solution()
{
	ofstream log_file(FILE_BASE + OUTPUT_SOLUTION_LOG_FILE);

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

/**
Creates or updates a solution log entry for a given solution.

Requires a solution vector reference, feasibility status, constraint function vector reference, constraint calculation time, objective value, and objective calculation time, respectively.

If the solution vector was not already present in the log, this will add a new row. If it was already present, this will overwrite its previous information.
*/
void SolutionLog::create_row(const vector<int> &sol, int feas, const vector<double> &ucc, double uc_time, double obj, double obj_time)
{
	sol_log[vec2str(sol)] = make_tuple(feas, ucc, uc_time, obj, obj_time);
}

/// Creates a partial solution log entry for a given solution, objective value, and objective calculation time.
void SolutionLog::create_partial_row(const vector<int> &sol, double obj, double obj_time)
{
	create_row(sol, FEAS_UNKNOWN, vector<double>(UC_COMPONENTS, FEAS_UNKNOWN), FEAS_UNKNOWN, obj, obj_time);
}

/// Returns a boolean indicating whether a given solution vector is present in the solution log.
bool SolutionLog::solution_exists(const vector<int> &sol)
{
	if (sol_log.count(vec2str(sol)) > 0)
		return true;
	else
		return false;
}

/// Returns a tuple containing the feasibility status, vector of constraint function elements, and objective value for a given solution vector.
tuple<int, vector<double>, double> SolutionLog::lookup_row(const vector<int> &sol)
{
	tuple<int, vector<double>, double, double, double> entry = sol_log[vec2str(sol)]; // raw log entry

	// Output tuple of specified elements
	return make_tuple(get<SOL_LOG_FEAS>(entry), get<SOL_LOG_UC>(entry), get<SOL_LOG_OBJ>(entry));
}

/// Returns a pair containing the feasibility status and objective value for a given solution vector.
pair<int, double> SolutionLog::lookup_row_quick(const vector<int> &sol)
{
	tuple<int, vector<double>, double, double, double> entry = sol_log[vec2str(sol)]; // raw log entry

	// Output tuple of specified elements
	return make_pair(get<SOL_LOG_FEAS>(entry), get<SOL_LOG_OBJ>(entry));
}

/**
Modifies the feasibility status, constraint function vector, and constraint evaluation time for a previously-logged solution.

Requires a solution vector reference, feasibility status, constraint vector reference, and constraint time, respectively.

This method is used to fill in constraint evaluation information for solutions whose constraint evaluation had previously been skipped during a neighborhood search.
*/
void SolutionLog::update_row(const vector<int> &sol, int feas, const vector<double> &ucc, double uc_time)
{
	string key = vec2str(sol); // solution log key

	// Update each tuple entry individually
	get<SOL_LOG_FEAS>(sol_log[key]) = feas;
	get<SOL_LOG_UC>(sol_log[key]) = ucc;
	get<SOL_LOG_CON_TIME>(sol_log[key]) = uc_time;
}
