/// Memory log class methods.

#include "search.hpp"

/**
Memory log constructor either reads the memory log file into the object's local attributes or sets initial values.

Requires the size of the solution vector and a boolean argument to specify whether to begin by loading the existing
memory log. If true, the object's attributes are initialized by reading the memory log file. If false, then the memory
log file is ignored and the attributes are instead set according to the search parameter file.
*/
MemoryLog::MemoryLog(Search * search_in, bool pickup)
{
	// Set pointer and immediately resize corresponding vectors
	Solver = search_in;
	sol_size = Solver->Net->lines.size();
	Solver->add_tenure.resize(sol_size);
	Solver->drop_tenure.resize(sol_size);
	Solver->sol_current.resize(sol_size);
	Solver->sol_best.resize(sol_size);

	if (pickup == true)
		// If continuing from a previous run, read in the existing memory log
		load_memory();
	else
		// If starting a new run, initialize the memory structures using the search parameter file
		reset_memory();
}

/// Reads the memory log file to set memory log attributes.
void MemoryLog::load_memory()
{
	// Read memory log file
	ifstream log_file;
	log_file.open(FILE_BASE + MEMORY_LOG_FILE);
	if (log_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(log_file, line); // skip comment line

		int count = 0;
		queue<double> attractive_objectives; // queue of att objs to associate with att sol vectors as they are read

		while (log_file.eof() == false)
		{
			count++;

			// Get whole line as a string stream
			getline(log_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);
			stream.exceptions(ios_base::failbit);

			// Expected data
			switch (count)
			{
			case 1:
				// ADD tenures
				for (int i = 0; i < sol_size; i++)
				{
					getline(stream, piece, '\t');
					Solver->add_tenure[i] = stod(piece);
				}
				break;
			case 2:
				// DROP tenures
				for (int i = 0; i < sol_size; i++)
				{
					getline(stream, piece, '\t');
					Solver->drop_tenure[i] = stod(piece);
				}
				break;
			case 3:
				// current solution
				for (int i = 0; i < sol_size; i++)
				{
					getline(stream, piece, '\t');
					Solver->sol_current[i] = stoi(piece);
				}
				break;
			case 4:
				// best solution
				for (int i = 0; i < sol_size; i++)
				{
					getline(stream, piece, '\t');
					Solver->sol_best[i] = stoi(piece);
				}
				break;
			case 5:
				// current objective
				getline(stream, piece, '\t');
				Solver->obj_current = stod(piece);
				break;
			case 6:
				// best objective
				getline(stream, piece, '\t');
				Solver->obj_best = stod(piece);
				break;
			case 7:
				// iteration number
				getline(stream, piece, '\t');
				Solver->iteration = stoi(piece);
				break;
			case 8:
				// inner nonimprovement counter
				getline(stream, piece, '\t');
				Solver->nonimp_in = stoi(piece);
				break;
			case 9:
				// outer nonimprovement counter
				getline(stream, piece, '\t');
				Solver->nonimp_out = stoi(piece);
				break;
			case 10:
				// tabu tenure
				getline(stream, piece, '\t');
				Solver->tenure = stod(piece);
				break;
			case 11:
				// simulated annealing temperature
				getline(stream, piece, '\t');
				Solver->temperature = stod(piece);
				break;
			case 12:
				// attractive solution objectives
				while (!stream.eof())
				{
					try
					{
						getline(stream, piece, '\t');
						attractive_objectives.push(stod(piece));
					}
					catch (ios_base::failure &e) {} // catch eof errors due to trailing whitespace
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
				Solver->attractive_solutions.push_back(make_pair(asol, attractive_objectives.front()));
				attractive_objectives.pop();
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
	Solver->iteration = 0;
	Solver->nonimp_in = 0;
	Solver->nonimp_out = 0;
	Solver->attractive_solutions.clear();
	for (int i = 0; i < sol_size; i++)
	{
		Solver->add_tenure[i] = 0;
		Solver->drop_tenure[i] = 0;
	}

	// Read search parameter file
	ifstream param_file;
	param_file.open(FILE_BASE + SEARCH_FILE);
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
				Solver->temperature = stod(piece);
			if (count == 11)
				Solver->tenure = stod(piece);
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
	Solver->sol_current = initial_sol.first;
	Solver->sol_best = initial_sol.first;
	Solver->obj_current = initial_sol.second;
	Solver->obj_best = initial_sol.second;
}

/**
Writes memory log attributes to the memory log file. Also calls a method to output the best known solution as a separate
file.
*/
void MemoryLog::save_memory()
{
	ofstream log_file(FILE_BASE + MEMORY_LOG_FILE);

	if (log_file.is_open())
	{
		// Write comment line
		log_file << "[add_tenure], [drop_tenure], [sol_current], [sol_best], obj_current, obj_best, iteration, " <<
			"nonimp_in, nonimp_out, tenure, temperature, [attractive_objectives], [[attractive_solutions]]" << fixed <<
			setprecision(15) << endl;

		// Write tabu tenure vectors
		for (int i = 0; i < sol_size; i++)
			log_file << Solver->add_tenure[i] << '\t';
		log_file << endl;
		for (int i = 0; i < sol_size; i++)
			log_file << Solver->drop_tenure[i] << '\t';
		log_file << endl;

		// Write solution vectors
		for (int i = 0; i < sol_size; i++)
			log_file << Solver->sol_current[i] << '\t';
		log_file << endl;
		for (int i = 0; i < sol_size; i++)
			log_file << Solver->sol_best[i] << '\t';
		log_file << endl;

		// Write scalars
		log_file << Solver->obj_current << endl;
		log_file << Solver->obj_best << endl;
		log_file << Solver->iteration << endl;
		log_file << Solver->nonimp_in << endl;
		log_file << Solver->nonimp_out << endl;
		log_file << Solver->tenure << endl;
		log_file << Solver->temperature << endl;

		// Write attractive solution objective vector
		for_each(Solver->attractive_solutions.begin(), Solver->attractive_solutions.end(),
			[&](pair<vector<int>, double> sol)
		{
			log_file << sol.second << '\t';
		});
		log_file << endl;

		// Write attractive solution vectors
		for_each(Solver->attractive_solutions.begin(), Solver->attractive_solutions.end(),
			[&](pair<vector<int>, double> sol)
		{
			for (int i = 0; i < sol_size; i++)
				log_file << sol.first[i] << '\t';
			log_file << endl;
		});

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
	ofstream log_file(FILE_BASE + FINAL_SOLUTION_FILE);

	if (log_file.is_open())
	{
		// Format output
		log_file << fixed << setprecision(15);

		// Write best solution vector
		for (int i = 0; i < sol_size; i++)
			log_file << Solver->sol_best[i] << '\t';
		log_file << endl;

		// Write best solution objective
		log_file << Solver->obj_best << endl;

		log_file.close();
		cout << "Successfully recorded solution." << endl;
	}
	else
		cout << "Failed to write solution." << endl;
}
