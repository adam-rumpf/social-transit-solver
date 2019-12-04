/// Search class methods.

#include "search.hpp"

/// Search constructor initializes Network, Objective, and Constraint objects and loads search parameters.
Search::Search()
{
	Net = new Network(); // network object
	Obj = new Objective(Net); // objective function object
	Con = new Constraint(Net); // constraint function object
	sol_size = Net->lines.size(); // get solution vector size
	srand(time(NULL)); // seed random number generator

	// Load search parameters
	ifstream parameter_file;
	parameter_file.open(FILE_BASE + SEARCH_FILE);
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
			case 16:
				exhaustive = stoi(piece);
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
}

/// Search destructor deletes Network, Objective, and Constraint objects created by the constructor.
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

/// Main driver of the solution algorithm. Calls main search loop and handles final output.
void Search::solve()
{
	started = true;

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
		clock_t start = clock(); // iteration timer

		iteration++;
		cout << "\n============================================================" << endl;
		cout << "Iteration " << iteration << " / " << max_iterations << endl;
		cout << "============================================================" << endl << endl;
		EveLog->iteration = iteration;

		// Perform neighborhood search
		nbhd_sol = neighborhood_search();

		// Break if no moves are available
		if (nbhd_obj1 == INFINITY)
		{
			EveLog->halt(FAILURE_HALT_SYMBOL);
			save_data();
			exit(FAILURE_HALT);
		}

		// TS/SA updates depending on search results

		if (nbhd_obj1 < obj_current)
		{
			// Improvement iteration: make the move and make it tabu to undo it
			EveLog->event_case = EVENT_IMPROVEMENT;

			nonimp_out = 0; // reset outer nonimprovement counter
			tenure = tenure_init; // reset tabu tenures
			sol_current = make_move(nbhd_sol1.first, nbhd_sol1.second); // move to best neighbor
			obj_current = nbhd_obj1; // update objective

			// ADD-related updates
			if (nbhd_sol1.first != NO_ID)
			{
				current_vehicles[vehicle_type[nbhd_sol1.first]]++; // increase vehicle usage
				drop_tenure[nbhd_sol1.first] = tenure; // make it tabu to DROP from the new ADD line
				EveLog->add_id = nbhd_sol1.first;
			}

			// DROP-related updates
			if (nbhd_sol1.second != NO_ID)
			{
				current_vehicles[vehicle_type[nbhd_sol1.second]]--; // decrease vehicle usage
				add_tenure[nbhd_sol1.second] = tenure; // make it tabu to ADD to the new DROP line
				EveLog->drop_id = nbhd_sol1.second;
			}

			// Update best known solution if needed
			if (obj_current < obj_best)
			{
				EveLog->new_best = 1;
				sol_best = sol_current;
				obj_best = obj_current;
			}
		}
		else
		{
			// Nonimprovement iteration

			// Increase nonimprovement counters
			nonimp_in++;
			nonimp_out++;

			// Evaluate simulated annealing criterion
			double prob = exp(-(nbhd_obj1 - obj_current) / temperature);
			EveLog->sa_prob = prob;
			if (((1.0 * rand()) / RAND_MAX) < prob)
			{
				// If passed, make the move as in an improvement iteration but with increased tabus, then keep the second best solution as attractive
				EveLog->event_case = EVENT_NONIMP_PASS;

				nonimp_in = 0; // reset inner nonimprovement counter
				increase_tenure(); // increase tabu tenures
				sol_current = make_move(nbhd_sol1.first, nbhd_sol1.second); // move to best neighbor
				obj_current = nbhd_obj1; // update objective

				// ADD-related updates
				if (nbhd_sol1.first != NO_ID)
				{
					current_vehicles[vehicle_type[nbhd_sol1.first]]++; // increase vehicle usage
					drop_tenure[nbhd_sol1.first] = tenure; // make it tabu to DROP from the new ADD line
					EveLog->add_id = nbhd_sol1.first;
				}

				// DROP-related updates
				if (nbhd_sol1.second != NO_ID)
				{
					current_vehicles[vehicle_type[nbhd_sol1.second]]--; // decrease vehicle usage
					add_tenure[nbhd_sol1.second] = tenure; // make it tabu to ADD to the new DROP line
					EveLog->drop_id = nbhd_sol1.second;
				}

				// Add the second best solution as attractive (unless there was only one feasible neigbhor)
				if (nbhd_obj2 < INFINITY)
					attractive_solutions.push_back(make_pair(make_move(nbhd_sol2.first, nbhd_sol2.second), nbhd_obj2));
			}
			else
			{
				// If failed, make no moves but keep the best neighbor as attractive
				EveLog->event_case = EVENT_NONIMP_FAIL;
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
			EveLog->jump = 1;

			// Recalculate vehicle usage
			vehicle_totals();
		}

		// If outer nonimprovement counter is too high, take actions to intensify
		if (nonimp_out > nonimp_out_max)
			tenure = tenure_init; // reset tabu tenures

		// Allow tabu tenures to decay
		for (int i = 0; i < sol_size; i++)
		{
			add_tenure[i] = max(add_tenure[i] - 1.0, 0.0);
			drop_tenure[i] = max(drop_tenure[i] - 1.0, 0.0);
		}

		// Apply cooling schedule
		cool_temperature();

		// Log the results of the iteration
		EveLog->obj_current = obj_current;
		EveLog->obj_best = obj_best;
		EveLog->nonimp_in = nonimp_in;
		EveLog->nonimp_out = nonimp_out;
		EveLog->tenure = tenure;
		EveLog->temperature = temperature;
		EveLog->total_time = (1.0*clock() - start) / CLOCKS_PER_SEC;
		EveLog->log_iteration(sol_current);

		// Save data
		cout << "\n" << (1.0*clock() - start) / CLOCKS_PER_SEC << " seconds spent on iteration." << endl;
		save_data();

		// Safely quit if a keyboard halt has been requested
		if (keyboard_halt == true)
		{
			EveLog->halt();
			exit(KEYBOARD_HALT);
		}
	}

	// Handle exhaustive search (if selected)
	if (exhaustive == 1)
	{
		exhaustive_iteration = 0;

		cout << "\n============================================================" << endl;
		cout << "Final exhaustive search" << endl;
		cout << "============================================================" << endl << endl;

		// Set current solution to best
		sol_current = sol_best;
		obj_current = obj_best;
		vehicle_totals();

		// Initialize local search method
		exhaustive_search();
	}

	// Perform final saves after search completes
	save_data();
	cout << "Generating metrics for best known solution." << endl;
	Obj->save_metrics(sol_best);
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

	In the first pass we generate non-tabu candidate ADD and DROP moves that satisfy the constant vehicle bound constraints. We do so by randomly selecting lines to ADD to or DROP from, checking whether this would satisfy the vehicle bound constraints, and then collecting the feasible candidates into vectors of candidate moves. We also calculate the objective values of these candidates.

	In the second pass we go through our candidates from the first pass in ascending order of objective value, evaluating the constraint function value for each and collecting the feasible results into a final candidate vector. If, at the end of the second pass, we have fewer than two feasible solutions, we throw away the tabu rules and start the search over again.

	After obtaining enough ADD and DROP move candidates we assemble a list of SWAP move candidates. A SWAP move is made by combining an ADD candidate with a DROP candidate (provided that both candidates involve the same type of vehicle). Because of the potentially large number of possible combinations that could be made, we generate combinations by moving through the ADD and DROP candidate lists in ascending order of objective until obtaining enough feasible SWAP moves.
	*/

	/*
	Initialize candidate move containers.

	During the first pass, ADD and DROP moves are stored in a min-priority queue of 3-tuples which contain the objective value, ADD/DROP line pair, and a boolean indicating whether the solution has been logged already. Non-tabu solutions which satisfy the vehicle bound constraints are pushed into the queue, and can later be processed in ascending order of objective.

	During the second pass, ADD and DROP moves are stored in a list of pairs which contain the objective value and the ADD/DROP line pair. Because the second pass always considers solutions in ascending order of objective, the lists will automatically be sorted in ascending order of objective.

	The final moves are stored in a min-priority queue of pairs which contain the objective value and ADD/DROP line pair. This includes (a second copy of) ADD/DROP moves chosen during the second pass and chosen SWAP moves. The first two elements of the queue can be popped to obtain the best and second best neighbors.
	*/
	candidate_queue add_moves1; // candidate ADD moves after first pass
	candidate_queue drop_moves1; // candidate DROP moves after first pass
	list<pair<double, pair<int, int>>> add_moves2; // candidate ADD moves after second pass
	list<pair<double, pair<int, int>>> drop_moves2; // candidate DROP moves after second pass
	neighbor_queue final_moves; // includes all ADD, DROP, and SWAP moves for final consideration

	// Initialize candidate ADD/DROP line containers (integer line IDs meant for random sampling without repetition)
	vector<int> add_candidates(sol_size); // initial set of candidate ADD lines
	vector<int> drop_candidates(sol_size); // initial set of candidate DROP lines
	unordered_set<int> add_chosen; // set of chosen ADD lines
	unordered_set<int> drop_chosen; // set of chosen DROP lines
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

	// Search until finding at least two feasible neighbors (in general we will find many more)
	while (add_moves2.size() + drop_moves2.size() < 2)
	{
		cout << '|';

		// ADD move first pass

		// Repeat until reaching our first-pass bound or running out of candidates
		while ((add_moves1.size() < nbhd_add_lim1) && (add_candidates.size() > 0))
		{
			// Pop a random ADD move from the candidate list
			int choice = add_candidates.back();
			add_candidates.pop_back();

			// Skip ADD moves that have already been selected (may occur if the ADD/DROP selection loop repeats)
			if (add_chosen.count(choice) > 0)
				continue;

			// Filter out moves that would violate a line fleet bound
			if (sol_current[choice] + step > line_max[choice])
				// Skip ADD moves that would exceed a line's vehicle bound
				continue;
			if (current_vehicles[vehicle_type[choice]] + 1 > max_vehicles[vehicle_type[choice]])
				// Skip ADD moves that would exceed a total vehicle bound
				continue;

			// Find objective and logged information for candidate solution
			cout << 'a';
			sol_candidate = make_move(choice, NO_ID); // solution vector resulting from chosen ADD
			bool new_candidate; // whether the candidate is new to the solution log
			if (SolLog->solution_exists(sol_candidate) == true)
			{
				// If the solution is logged already, look up its feasibility status and objective
				new_candidate = false;
				obj_lookups++;
				pair<int, double> info = SolLog->lookup_row_quick(sol_candidate);
				if ((info.first == FEAS_FALSE) || (info.first == FEAS_BAN))
				{
					// Skip solutions known to be infeasible and banned solutions
					con_lookups++;
					continue;
				}
				obj_candidate = info.second;
			}
			else
			{
				// If the solution is new, calculate its objective and create a tentative log entry
				new_candidate = true;
				new_obj++;
				clock_t start = clock(); // objective calculation timer
				obj_candidate = Obj->calculate(sol_candidate); // calculate objective value
				double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // objective calculation time
				SolLog->create_partial_row(sol_candidate, obj_candidate, candidate_time); // create an initial solution log entry for the candidate solution
			}

			// Skip a tabu move, unless it would improve our best known solution
			if (add_tenure[choice] > 0)
			{
				if (obj_candidate >= obj_best)
					continue;
			}

			// Add candidate move to the first-pass queue and add to list of chosen lines
			add_moves1.push(make_tuple(obj_candidate, make_pair(choice, NO_ID), new_candidate));
			add_chosen.insert(choice);
		}
		EveLog->add_first += add_moves1.size();

		// DROP move first pass

		// Repeat until reaching our first-pass bound or running out of candidates
		while ((drop_moves1.size() < nbhd_drop_lim1) && (drop_candidates.size() > 0))
		{
			// Pop a random DROP move from the candidate list
			int choice = drop_candidates.back();
			drop_candidates.pop_back();

			// Skip DROP moves that have already been selected (may occur if the ADD/DROP selection loop repeats)
			if (drop_chosen.count(choice) > 0)
				continue;

			// Filter out moves that would violate a line fleet bound
			if (sol_current[choice] - step < line_min[choice])
				// Skip DROP moves that would fall below a line's vehicle bound
				continue;
			if (current_vehicles[vehicle_type[choice]] - 1 < 0)
				// Skip DROP moves that would result in negative vehicles
				continue;

			// Find objective and logged information for candidate solution
			cout << 'd';
			sol_candidate = make_move(NO_ID, choice); // solution vector resulting from chosen DROP
			bool new_candidate; // whether the candidate is new to the solution log
			if (SolLog->solution_exists(sol_candidate) == true)
			{
				// If the solution is logged already, look up its feasibility status and objective
				new_candidate = false;
				obj_lookups++;
				pair<int, double> info = SolLog->lookup_row_quick(sol_candidate);
				if ((info.first == FEAS_FALSE) || (info.first == FEAS_BAN))
				{
					// Skip solutions known to be infeasible and banned solutions
					con_lookups++;
					continue;
				}
				obj_candidate = info.second;
			}
			else
			{
				// If the solution is new, calculate its objective and create a tentative log entry
				new_candidate = true;
				new_obj++;
				clock_t start = clock(); // objective calculation timer
				obj_candidate = Obj->calculate(sol_candidate); // calculate objective value
				double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // objective calculation time
				SolLog->create_partial_row(sol_candidate, obj_candidate, candidate_time); // create an initial solution log entry for the candidate solution
			}

			// Skip a tabu move, unless it would improve our best known solution
			if (drop_tenure[choice] > 0)
			{
				if (obj_candidate >= obj_best)
					continue;
			}

			// Add candidate move to the first-pass queue and add to list of chosen lines
			drop_moves1.push(make_tuple(obj_candidate, make_pair(NO_ID, choice), new_candidate));
			drop_chosen.insert(choice);
		}
		EveLog->drop_first += drop_moves1.size();

		// ADD move second pass

		// Repeat until reaching our second-pass bound or running out of first-pass candidates
		while ((add_moves1.size() > 0) && (add_moves2.size() < nbhd_add_lim2))
		{
			// Pop the best candidate out of the first-pass move list
			tuple<double, pair<int, int>, bool> move_triple = add_moves1.top();
			add_moves1.pop();

			// If the solution is new, calculate and log its constraint function values
			if (get<2>(move_triple) == true)
			{
				new_con++;
				sol_candidate = make_move(get<1>(move_triple).first, NO_ID); // solution vector resulting from chosen ADD
				clock_t start = clock(); // constraint calculation timer
				pair<int, vector<double>> con_candidate = Con->calculate(sol_candidate); // calculate feasibility status and constraint vector
				double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // constraint calculation time
				SolLog->update_row(sol_candidate, con_candidate.first, con_candidate.second, candidate_time); // fill in solution log with missing constraint information
				if (con_candidate.first == FEAS_FALSE)
					// Skip candidate if we've discovered that it is infeasible
					continue;
			}
			else
				con_lookups++;

			// Add the feasible candidate to the second-pass move list and the final move queue
			add_moves2.push_back(make_pair(get<0>(move_triple), get<1>(move_triple)));
			final_moves.push(make_pair(get<0>(move_triple), get<1>(move_triple)));
		}
		EveLog->add_second += add_moves2.size();

		// DROP move second pass

		// Repeat until reaching our second-pass bound or running out of first-pass candidates
		while ((drop_moves1.size() > 0) && (drop_moves2.size() < nbhd_drop_lim2))
		{
			// Pop the best candidate out of the first-pass move list
			tuple<double, pair<int, int>, bool> move_triple = drop_moves1.top();
			drop_moves1.pop();

			// If the solution is new, calculate and log its constraint function values
			if (get<2>(move_triple) == true)
			{
				new_con++;
				sol_candidate = make_move(NO_ID, get<1>(move_triple).second); // solution vector resulting from chosen DROP
				clock_t start = clock(); // constraint calculation timer
				pair<int, vector<double>> con_candidate = Con->calculate(sol_candidate); // calculate feasibility status and constraint vector
				double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // constraint calculation time
				SolLog->update_row(sol_candidate, con_candidate.first, con_candidate.second, candidate_time); // fill in solution log with missing constraint information
				if (con_candidate.first == FEAS_FALSE)
					// Skip candidate if we've discovered that it is infeasible
					continue;
			}
			else
				con_lookups++;

			// Add the feasible candidate to the second-pass move list and the final move queue
			drop_moves2.push_back(make_pair(get<0>(move_triple), get<1>(move_triple)));
			final_moves.push(make_pair(get<0>(move_triple), get<1>(move_triple)));
		}
		EveLog->drop_second += drop_moves2.size();

		// Unsuccessful search handling
		if (add_moves2.size() + drop_moves2.size() + add_candidates.size() + drop_candidates.size() < 2)
		{
			// If there are no longer enough candidates to generate at least two moves, take actions to intensify
			
			// Reset tabu tenures
			tenure = tenure_init;

			// Test to see whether at least one move is still tabu
			bool tabu_exists = false;
			for (int i = 0; i < sol_size; i++)
			{
				if ((add_tenure[i] > 0) || (drop_tenure[i] > 0))
				{
					tabu_exists = true;
					break;
				}
			}

			// If no moves are tabu and we are still in the failure case, we can only have one feasible neighbor left and we end the ADD/DROP search and ban the solution
			if (tabu_exists == false)
				break;
			
			// Decrement the tabu list and reset the candidate lists to prepare to go through this loop a second time
			add_candidates.resize(sol_size);
			drop_candidates.resize(sol_size);
			for (int i = 0; i < sol_size; i++)
			{
				add_tenure[i] = max(add_tenure[i] - 1.0, 0.0);
				drop_tenure[i] = max(drop_tenure[i] - 1.0, 0.0);
				add_candidates[i] = i;
				drop_candidates[i] = i;
			}
		}
	}

	// Clear first-pass containers
	add_moves1 = candidate_queue();
	drop_moves1 = candidate_queue();
	add_candidates.clear();
	drop_candidates.clear();
	add_chosen.clear();
	drop_chosen.clear();

	// SWAP move selection

	int swaps = 0; // number of swap moves accepted

	// Proceed only if we have at least one ADD and one DROP
	if ((add_moves2.size() > 0) && (drop_moves2.size()))
	{
		/*
		SWAP moves are generated by combining pairs of ADD and DROP moves from the candidate lists. Under the assumption that the best ADD and DROP moves will combine to give the best SWAP moves, we consider ADD and DROP moves in ascending order of objective value. Due to the order in which the lists were constructed, they are already both sorted by objective.

		Specifically, we consider pairs of candidates in a triangular search pattern by iterating through the ADD list, and for each ADD move by iterating through all of the DROP moves located at or ahead of the corresponding ADD move in their own list. The search ends as soon as we find enough swap candidates, or we run out of pairs to search.
		*/
		int limit = min(add_moves2.size(), drop_moves2.size()); // smaller of the two candidate list sizes
		int add_loop = 0; // iteration of outer ADD list loop
		int drop_loop; // iteration of inner DROP list loop

		// Iterate through the ADD list (breaks if we reach a stopping condition)
		for (list<pair<double, pair<int, int>>>::iterator add_it = add_moves2.begin(); add_it != add_moves2.end(); add_it++)
		{
			// Break if we have enough candidate SWAPs, or if the outer loop iteration counter reaches its limit
			if ((swaps >= nbhd_swap_lim) || (add_loop > limit))
				break;

			drop_loop = 0;

			// Iterate through the DROP list (breaks if we reach a stopping condition, or upon iterating to the same position as the current ADD iterator)
			for (list<pair<double, pair<int, int>>>::iterator drop_it = drop_moves2.begin(); drop_it != drop_moves2.end(); drop_it++)
			{
				// Break if inner loop's iteration number exceeds the outer loop's iteration number
				if (drop_loop > add_loop)
					break;

				drop_loop++;
				cout << 's';

				// Get ADD and DROP line IDs
				int add_id = add_it->second.first;
				int drop_id = drop_it->second.second;

				// Filter logically incompatible SWAP moves
				if (add_id == drop_id)
					// Skip pairs that would ADD or DROP on the same line
					continue;
				if (vehicle_type[add_id] != vehicle_type[drop_id])
					// Skip line pairs with different vehicle types
					continue;

				// Generate candidate solution and check the solution log
				sol_candidate = make_move(add_id, drop_id);
				if (SolLog->solution_exists(sol_candidate) == true)
				{
					// If the solution is logged already, look up its feasibility status and objective
					obj_lookups++;
					pair<int, double> info = SolLog->lookup_row_quick(sol_candidate);
					obj_candidate = info.second;

					// Decide what to do based on logged feasibility status
					if (info.first == FEAS_FALSE)
					{
						// Skip solutions known to be infeasible
						con_lookups++;
						continue;
					}
					else if (info.first == FEAS_TRUE)
					{
						// Immediately add solutions known to be feasible
						con_lookups++;
						swaps++;
						final_moves.push(make_pair(obj_candidate, make_pair(add_id, drop_id)));
						continue;
					}
					else if (info.first == FEAS_UNKNOWN)
					{
						// Calculate constraints for solutions of unknown feasibility
						new_con++;
						clock_t start = clock(); // constraint calculation timer
						pair<int, vector<double>> con_candidate = Con->calculate(sol_candidate); // calculate feasibility status and constraint vector
						double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // constraint calculation time
						SolLog->update_row(sol_candidate, con_candidate.first, con_candidate.second, candidate_time); // fill in solution log with missing constraint information
					}
				}
				else
				{
					// If the solution is new, generate all necessary information for the log entry

					// Calculate objective
					new_obj++;
					clock_t start = clock(); // objective calculation timer
					obj_candidate = Obj->calculate(sol_candidate); // calculate objective value
					double obj_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // objective calculation time

					// Calculate constraints
					new_con++;
					start = clock(); // constraint calculation timer
					pair<int, vector<double>> con_candidate = Con->calculate(sol_candidate); // calculate feasibility status and constraint vector
					double con_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // constraint calculation time

					// Create new solution log entry
					SolLog->create_row(sol_candidate, con_candidate.first, con_candidate.second, con_time, obj_candidate, obj_time);

					// If the solution is feasible, add it to the candidate list
					if (con_candidate.first == FEAS_TRUE)
					{
						swaps++;
						final_moves.push(make_pair(obj_candidate, make_pair(add_id, drop_id)));
					}
				}

				// Break if have enough SWAP candidates
				if (swaps >= nbhd_swap_lim)
					break;
			}

			add_loop++;
		}
	}
	EveLog->swaps = swaps;

	// Clear second-pass move lists
	add_moves2.clear();
	drop_moves2.clear();

	// Return the two best solutions from the final move queue (filling missing solutions with placeholders)
	pair<pair<int, int>, double> neighbor1;
	pair<pair<int, int>, double> neighbor2;

	if (final_moves.empty() == false)
	{
		// If there is a best neighbor, record it
		neighbor1 = make_pair(final_moves.top().second, final_moves.top().first);
		final_moves.pop();

		if (final_moves.empty() == false)
			// If there is a second best neighbor, record it
			neighbor2 = make_pair(final_moves.top().second, final_moves.top().first);
		else
		{
			// If there was only one feasible neighbor, output a null second solution ban the current solution so that we never return to it
			neighbor2 = make_pair(make_pair(NO_ID, NO_ID), INFINITY);
			SolLog->ban_solution(sol_current); // ban current solution
		}
	}
	else
	{
		// If no neighbors were feasible, return null moves
		neighbor1 = make_pair(make_pair(NO_ID, NO_ID), INFINITY);
		neighbor2 = make_pair(make_pair(NO_ID, NO_ID), INFINITY);
	}

	EveLog->obj_lookups = obj_lookups;
	EveLog->con_lookups = con_lookups;
	EveLog->obj_evals = new_obj;
	EveLog->con_evals = new_con;

	cout << endl;

	return make_pair(neighbor1, neighbor2);
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
	MemLog->save_memory();
	SolLog->save_solution();
}

