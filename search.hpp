/**
Main driver class of the TS/SA solution algorithm along with various logger classes.

Called by the main() function after all subroutine objects have been initialized, and uses them to conduct the search
process.
*/

#pragma once

#include <algorithm>
#include <cmath>
#include <csignal>
#include <cstdlib>
#include <ctime>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <list>
#include <queue>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>
#include "definitions.hpp"
#include "constraints.hpp"
#include "network.hpp"
#include "objective.hpp"

using namespace std;

extern string FILE_BASE;

typedef pair<pair<pair<int, int>, double>, pair<pair<int, int>, double>> neighbor_pair; // nbhd search output
typedef priority_queue<tuple<double, pair<int, int>, bool>, vector<tuple<double, pair<int, int>, bool>>,
	greater<tuple<double, pair<int, int>, bool>>> candidate_queue; // min-priority queue for obj/move/new tuples
typedef priority_queue<pair<double, pair<int, int>>, vector<pair<double, pair<int, int>>>, greater<pair<double,
	pair<int, int>>>> neighbor_queue; // min-priority queue for obj/move pairs at the end of neighborhood search

// Global function prototypes
pair<vector<int>, double> get_initial_solution(); // returns the initial solution vector and objective value
string vec2str(const vector<int> &); // returns string version of integer vector
vector<int> str2vec(string); // returns an integer vector for a given solution string

// Structure declarations
struct Search;
struct EventLog;
struct MemoryLog;
struct SolutionLog;

/**
Search object.

Contains search-related attributes and pointers to the main subroutine objects, and carries out the main solution
algorithm.
*/
struct Search
{
	// Public attributes (object pointers)
	Network * Net; // pointer to main network object
	Objective * Obj; // pointer to main objective object
	Constraint * Con; // pointer to main constraint object
	EventLog * EveLog; // pointer to event log object
	MemoryLog * MemLog; // pointer to memory log object
	SolutionLog * SolLog; // pointer to solution log object

	// Public attributes (search parameters and technical)
	bool started = false; // whether or not the solve() method has been called
	bool keyboard_halt = false; // whether or not to stop due to a keyboard halt
	bool pickup; // whether or not to continue a search from its saved data files (if false, log files are wiped clean)
	bool exhaustive; // whether or not to end with an exhaustive search from the final solution
	int sol_size; // size of solution vector
	int max_iterations; // maximum number of search iterations
	double temp_factor; // simulated annealing decay factor
	int attractive_max; // number of attractive solutions to store for tabu search
	int nbhd_add_lim1; // ADD moves to collect in first pass of neighborhood search
	int nbhd_add_lim2; // ADD moves to keep in second pass of neighborhood search
	int nbhd_drop_lim1; // DROP moves to collect in first pass of neighborhood search
	int nbhd_drop_lim2; // DROP moves to keep in second pass of neighborhood search
	int nbhd_swap_lim; // SWAP moves to keep in neighborhood search
	double tenure_init; // initial tabu tenure
	double tenure_factor; // tabu tenure multiplicative factor
	int nonimp_in_max; // cutoff for inner nonimprovement counter
	int nonimp_out_max; // cutoff for outer nonimprovement counter
	int step; // step size for moves
	vector<int> line_min; // lower vehicle bounds for all lines
	vector<int> line_max; // upper vehicle bounds for all lines
	vector<int> max_vehicles; // maximum number of each vehicle type
	vector<int> vehicle_type; // vector of vehicle types for each line

	// Public attributes (solution algorithm memory)
	vector<double> add_tenure; // vector of tabu tenures for ADD moves to each solution vector element
	vector<double> drop_tenure; // vector of tabu tenures for DROP moves to each solution vector element
	vector<int> sol_current; // current solution vector
	vector<int> sol_best; // best known solution vector
	double obj_current; // current objective value
	double obj_best; // best known objective value
	int iteration; // current iteration number
	int nonimp_in; // inner nonimprovement counter
	int nonimp_out; // outer nonimprovement counter
	double tenure; // tabu tenure for newly-added tabu moves
	double temperature; // simulated annealing temperature
	list<pair<vector<int>, double>> attractive_solutions; // list of attractive sols, as solution vector/obj value pairs
	vector<int> current_vehicles; // number of each vehicle type currently in use
	int exhaustive_iteration; // iteration of exhaustive local search

	// Public methods
	Search(); // constructor initializes network, objective, constraint, and various logger objects
	~Search(); // destructor deletes network, objective, and constraint objects
	void solve(); // main driver of the solution algorithm
	neighbor_pair neighborhood_search(); // performs nbhd search to find the best and second best neighboring moves
	vector<int> make_move(int, int); // returns the results of applying a move to the current solution
	void pop_attractive(bool); // deletes a random attractive solution and optionally sets it as the current solution
	void vehicle_totals(); // calculates total vehicles of each type in use
	void increase_tenure(); // increase the tabu tenure value
	void cool_temperature(); // apply a cooling schedule to the simulated annealing temperature
	void save_data(); // writes all current progress to the log files
	pair<pair<int, int>, double> best_neighbor(); // finds best move from current solution via exhaustive nbhd search
	void exhaustive_search(); // conducts an exhaustive local search from the current solution
};

