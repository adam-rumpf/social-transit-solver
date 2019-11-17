/// Search class methods.

#include "search.hpp"

/// Search constructor initializes Network, Objective, and Constraint objects.
Search::Search()
{
	Net = new Network(); // network object
	Obj = new Objective(Net); // objective function object
	Con = new Constraint(Net); // constraint function object
	sol_size = Net->lines.size(); // get solution vector size
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
	MemLog = new MemoryLog(this, pickup);
	SolLog = new SolutionLog(pickup);

	// Initialize neighborhood search solution container and set references to its elements
	pair<pair<vector<int>, double>, pair<vector<int>, double>> nbhd_sol;
	vector<int> & nbhd_sol1 = nbhd_sol.first.first;
	vector<int> & nbhd_sol2 = nbhd_sol.second.first;
	double & nbhd_obj1 = nbhd_sol.first.second;
	double & nbhd_obj2 = nbhd_sol.second.second;
	nbhd_sol1.resize(sol_size);
	nbhd_sol2.resize(sol_size);

	// Main search loop

	while (iteration < max_iterations)
	{
		iteration++;
		cout << "Iteration " << iteration << " / " << max_iterations << endl;

		// Perform neighborhood search
		nbhd_sol = neighborhood_search();

		//////////////////////////////////////////////////////// placeholder neighborhood search test
		cout << "Neighborhood search results:" << endl;
		cout << "1st best objective: " << nbhd_obj1 << endl;
		cout << "1st best solution:" << endl;
		for (int i = 0; i < nbhd_sol1.size(); i++)
			cout << nbhd_sol1[i] << '\t';
		cout << endl;
		cout << "2nd best objective: " << nbhd_obj2 << endl;
		cout << "2nd best solution:" << endl;
		for (int i = 0; i < nbhd_sol2.size(); i++)
			cout << nbhd_sol2[i] << '\t';
		cout << endl;













		/////////////////////////////////////////////
		cout << "Breaking search for testing purposes." << endl;
		break;

		// Safely quit if a keyboard halt has been requested
		if (keyboard_halt == true)
		{
			save_data();
			exit(KEYBOARD_HALT);
		}
	}





	/////// Note: If we end a loop due to stopping == true, we should safely quit with exit(KEYBOARD_HALT). Write a method to safely quit. Call it either after the while loop ends, or at the end of the while loop if we've decided to quit (and then exit).
}

/**
Performs the neighborhood search of the tabu search/simulated annealing hybrid algorithm.

Returns a pair of pairs, with each pair containing a solution vector and objective value for the best and second best solutions found.
*/
pair<pair<vector<int>, double>, pair<vector<int>, double>> Search::neighborhood_search()
{





	/////////////////////////////////////////////////////////////////////////////////
	return make_pair(make_pair(vector<int>(Net->lines.size(), 0.0), 0.0), make_pair(vector<int>(Net->lines.size(), 0.0), 0.0));
}

/// Writes current memory structures to the output logs.
void Search::save_data()
{
	//////////////////////////////////////
	cout << "\n---------- SAVING DATA (PLACEHOLDER) ----------\n" << endl;
}
