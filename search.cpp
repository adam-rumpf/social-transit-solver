#include "search.hpp"

/// Search constructor initializes Network, Objective, and Constraint objects.
Search::Search()
{
	Net = new Network(); // network object
	Obj = new Objective(Net); // objective function object
	Con = new Constraint(Net); // constraint function object
}

/// Search constructor deletes Network, Objective, and Constraint objects created by the constructor.
Search::~Search()
{
	delete Net;
	delete Obj;
	delete Con;

	// Only delete logger objects if they have been instantiated
	if (started == true)
	{
		delete EveLog;
		delete MemLog;
		delete SolLog;
	}
}

/// Main driver of the solution algorithm. Loads parameter files, calls main search loop, and handles final output.
void Search::solve()
{
	started = true;

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
			switch (count)
			{
				case 1:
					if (stoi(piece) == NEW_SEARCH)
						pickup = false;
					else if (stoi(piece) == CONTINUE_SEARCH)
						pickup = true;
					else
					{
						cout << "Unrecognized search continuation specification. Use '" << NEW_SEARCH << "' for a new search or '" << CONTINUE_SEARCH << "' to continue a previous search." << endl;
						exit(INCORRECT_FILE);
					}
					break;
				case 2:
					max_iterations = stoi(piece);
					break;
				case 4:
					temp_factor = stod(piece);
					break;
				case 5:
					attractive_max = stoi(piece);
					break;
				case 6:
					nbhd_add_lim1 = stoi(piece);
					break;
				case 7:
					nbhd_add_lim2 = stoi(piece);
					break;
				case 8:
					nbhd_drop_lim1 = stoi(piece);
					break;
				case 9:
					nbhd_drop_lim2 = stoi(piece);
					break;
				case 10:
					nbhd_swap_lim = stoi(piece);
					break;
				case 11:
					tenure_init = stod(piece);
					break;
				case 12:
					tenure_factor = stod(piece);
					break;
				case 13:
					nonimp_in_max = stoi(piece);
					break;
				case 14:
					nonimp_out_max = stoi(piece);
					break;
				case 15:
					step = stoi(piece);
					break;
			}
		}

		parameter_file.close();
	}
	else
	{
		cout << "Search parameter file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}

	// Initialize logger objects
	EveLog = new EventLog(pickup);
	MemLog = new MemoryLog(Net->lines.size(), pickup);
	SolLog = new SolutionLog(pickup);



	/////// Note: If we end a loop due to stopping == true, we should safely quit with exit(KEYBOARD_HALT).
}

/**
Reads the initial solution vector and objective value from the initial solution log file.

Returns a pair consisting of the initial fleet size vector along with its objective value, respectively.

Normally the current and best solutions along with their objective values are stored in the memory log file, and can be passed to the search object in order to continue a search. If we are starting with a new search, however, the initial solution information must be (re-)acquired from the initial solution log file.
*/
pair<vector<int>, double> get_initial_solution()
{
	// Initialize containers to temporarily hold row contents
	string row_sol;
	double row_obj;

	// Read specified file
	ifstream log_file;
	log_file.open(INPUT_SOLUTION_LOG_FILE);
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
		obj_file << iteration << '\t' << obj_current << '\t' << obj_best << fixed << setprecision(15) << endl;
		obj_file.close();
	}
}

/**
Memory log constructor either reads the memory log file into the object's local attributes or sets initial values.

Requires the size of the solution vector and a boolean argument to specify whether to begin by loading the existing memory log. If true, the object's attributes are initialized by reading the memory log file. If false, then the memory log file is ignored and the attributes are instead set according to the search parameter file.
*/
MemoryLog::MemoryLog(int size_in, bool pickup)
{
	// Get solution vector size and immediately resize corresponding vectors
	sol_size = size_in;
	add_tenure.resize(sol_size);
	drop_tenure.resize(sol_size);
	sol_current.resize(sol_size);
	sol_best.resize(sol_size);

	if (pickup == true)
		// If continuing from a previous run, read in the existing memory log
		load_memory();
	else
		// If starting a new run, initialize the memory structures using the search parameter file
		reset_memory();
}

