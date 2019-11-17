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
	neighbor_pair nbhd_sol;
	pair<int, int> & nbhd_sol1 = nbhd_sol.first.first;
	pair<int, int> & nbhd_sol2 = nbhd_sol.second.first;
	double & nbhd_obj1 = nbhd_sol.first.second;
	double & nbhd_obj2 = nbhd_sol.second.second;

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
		cout << "1st best solution: " << nbhd_sol1.first << ", " << nbhd_sol2.second << endl;
		cout << "2nd best objective: " << nbhd_obj2 << endl;
		cout << "2nd best solution: " << nbhd_sol2.first << ", " << nbhd_sol2.second << endl;
		cout << "Current solution:" << endl;
		for (int i = 0; i < sol_current.size(); i++)
			cout << sol_current[i] << '\t';
		cout << endl;
		cout << "Showing result of ADDing to 2 and DROPping from 3:" << endl;
		sol_current = move2sol(2, 3);
		for (int i = 0; i < sol_current.size(); i++)
			cout << sol_current[i] << '\t';
		cout << endl;

		// TS/SA updates depending on search results

		if (nbhd_obj1 < obj_current)
		{
			// Improvement iteration

			nonimp_out = 0; // reset outer nonimprovement counter
			tenure = tenure_init; // reset tabu tenures

		}
		else
		{
			// Nonimprovement iteration
		}













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

Returns a structure of nested pairs to represent the best and second best solutions found. The structure is arranged as follows:
	The overall pair's elements <pair, pair> correspond to the best and second best solutions, respectively.
		For each solution we have a pair <pair, double> to represent the move and the objective value, respectively.
			For each move we have a pair <int, int> containing the line IDs of the ADD and DROP lines, respectively.

A move is represented as a pair of integers containing the line IDs of the lines being ADDed to or DROPped from, respectively. If the move does not involve an ADD or a DROP, then one of these elements will be set to "NO_ID" to indicate no change. SWAP moves should always include two legitimate line IDs.
*/
neighbor_pair Search::neighborhood_search()
{





	/////////////////////////////////////////////////////////////////////////////////
	return make_pair(make_pair(make_pair(NO_ID, NO_ID), 0.0), make_pair(make_pair(NO_ID, NO_ID), 0.0));
}

/**
Generates the solution vector resulting from a specified move.

Requires an ADD line ID and a DROP line ID. Use an ID of "NO_ID" to ignore one of the move types.

Returns a vector resulting from applying the specified move to the current solution.
*/
vector<int> Search::move2sol(int add_id, int drop_id)
{
	vector<int> sol = sol_current;

	// ADD move
	if (add_id != NO_ID)
		sol[add_id] += step;

	// DROP move
	if (drop_id != NO_ID)
		sol[drop_id] -= step;

	return sol;
}

/// Writes current memory structures to the output logs.
void Search::save_data()
{
	//////////////////////////////////////
	cout << "\n---------- SAVING DATA (PLACEHOLDER) ----------\n" << endl;
}
