/// Search class methods.

#include "search.hpp"

/// Search constructor initializes Network, Objective, and Constraint objects.
Search::Search()
{
	Net = new Network(); // network object
	Obj = new Objective(Net); // objective function object
	Con = new Constraint(Net); // constraint function object
	sol_size = Net->lines.size(); // get solution vector size
	srand(time(NULL)); // seed random number generator
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

	// Determine vehicle bounds
	max_vehicles.resize(Net->vehicles.size());
	for (int i = 0; i < Net->vehicles.size(); i++)
		max_vehicles[i] = Net->vehicles[i]->max_fleet;

	// Determine current vehicle usage and establish vehicle type vector
	vehicle_type.resize(Net->lines.size());
	current_vehicles = vector<int>(Net->vehicles.size(), 0);
	for (int i = 0; i < Net->lines.size(); i++)
	{
		vehicle_type[i] = Net->lines[i]->vehicle_id;
		current_vehicles[vehicle_type[i]] += sol_current[i];
	}

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
		sol_current = make_move(2, 3);
		for (int i = 0; i < sol_current.size(); i++)
			cout << sol_current[i] << '\t';
		cout << endl;
		cout << "\n\nTesting vehicles." << endl;
		for (int i = 0; i < Net->vehicles.size(); i++)
			cout << "Type " << i << " (current, max, cap): " << current_vehicles[i] << ", " << Net->vehicles[i]->max_fleet << ", " << Net->vehicles[i]->capacity << endl;
		cout << endl;

		// TS/SA updates depending on search results

		if (nbhd_obj1 < obj_current)
		{
			// Improvement iteration: make the move and make it tabu to undo it
			cout << "Improvement iteration." << endl;

			nonimp_out = 0; // reset outer nonimprovement counter
			tenure = tenure_init; // reset tabu tenures
			sol_current = make_move(nbhd_sol1.first, nbhd_sol1.second); // move to best neighbor
			obj_current = nbhd_obj1; // update objective

			// ADD-related updates
			if (nbhd_sol1.first != NO_ID)
			{
				current_vehicles[vehicle_type[nbhd_sol1.first]]++; // increase vehicle usage
				drop_tenure[nbhd_sol1.first] = tenure; // make it tabu to DROP from the new ADD line
			}

			// DROP-related updates
			if (nbhd_sol1.second != NO_ID)
			{
				current_vehicles[vehicle_type[nbhd_sol1.second]]--; // decrease vehicle usage
				add_tenure[nbhd_sol1.second] = tenure; // make it tabu to ADD to the new DROP line
			}

			// Update best known solution if needed
			if (obj_current < obj_best)
			{
				cout << "New best solution found!" << endl;
				sol_best = sol_current;
				obj_best = obj_current;
			}
		}
		else
		{
			// Nonimprovement iteration
			cout << "Nonimprovement iteration." << endl;

			// Increase nonimprovement counters
			nonimp_in++;
			nonimp_out++;

			// Evaluate simulated annealing criterion
			cout << "Probability of passing SA criterion: " << exp(-(nbhd_obj1 - obj_current) / temperature) << endl;
			if (((1.0 * rand()) / RAND_MAX) < exp(-(nbhd_obj1 - obj_current) / temperature))
			{
				// If passed, make the move as in an improvement iteration but with increased tabus, then keep the second best solution as attractive
				cout << "Passed SA criterion." << endl;

				nonimp_in = 0; // reset inner nonimprovement counter
				increase_tenure(); // increase tabu tenures
				sol_current = make_move(nbhd_sol1.first, nbhd_sol1.second); // move to best neighbor
				obj_current = nbhd_obj1; // update objective

				// ADD-related updates
				if (nbhd_sol1.first != NO_ID)
				{
					current_vehicles[vehicle_type[nbhd_sol1.first]]++; // increase vehicle usage
					drop_tenure[nbhd_sol1.first] = tenure; // make it tabu to DROP from the new ADD line
				}

				// DROP-related updates
				if (nbhd_sol1.second != NO_ID)
				{
					current_vehicles[vehicle_type[nbhd_sol1.second]]--; // decrease vehicle usage
					add_tenure[nbhd_sol1.second] = tenure; // make it tabu to ADD to the new DROP line
				}

				// Add the second best solution as attractive
				attractive_solutions.push_back(make_pair(make_move(nbhd_sol2.first, nbhd_sol2.second), nbhd_obj2));
			}
			else
			{
				cout << "Failed SA criterion." << endl;
				// If failed, make no moves but keep the best neighbor as attractive
				attractive_solutions.push_back(make_pair(make_move(nbhd_sol1.first, nbhd_sol1.second), nbhd_obj1));
			}
		}

		// Iteration end updates

		// Trim the attractive solution set if too long
		if (attractive_solutions.size() > attractive_max)
			pop_attractive(false); // in no-replace mode

		// If inner nonimprovement counter is too high, take actions to diversify
		if (nonimp_in > nonimp_in_max)
		{
			nonimp_in = 0; // reset inner counter
			nonimp_out++; // increment outer counter
			increase_tenure(); // increase tabu tenures

			// Move to a random attractive solution
			pop_attractive(true); // replace mode resets current solution

			//////////////
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

	//////////////////////////////
	cout << "Attractive solution size: " << attractive_solutions.size() << endl;





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
vector<int> Search::make_move(int add_id, int drop_id)
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

/**
Deletes a random solution from the attractive solution set, and optionally sets it as the current solution.

Requires a boolean variable to set the mode. If true, then the chosen attractive solution replaces the current solution. If false, then the chosen attractive solution is simply deleted from the list and discarded.
*/
void Search::pop_attractive(bool replace)
{
	// Set an iterator to point to a random attractive solution
	int r = rand() % attractive_solutions.size();
	list<pair<vector<int>, double>>::iterator it = attractive_solutions.begin();
	advance(it, r);

	// If in replace mode, set the current solution to the chosen element
	if (replace == true)
	{
		sol_current = it->first;
		obj_current = it->second;
	}

	// In either case, delete the chosen element
	attractive_solutions.erase(it);
}

/// Increases tabu tenure.
void Search::increase_tenure()
{
	// Here we simply apply a multiplicative factor, but randomized and iteration/counter-dependent alternatives exist.
	tenure *= tenure_factor;
}

/// Decreases simulated annealing temperature.
void Search::cool_temperature()
{
	// Here we simply apply a decay factor, but iteration-dependent alternatives exist.
	temperature *= temp_factor;
}

/// Writes current memory structures to the output logs.
void Search::save_data()
{
	//////////////////////////////////////
	cout << "\n---------- SAVING DATA (PLACEHOLDER) ----------\n" << endl;
}