/// Memory log destructor automatically calls the writing method to export the current memory structures to a file.
MemoryLog::~MemoryLog()
{
	save_memory();
}

/// Reads the memory log file to set memory log attributes.
void MemoryLog::load_memory()
{
	// Read memory log file
	ifstream log_file;
	log_file.open(MEMORY_LOG_FILE);
	if (log_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(log_file, line); // skip comment line

		int count = 0;
		queue<double> attractive_objectives; // queue of attractive objectives to associate with attractive solution vectors as they are read

		while (log_file.eof() == false)
		{
			count++;

			// Get whole line as a string stream
			getline(log_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);
			stream.exceptions(ifstream::failbit);

			// Expected data
			try
			{
				switch (count)
				{
				case 1:
					// ADD tenures
					for (int i = 0; i < sol_size; i++)
					{
						getline(stream, piece, '\t');
						add_tenure[i] = stod(piece);
					}
					break;
				case 2:
					// DROP tenures
					for (int i = 0; i < sol_size; i++)
					{
						getline(stream, piece, '\t');
						drop_tenure[i] = stod(piece);
					}
					break;
				case 3:
					// current solution
					for (int i = 0; i < sol_size; i++)
					{
						getline(stream, piece, '\t');
						sol_current[i] = stoi(piece);
					}
					break;
				case 4:
					// best solution
					for (int i = 0; i < sol_size; i++)
					{
						getline(stream, piece, '\t');
						sol_best[i] = stoi(piece);
					}
					break;
				case 5:
					// current objective
					getline(stream, piece, '\t');
					obj_current = stod(piece);
					break;
				case 6:
					// best objective
					getline(stream, piece, '\t');
					obj_best = stod(piece);
					break;
				case 7:
					// iteration number
					getline(stream, piece, '\t');
					iteration = stoi(piece);
					break;
				case 8:
					// inner nonimprovement counter
					getline(stream, piece, '\t');
					nonimp_in = stoi(piece);
					break;
				case 9:
					// outer nonimprovement counter
					getline(stream, piece, '\t');
					nonimp_out = stoi(piece);
					break;
				case 10:
					// tabu tenure
					getline(stream, piece, '\t');
					tenure = stod(piece);
					break;
				case 11:
					// simulated annealing temperature
					getline(stream, piece, '\t');
					temperature = stod(piece);
					break;
				case 12:
					// attractive solution objectives
					while (!stream.eof())
					{
						getline(stream, piece, '\t');
						attractive_objectives.push(stod(piece));
					}
					break;
				default:
					// attractive solution vectors
					vector<int> asol(sol_size);
					for (int i = 0; i < sol_size; i++)
					{
						getline(stream, piece, '\t');
						asol[i] = stoi(piece);
					}
					attractive_solutions.push_back(make_pair(asol, attractive_objectives.front()));
					attractive_objectives.pop();
				}
			}
			catch (ifstream::failure &e)
			{
				cout << "Mismatch between memory log row size and solution vector size." << endl;
				exit(INCOMPATIBLE_DATA);
			}
		}

		log_file.close();
	}
	else
	{
		cout << "Memory log file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}
}

/// Initializes memory log attributes according to search parameter file and the initial solution log file.
void MemoryLog::reset_memory()
{
	// Set fresh memory structure values
	iteration = 1;
	nonimp_in = 0;
	nonimp_out = 0;
	attractive_solutions.clear();
	for (int i = 0; i < sol_size; i++)
	{
		add_tenure[i] = 0;
		drop_tenure[i] = 0;
	}

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
			if (count == 3)
				temperature = stod(piece);
			if (count == 11)
				tenure = stod(piece);
		}

		param_file.close();
	}
	else
	{
		cout << "Search parameter file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}

	// Read initial solution log file and use for both current and best solutions
	pair<vector<int>, double> initial_sol = get_initial_solution();
	sol_current = initial_sol.first;
	sol_best = initial_sol.first;
	obj_current = initial_sol.second;
	obj_best = initial_sol.second;
}