/**
Finds the absolute best neighbor of the current solution.

Returns a move/objective value pair corresponding to the best neighbor. If no neighbor has an objective value strictly lower than the given solution (meaning that the given solution is locally optimal), the returned solution will consist of the NO_ID move pair and an infinite objective.

This is for use in an exhaustive local search. Every possible ADD and DROP move from the given solution is considered (we do not consider SWAP moves since there are so many). Tabu rules are ignored but all other constraints are enforced.
*/
pair<pair<int, int>, double> Search::best_neighbor()
{
	// Current best known neighbor objective and move
	pair<int, int> top_move = make_pair(NO_ID, NO_ID);
	double top_objective = INFINITY;

	// Consider every possible ADD move
	for (int choice = 0; choice < sol_size; choice++)
	{
		// Filter out moves that would violate a line fleet bound
		if (sol_current[choice] + step > line_max[choice])
			// Skip ADD moves that would exceed a line's vehicle bound
			continue;
		if (current_vehicles[vehicle_type[choice]] + 1 > max_vehicles[vehicle_type[choice]])
			// Skip ADD moves that would exceed a total vehicle bound
			continue;

		// Initialize candidate solution containers
		vector<int> sol_candidate = make_move(choice, NO_ID); // solution vector resulting from chosen ADD
		double obj_candidate; // objective of candidate solution
		int feas; // candidate solution feasibility status

		// Look up logged information for candidate solution
		if (SolLog->solution_exists(sol_candidate) == true)
		{
			// If the solution is logged already, look up its feasibility status and objective
			pair<int, double> info = SolLog->lookup_row_quick(sol_candidate);
			feas = info.first;
			if (feas == FEAS_FALSE)
				// Skip solutions known to be infeasible
				continue;
			obj_candidate = info.second;
		}
		else
		{
			// If the solution is new, calculate its objective and create a tentative log entry
			feas = FEAS_UNKNOWN;
			clock_t start = clock(); // objective calculation timer
			obj_candidate = Obj->calculate(sol_candidate); // calculate objective value
			double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // objective calculation time
			SolLog->create_partial_row(sol_candidate, obj_candidate, candidate_time); // create an initial solution log entry for the candidate solution
		}

		// Filter out moves that do not improve on the current solution or best known neighbor
		if ((obj_candidate >= obj_current) || (obj_candidate >= top_objective))
			continue;

		// Evaluate the feasibility of the candidate solution
		if (feas == FEAS_UNKNOWN)
		{
			// If feasibility is unknown, calculate its constraints and create a full log entry
			clock_t start = clock(); // constraint calculation timer
			pair<int, vector<double>> con_candidate = Con->calculate(sol_candidate); // calculate feasibility status and constraint vector
			double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // constraint calculation time
			SolLog->update_row(sol_candidate, con_candidate.first, con_candidate.second, candidate_time); // fill in solution log with missing constraint information
			if (con_candidate.first == FEAS_FALSE)
				// Skip candidate if discovered to be infeasible
				continue;
		}

		// If we've made it this far, the candidate should be kept
		top_move = make_pair(choice, NO_ID);
		top_objective = obj_candidate;
	}
	cout << '.';

	// Consider every possible DROP move
	for (int choice = 0; choice < sol_size; choice++)
	{
		// Filter out moves that would violate a line fleet bound
		if (sol_current[choice] - step < line_min[choice])
			// Skip DROP moves that would fall below a line's vehicle bound
			continue;
		if (current_vehicles[vehicle_type[choice]] - 1 < 0)
			// Skip DROP moves that would result in negative vehicles
			continue;

		// Initialize candidate solution containers
		vector<int> sol_candidate = make_move(NO_ID, choice); // solution vector resulting from chosen DROP
		double obj_candidate; // objective of candidate solution
		int feas; // candidate solution feasibility status

		// Look up logged information for candidate solution
		if (SolLog->solution_exists(sol_candidate) == true)
		{
			// If the solution is logged already, look up its feasibility status and objective
			pair<int, double> info = SolLog->lookup_row_quick(sol_candidate);
			feas = info.first;
			if (feas == FEAS_FALSE)
				// Skip solutions known to be infeasible
				continue;
			obj_candidate = info.second;
		}
		else
		{
			// If the solution is new, calculate its objective and create a tentative log entry
			feas = FEAS_UNKNOWN;
			clock_t start = clock(); // objective calculation timer
			obj_candidate = Obj->calculate(sol_candidate); // calculate objective value
			double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // objective calculation time
			SolLog->create_partial_row(sol_candidate, obj_candidate, candidate_time); // create an initial solution log entry for the candidate solution
		}

		// Filter out moves that do not improve on the current solution or best known neighbor
		if ((obj_candidate >= obj_current) || (obj_candidate >= top_objective))
			continue;

		// Evaluate the feasibility of the candidate solution
		if (feas == FEAS_UNKNOWN)
		{
			// If feasibility is unknown, calculate its constraints and create a full log entry
			clock_t start = clock(); // constraint calculation timer
			pair<int, vector<double>> con_candidate = Con->calculate(sol_candidate); // calculate feasibility status and constraint vector
			double candidate_time = (1.0*clock() - start) / CLOCKS_PER_SEC; // constraint calculation time
			SolLog->update_row(sol_candidate, con_candidate.first, con_candidate.second, candidate_time); // fill in solution log with missing constraint information
			if (con_candidate.first == FEAS_FALSE)
				// Skip candidate if discovered to be infeasible
				continue;
		}

		// If we've made it this far, the candidate should be kept
		top_move = make_pair(NO_ID, choice);
		top_objective = obj_candidate;
	}
	cout << '.';

	// Return the best solution vector
	cout << endl;
	return make_pair(top_move, top_objective);
}