/**
Event logger.

Reports the events that occur during each iteration of the search process and prints them to an output log for later
review.

In order to reduce the number of times we need to open the output file, the results of an iteration are only written at
the end of an iteration. Until that point, internal attributes and search object attributes are used to store the events
of the iteration.
*/
struct EventLog
{
	// Public attributes
	ios_base::openmode mode; // file open mode (append or truncate)
	int iteration; // current iteration
	double tenure; // current tabu tenure
	double temperature; // current simulated annealing temperature
	double obj_current; // current objective value
	double obj_best; // best known objective value
	int new_best; // whether a new best solution has been found
	int event_case; // code to describe case of main loop
	double sa_prob; // simulated annealing pass probability
	int jump; // whether we have jumped to an attractive solution
	int nonimp_in; // inner nonimprovement counter
	int nonimp_out; // outer nonimprovement counter
	int add_id; // line ID of ADD move
	int drop_id; // line ID of DROP move
	int obj_lookups; // count of objectives looked up
	int con_lookups; // count of constraints looked up
	int obj_evals; // count of objectives calculated
	int con_evals; // count of constraints calculated
	int add_first; // size of first-pass ADD list
	int drop_first; // size of first-pass DROP list
	int add_second; // size of second-pass ADD list
	int drop_second; // size of second-pass DROP list
	int swaps; // size of SWAP list
	double total_time; // time spent on iteration

	// Public methods
	EventLog(bool); // constructor initializes event and objective log files and sets file open mode
	void log_iteration(const vector<int> &); // writes a row for the current iteration to the event log file
	void reset(); // sets event log attributes to their default values
	void halt(int = KEYBOARD_HALT_SYMBOL); // writes a row to indicate that a halt has taken place
};

/**
Memory logger.

Reads and writes the TS/SA algorithm's memory structures from and to an output file. This allows the search process to
be halted and resumed later.

Most of the memory structures associated with the search process are stored as public attributes of the main search's
memory log object. The memory logger accesses these values via a pointer to the main search object.

The log file should always be automatically generated by this program and should never need to be viewed or edited by
the user, but for reference its rows are arranged as follows:
	comment line to explain row structure
	tab-separated list of ADD tabu tenures for all solution vector elements
	tab-separated list of DROP tabu tenures for all solution vector elements
	tab-separated list representing current solution vector
	tab-separated list representing best known solution vector
	current objective value
	best known objective value
	current iteration number
	inner nonimprovement counter
	outer nonimprovement counter
	current tenure for new tabu moves
	current simulated annealing temperature
	tab-separated list of attractive solution objectives
	all remaining rows consist of tab-separated lists defining the attractive solution vectors, in the same order as the
		objectives in the above row
*/
struct MemoryLog
{
	// Public attributes
	Search * Solver; // pointer to calling search object
	int sol_size; // length of solution vector

	// Public methods
	MemoryLog(Search *, bool); // constructor initializes internal search parameters associated with memory file
	void load_memory(); // reads contents of memory log file into memory log object
	void reset_memory(); // sets memory structures according to initial vals from search param file and init sol log
	void save_memory(); // writes contents of memory log object to the memory log file
	void output_best(); // writes an output file containing just the best known solution and its objective value
};

/**
Solution logger.

Records information about every solution encountered during the search process. This allows us to simply look up
previously-generated solutions instead of having to reevaluate the objective and constraint functions.

Includes methods for reading from and writing to the solution log input and output files.

Due to the structure of the main solver's neighborhood search, we may occasionally generate the objective function value
but not the constraint function value for a logged solution. For this reason, some rows in the solution log may be
incomplete, but may be filled in later if the constraint function value is needed. Several of the methods of this class
revolve around looking up or filling in pieces of information about solutions.

The most important attribute of this class is the solution log, which is an unordered map of solutions. The log is
indexed by the string version of the corresponding solution vector. The entry in the log is a 5-part tuple consisting of
the following:
	<0> the feasibility result
	<1> a vector of constraint function elements
	<2> the constraint evaluation time
	<3> the objective value
	<4> the objective evaluation time
*/
struct SolutionLog
{
	// Public attributes
	unordered_map<string, tuple<int, vector<double>, double, double, double>> sol_log; // dict of sols, by sol vec str

	// Public methods
	SolutionLog(bool); // constructor reads the solution log file and initializes the solution memory structure
	void load_solution(string); // reads a given solution log into the dictionary
	void save_solution(); // writes the current solution log to the log file
	void create_row(const vector<int> &, int, const vector<double> &, double, double, double); // updates sol log entry
	void create_partial_row(const vector<int> &, double, double); // creates sol log entry for sol given obj and time
	bool solution_exists(const vector<int> &); // determines whether a given sol vector is present in the solution log
	tuple<int, vector<double>, double> lookup_row(const vector<int> &); // returns feas, constraint elements, and obj
	pair<int, double> lookup_row_quick(const vector<int> &); // returns feas and objof a given solution
	void update_row(const vector<int> &, int, const vector<double> &, double); // modifies feas, cons, and time
	void ban_solution(const vector<int> &); // bans a solution so that it will never be searched again
};