/// Writes memory log attributes to the memory log file. Also calls a method to output the best known solution as a separate file.
void MemoryLog::save_memory()
{
	ofstream log_file(MEMORY_LOG_FILE);

	if (log_file.is_open())
	{
		// Write comment line
		log_file << "[add_tenure], [drop_tenure], [sol_current], [sol_best], obj_current, obj_best, iteration, nonimp_in, nonimp_out, tenure, temperature, [attractive_objectives], [[attractive_solutions]]" << fixed << setprecision(15) << endl;

		// Write tabu tenure vectors
		for (int i = 0; i < sol_size; i++)
			log_file << add_tenure[i] << '\t';
		log_file << endl;
		for (int i = 0; i < sol_size; i++)
			log_file << drop_tenure[i] << '\t';
		log_file << endl;

		// Write solution vectors
		for (int i = 0; i < sol_size; i++)
			log_file << sol_current[i] << '\t';
		log_file << endl;
		for (int i = 0; i < sol_size; i++)
			log_file << sol_best[i] << '\t';
		log_file << endl;

		// Write scalars
		log_file << obj_current << endl;
		log_file << obj_best << endl;
		log_file << iteration << endl;
		log_file << nonimp_in << endl;
		log_file << nonimp_out << endl;
		log_file << tenure << endl;
		log_file << temperature << endl;

		// Write attractive solution objective vector
		for (int i = 0; i < attractive_solutions.size(); i++)
			log_file << attractive_solutions[i].second << '\t';
		log_file << endl;

		// Write attractive solution vectors
		for (int i = 0; i < attractive_solutions.size(); i++)
		{
			for (int j = 0; j < sol_size; j++)
				log_file << attractive_solutions[i].first[j] << '\t';
			log_file << endl;
		}

		log_file.close();
		cout << "Successfully recorded memory log." << endl;
	}
	else
		cout << "Failed to write to memory log." << endl;

	// Also write the best known solution to its own file
	output_best();
}

/// Writes an output file containing only the best solution and its objective value.
void MemoryLog::output_best()
{
	ofstream log_file(FINAL_SOLUTION_FILE);

	if (log_file.is_open())
	{
		// Format output
		log_file << fixed << setprecision(15);

		// Write best solution vector
		for (int i = 0; i < sol_size; i++)
			log_file << sol_best[i] << '\t';
		log_file << endl;

		// Write best solution objective
		log_file << obj_best << endl;

		log_file.close();
		cout << "Successfully recorded final solution." << endl;
	}
	else
		cout << "Failed to write final solution." << endl;
}

/**
Solution log constructor reads the solution log file into the solution log unordered map.

Requires a boolean argument to specify whether to begin by loading the existing log files. If true, the existing solution log is loaded. If false, the existing log file is overwritten.
*/
SolutionLog::SolutionLog(bool pickup)
{
	if (pickup == true)
		// If continuing from a previous run, read in the existing log
		load_solution(OUTPUT_SOLUTION_LOG_FILE);
	else
		// If starting a new run, read in only the initial solution log
		load_solution(INPUT_SOLUTION_LOG_FILE);
}

/// Solution log destructor automatically calls the writing method to export the unordered map to a file.
SolutionLog::~SolutionLog()
{
	save_solution();
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

/**
Creates or updates a solution log entry for a given solution.

Requires a solution vector reference, feasibility status, constraint function vector reference, constraint calculation time, objective value, and objective calculation time, respectively.

If the solution vector was not already present in the log, this will add a new row. If it was already present, this will overwrite its previous information.
*/
void SolutionLog::create_row(const vector<int> &sol, int feas, const vector<double> &ucc, double uc_time, double obj, double obj_time)
{
	sol_log[vec2str(sol)] = make_tuple(feas, ucc, uc_time, obj, obj_time);
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
