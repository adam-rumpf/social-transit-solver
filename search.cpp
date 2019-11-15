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



	////////////////////////////////
	EveLog->log_objective(0, 100, 100);
	EveLog->log_objective(1, 110, 100);



	/////// Note: If we end a loop due to stopping == true, we should safely quit with exit(KEYBOARD_HALT).
}