/**
Conducts an exhaustive, greedy local search from the current solution.

Each iteration of the search moves to the neighbor with the best objective value. The search ends when local optimality is achieved.
*/
void Search::exhaustive_search()
{
	// Find best neighbor
	pair<pair<int, int>, double> move = best_neighbor();

	// Continue main loop until reaching local optimality
	while (move.second < INFINITY)
	{
		clock_t start = clock(); // iteration timer

		exhaustive_iteration++;
		cout << "\n---------- Exhaustive Search Iteration " << exhaustive_iteration << " ----------\n" << endl;
		EveLog->iteration = exhaustive_iteration;
		EveLog->event_case = EVENT_EXHAUSTIVE;

		// Make local move and update objective and vehicle usage
		sol_current = make_move(move.first.first, move.first.second);
		sol_best = sol_current;
		obj_current = move.second;
		obj_best = obj_current;
		vehicle_totals();
		EveLog->obj_current = obj_current;
		EveLog->obj_best = obj_best;
		EveLog->new_best = 1;

		if (move.first.second == NO_ID)
			EveLog->add_id = move.first.first;
		else if (move.first.first == NO_ID)
			EveLog->drop_id = move.first.second;
		EveLog->total_time = (1.0*clock() - start) / CLOCKS_PER_SEC;
		EveLog->log_iteration(sol_current);

		// Repeat neighborhood search
		move = best_neighbor();
	}
}
