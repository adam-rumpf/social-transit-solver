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

	// Determine total vehicle bounds
	max_vehicles.resize(Net->vehicles.size());
	for (int i = 0; i < Net->vehicles.size(); i++)
		max_vehicles[i] = Net->vehicles[i]->max_fleet;

	// Determine line fleet bounds
	line_min.resize(sol_size);
	line_max.resize(sol_size);
	for (int i = 0; i < sol_size; i++)
	{
		line_min[i] = Net->lines[i]->min_fleet;
		line_max[i] = Net->lines[i]->max_fleet;
	}

	// Determine current total vehicle usage and establish vehicle type vector
	vehicle_type.resize(Net->lines.size());
	for (int i = 0; i < Net->lines.size(); i++)
		vehicle_type[i] = Net->lines[i]->vehicle_id;
	vehicle_totals();

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
		/*for (int i = 0; i < sol_current.size(); i++)
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
		cout << endl;*/

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

			// Recalculate vehicle usage
			vehicle_totals();
		}

		// If outer nonimprovement counter is too high, take actions to intensivy
		if (nonimp_out > nonimp_out_max)
			tenure = tenure_init; // reset tabu tenures

		// Allow tabu tenures to decay
		for (int i = 0; i < sol_size; i++)
		{
			add_tenure[i] = max(add_tenure[i] - 1, 0.0);
			drop_tenure[i] = max(drop_tenure[i] - 1, 0.0);
		}

		// Apply cooling schedule
		cool_temperature();

		// Log the results of the iteration
		EveLog->log_iteration(iteration, obj_current, obj_best);

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

	// Perform final saves after search completes
	save_data();
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
	/*
	The neighborhood search is conducted in two passes in order to minimize the number of constraint function evaluations, since evaluating the constraints is many orders of magnitude more expensive than evaluating the objective.

	In the first pass we generate candidate ADD and DROP moves that satisfy the constant vehicle bound constraints. We do so by randomly selecting lines to ADD to or DROP from, checking whether this would satisfy the vehicle bound constraints, and then collecting the feasible candidates into vectors of candidate moves. We also calculate the objective values of these candidates.

	In the second pass we go through our candidates from the first pass in ascending order of objective value, evaluating the constraint function value for each and collecting the feasible results into a final candidate vector. If, at the end of the second pass, we have fewer than two feasible solutions, we throw away the tabu rules and start the search over again.

	After obtaining enough ADD and DROP move candidates we assemble a list of SWAP move candidates. A SWAP move is made by combining an ADD candidate with a DROP candidate (provided that both candidates involve the same type of vehicle). Because of the potentially large number of possible combinations that could be made, we generate combinations by moving through the ADD and DROP candidate lists in ascending order of objective until obtaining enough feasible SWAP moves.
	*/

	cout << "\n==================== Beginning neighborhood search. ====================" << endl << endl;
	clock_t nbhd_time = clock(); // neighborhood search timer for event log

	// Initialize candidate move containers (all min-priority queues of 3-tupes, respectivaly containing an objective value, ADD/DROP pair, and boolean flag to indicate new log entries)
	move_queue add_moves1; // candidate ADD moves after first pass
	move_queue add_moves2; // candidate ADD moves after second pass
	move_queue drop_moves1; // candidate DROP moves after first pass
	move_queue drop_moves2; // candidate DROP moves after second pass
	move_queue swap_moves; // candidate SWAP moves

	// Initialize candidate ADD/DROP line containers (integer line IDs meant for random sampling without repetition)
	vector<int> add_candidates(sol_size); // initial set of candidate ADD lines
	vector<int> drop_candidates(sol_size); // initial set of candidate DROP lines
	for (int i = 0; i < sol_size; i++)
	{
		add_candidates[i] = i;
		drop_candidates[i] = i;
	}
	random_shuffle(add_candidates.begin(), add_candidates.end());
	random_shuffle(drop_candidates.begin(), drop_candidates.end());

	// Initialize candidate solution temporary containers
	vector<int> sol_candidate(sol_size); // candidate solution vector
	double obj_candidate; // candidate solution objective

	// Initialize counters for the event log
	int obj_lookups = 0; // number of objectives successfully looked up from the solution log
	int con_lookups = 0; // number of constraints successfully looked up from the solution log
	int new_obj = 0; // number of new solution log objective entries
	int new_con = 0; // number of new solution log constraint entries

	// ADD/DROP move selection loop

	cout << "\n-------------------- Generating ADD/DROP candidates. --------------------" << endl;
	// Search until finding at least two feasible neighbors (in general we will find many more)
	while (add_moves2.size() + drop_moves2.size() < 2)
	{
		// ADD move first pass

		// Repeat until reaching our first-pass bound or running out of candidates
		while ((add_moves1.size() < nbhd_add_lim1) && (add_candidates.size() > 0))
		{
			// Pop a random ADD move from the candidate list
			int choice = add_candidates.back();
			add_candidates.pop_back();
			cout << "\nConsidering move ADD " << choice << endl;

			// Filter out moves that would violate a line fleet bound
			if (sol_current[choice] + step > line_max[choice])
			{
				// Skip ADD moves that would exceed a line's vehicle bound
				cout << "Skipping: That would add too many vehicles to line " << choice << endl;
				continue;
			}
			if (current_vehicles[vehicle_type[choice]] + 1 > max_vehicles[vehicle_type[choice]])
			{
				// Skip ADD moves that would exceed a total vehicle bound
				cout << "Skipping: We don't have enough of that vehicle type." << endl;
				continue;
			}

			// Find objective and logged information for candidate solution
			sol_candidate = make_move(choice, NO_ID); // solution vector resulting from chosen ADD
			bool new_candidate; // whether the candidate is new to the solution log
			if (SolLog->solution_exists(sol_candidate) == true)
			{
				// If the solution is logged already, look up its feasibility status and objective
				cout << "We've seen this!" << endl;
				new_candidate = false;
				obj_lookups++;
				pair<int, double> info = SolLog->lookup_row_quick(sol_candidate);
				if (info.first == FEAS_FALSE)
				{
					// Skip solutions known to be infeasible
					cout << "Skipping: We already know that this solution is infeasible." << endl;
					con_lookups++;
					continue;
				}
				obj_candidate = info.second;
			}
			else
			{
				// If the solution is new, calculate its objective and create a tentative log entry
				cout << "This is new!" << endl;
				new_candidate = true;
				new_obj++;
				clock_t start = clock(); // objective calculation timer
				obj_candidate = Obj->calculate(sol_candidate); // calculate objective value
				double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // objective calculation time
				SolLog->create_partial_row(sol_candidate, obj_candidate, candidate_time); // create an initial solution log entry for the candidate solution
				cout << "Logging new solution with objective " << obj_candidate << " calculated in " << candidate_time << " seconds." << endl;
			}

			// Skip a tabu move, unless it would improve our best known solution
			if (add_tenure[choice] > 0)
			{
				if (obj_candidate >= obj_best)
				{
					cout << "Skipping tabu move." << endl;
					continue;
				}
				else
					cout << "Keeping tabu move due to improved best aspiration!" << endl;
			}

			// Add candidate move to the first-pass queue
			cout << "Adding move as a candidate." << endl;
			add_moves1.push(make_tuple(obj_candidate, make_pair(choice, NO_ID), new_candidate));
		}

		// DROP move first pass

		// Repeat until reaching our first-pass bound or running out of candidates
		while ((drop_moves1.size() < nbhd_drop_lim1) && (drop_candidates.size() > 0))
		{
			// Pop a random DROP move from the candidate list
			int choice = drop_candidates.back();
			drop_candidates.pop_back();
			cout << "\nConsidering move DROP " << choice << endl;

			// Filter out moves that would violate a line fleet bound
			if (sol_current[choice] - step < line_min[choice])
			{
				// Skip DROP moves that would fall below a line's vehicle bound
				cout << "Skipping: That would give too few vehicles to line " << choice << endl;
				continue;
			}
			if (current_vehicles[vehicle_type[choice]] - 1 < 0)
			{
				// Skip DROP moves that would result in negative vehicles
				cout << "Skipping: We're out of that type of vehicle." << endl;
				continue;
			}

			// Find objective and logged information for candidate solution
			sol_candidate = make_move(NO_ID, choice); // solution vector resulting from chosen DROP
			bool new_candidate; // whether the candidate is new to the solution log
			if (SolLog->solution_exists(sol_candidate) == true)
			{
				// If the solution is logged already, look up its feasibility status and objective
				cout << "We've seen this!" << endl;
				new_candidate = false;
				obj_lookups++;
				pair<int, double> info = SolLog->lookup_row_quick(sol_candidate);
				if (info.first == FEAS_FALSE)
				{
					// Skip solutions known to be infeasible
					cout << "Skipping: We already know that this solution is infeasible." << endl;
					con_lookups++;
					continue;
				}
				obj_candidate = info.second;
			}
			else
			{
				// If the solution is new, calculate its objective and create a tentative log entry
				cout << "This is new!" << endl;
				new_candidate = true;
				new_obj++;
				clock_t start = clock(); // objective calculation timer
				obj_candidate = Obj->calculate(sol_candidate); // calculate objective value
				double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // objective calculation time
				SolLog->create_partial_row(sol_candidate, obj_candidate, candidate_time); // create an initial solution log entry for the candidate solution
				cout << "Logging new solution with objective " << obj_candidate << " calculated in " << candidate_time << " seconds." << endl;
			}

			// Skip a tabu move, unless it would improve our best known solution
			if (drop_tenure[choice] > 0)
			{
				if (obj_candidate >= obj_best)
				{
					cout << "Skipping tabu move." << endl;
					continue;
				}
				else
					cout << "Keeping tabu move due to improved best aspiration!" << endl;
			}

			// Add candidate move to the first-pass queue
			cout << "Adding move as a candidate." << endl;
			drop_moves1.push(make_tuple(obj_candidate, make_pair(NO_ID, choice), new_candidate));
		}

		// ADD move second pass

		// Repeat until reaching our second-pass bound or running out of first-pass candidates
		while ((add_moves1.size() > 0) && (add_moves2.size() < nbhd_add_lim2))
		{
			// Pop the best candidate out of the first-pass move list
			tuple<double, pair<int, int>, bool> move_triple = add_moves1.top();
			add_moves1.pop();
			cout << "\nSecond pass for ADD " << get<1>(move_triple).first << endl;

			// If the solution is new, calculate and log its constraint function values
			if (get<2>(move_triple) == true)
			{
				cout << "Calculating constraints." << endl;
				new_con++;
				sol_candidate = make_move(get<1>(move_triple).first, NO_ID); // solution vector resulting from chosen ADD
				clock_t start = clock(); // constraint calculation timer
				pair<int, vector<double>> con_candidate = Con->calculate(sol_candidate); // calculate feasibility status and constraint vector
				double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // constraint calculation time
				SolLog->update_row(sol_candidate, con_candidate.first, con_candidate.second, candidate_time); // fill in solution log with missing constraint information
				cout << "Updating constraints for solution with feasibility status " << con_candidate.first << " calculated in " << candidate_time << " seconds." << endl;
				if (con_candidate.first == FEAS_FALSE)
					// Skip candidate if we've discovered that it is infeasible
					continue;
			}

			// Add the feasible candidate to the second-pass move queue
			cout << "Adding move as a final candidate." << endl;
			add_moves2.push(move_triple);
		}

		// DROP move second pass

		// Repeat until reaching our second-pass bound or running out of first-pass candidates
		while ((drop_moves1.size() > 0) && (drop_moves2.size() < nbhd_drop_lim2))
		{
			// Pop the best candidate out of the first-pass move list
			tuple<double, pair<int, int>, bool> move_triple = drop_moves1.top();
			drop_moves1.pop();
			cout << "\nSecond pass for DROP " << get<1>(move_triple).second << endl;

			// If the solution is new, calculate and log its constraint function values
			if (get<2>(move_triple) == true)
			{
				cout << "Calculating constraints." << endl;
				new_con++;
				sol_candidate = make_move(NO_ID, get<1>(move_triple).second); // solution vector resulting from chosen DROP
				clock_t start = clock(); // constraint calculation timer
				pair<int, vector<double>> con_candidate = Con->calculate(sol_candidate); // calculate feasibility status and constraint vector
				double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // constraint calculation time
				SolLog->update_row(sol_candidate, con_candidate.first, con_candidate.second, candidate_time); // fill in solution log with missing constraint information
				cout << "Updating constraints for solution with feasibility status " << con_candidate.first << " calculated in " << candidate_time << " seconds." << endl;
				if (con_candidate.first == FEAS_FALSE)
					// Skip candidate if we've discovered that it is infeasible
					continue;
			}

			// Add the feasible candidate to the second-pass move queue
			cout << "Adding move as a final candidate." << endl;
			drop_moves2.push(move_triple);
		}

		// Unsuccessful search handling
		if (add_moves2.size() + drop_moves2.size() + add_candidates.size() + drop_candidates.size() < 2)
		{
			// If there are no longer enough candidates to generate at least two moves, delete the tabu list
			cout << "Out of options. Throwing away tabu set." << endl;
			for (int i = 0; i < sol_size; i++)
			{
				add_tenure[i] = 0;
				drop_tenure[i] = 0;
			}
		}
	}

	// Clear first-pass move queues
	add_moves1 = move_queue();
	drop_moves1 = move_queue();

	cout << "\n-------------------- Generating SWAP candidates. --------------------" << endl;

	// SWAP move selection

	// Proceed only if we have at least one ADD and one DROP
	if ((add_moves2.size() > 0) && (drop_moves2.size()))
	{
		cout << "Generating SWAPs from " << add_moves2.size() << " ADD moves and " << drop_moves2.size() << " DROP moves." << endl;
	}

	/////////////////////// We need to be able to iterate through the ADD and DROP 2nd-pass lists, so the 2nd-pass lists should not be priority queues but rather something like a list or a vector. We also don't need to include the "new" flag, so the second structure can be simpler. They will already be sorted so we can simply add them as they get popped off of the queue, but we should still use a priority queue for the SWAPs since they will not necessarily be generated in order.






	/////////////////////////////////////////////////////////////////////////////////
	cout << "\nSpent " << (1.0*clock() - nbhd_time) / CLOCKS_PER_SEC << " seconds on the neighborhood search." << endl << endl;
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

/// Calculates total number of each vehicle type in use for the current solution, and updates vehicle total variable.
void Search::vehicle_totals()
{
	current_vehicles = vector<int>(Net->vehicles.size(), 0);
	for (int i = 0; i < Net->lines.size(); i++)
		current_vehicles[vehicle_type[i]] += sol_current[i];
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
