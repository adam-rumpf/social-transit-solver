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
			string value = piece;

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
				case 3:
					temp_init = stod(piece);
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
	EveLog = new EventLog();////////////////////////////
	MemLog = new MemoryLog(Net->lines.size(), pickup);
	SolLog = new SolutionLog(pickup);

	///////////////// Test a variety of circumstances, including saving/loading (CONTINUE_SEARCH, NEW_SEARCH).

	cout << "Search initialized." << endl;

	cout << "Initial log entries:" << endl;
	vector<int> temp_sol = SolLog->str2vec(SolLog->sol_log.begin()->first);
	for (int i = 0; i < temp_sol.size(); i++)
		cout << temp_sol[i] << '\t';
	cout << endl;
	tuple<int, vector<double>, double> temp = SolLog->lookup_row(temp_sol);
	cout << get<0>(temp) << '\t';
	for (int i = 0; i < UC_COMPONENTS; i++)
		cout << get<1>(temp)[i] << '\t';
	cout << get<2>(temp) << endl;

	cout << "Writing new entry." << endl;
	vector<int> new_sol = { 11, 22, 33 };
	vector<double> new_sol_con = { 1.1, 2.3, 5.8 };
	SolLog->create_row(new_sol, -1, new_sol_con, 0.125, 999999999, 0.294);

	cout << "Updating entry." << endl;
	new_sol_con = { 2.1, 3.4, 7.11 };
	SolLog->update_row(new_sol, 0, new_sol_con, 0.445);

	cout << "Getting initial solution." << endl;
	pair<vector<int>, double> initial_pair = SolLog->get_initial_solution();
	for (int i = 0; i < initial_pair.first.size(); i++)
		cout << initial_pair.first[i] << '\t';
	cout << '\n' << initial_pair.second << endl;




	/////// Note: If we end a loop due to stopping == true, we should safely quit with exit(KEYBOARD_HALT).
}
