/**
Logger objects for reading and writing various files required by the main solution algorithm.
*/

#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <queue>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include "definitions.hpp"

using namespace std;

// Global function prototypes
pair<vector<int>, double> get_initial_solution(); // returns the initial solution vector and objective value
string vec2str(const vector<int> &); // returns string version of integer vector
vector<int> str2vec(string); // returns an integer vector for a given solution string

/**
Event logger.

Reports the events that occur during each iteration of the search process and prints them to an output log for later review. Also outputs a log of the current and best objective values in each iteration.

In order to reduce the number of times we need to open the output file, the results of an iteration are only written at the end of an iteration. Until that point, internal attributes and search object attributes are used to store the events of the iteration.
*/
struct EventLog
{
	// Public attributes
	ios_base::openmode mode; // file open mode (append or truncate)
	int max_iterations; // iteration number cutoff

	// Public methods
	EventLog(bool); // constructor initializes event and objective log files and sets file open mode
	void log_iteration(int, double, double); // appends an iteration report to the event log file
};

/**
Memory logger.

Reads and writes the TS/SA algorithm's memory structures from and to an output file. This allows the search process to be halted and resumed later.

Most of the memory structures associated with the search process are stored as public attributes of the main search's memory log object, meaning that the memory log object should be involved whenever these values are accessed or modified.

The log file should always be automatically generated by this program and should never need to be viewed or edited by the user, but for reference its rows are arranged as follows:
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
	all remaining rows consist of tab-separated lists defining the attractive solution vectors, in the same order as the objectives in the above row
*/
struct MemoryLog
{
	// Public attributes
	int sol_size; // length of solution vector
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
	vector<pair<vector<int>, double>> attractive_solutions; // vector of attractive solutions, stored as solution vector/objective value pairs

	// Public methods
	MemoryLog(int, bool); // constructor initializes internal search parameters associated with memory file
	~MemoryLog(); // destructor automatically calls the memory writing method
	void load_memory(); // reads contents of memory log file into memory log object
	void reset_memory(); // sets memory structures according to the initial values from the search parameter file and the initial solution log file
	void save_memory(); // writes contents of memory log object to the memory log file
	void output_best(); // writes an output file containing just the best known solution and its objective value
};

/**
Solution logger.

Records information about every solution encountered during the search process. This allows us to simply look up previously-generated solutions instead of having to reevaluate the objective and constraint functions.

Includes methods for reading from and writing to the solution log input and output files.

Due to the structure of the main solver's neighborhood search, we may occasionally generate the objective function value but not the constraint function value for a logged solution. For this reason, some rows in the solution log may be incomplete, but may be filled in later if the constraint function value is needed. Several of the methods of this class revolve around looking up or filling in pieces of information about solutions.

The most important attribute of this class is the solution log, which is an unordered map of solutions. The log is indexed by the string version of the corresponding solution vector. The entry in the log is a 5-part tuple consisting of the following:
	<0> the feasibility result
	<1> a vector of constraint function elements
	<2> the constraint evaluation time
	<3> the objective value
	<4> the objective evaluation time
*/
struct SolutionLog
{
	// Public attributes
	unordered_map<string, tuple<int, vector<double>, double, double, double>> sol_log; // dictionary of solutions, indexed by the string version of the solution vector

	// Public methods
	SolutionLog(bool); // constructor reads the solution log file and initializes the solution memory structure
	~SolutionLog(); // destructor automatically calls the log writing method
	void load_solution(string); // reads a given solution log into the dictionary
	void save_solution(); // writes the current solution log to the log file
	void create_row(const vector<int> &, int, const vector<double> &, double, double, double); // creates or updates a solution log entry for a given solution vector with given information
	bool solution_exists(const vector<int> &); // determines whether a given solution vector is present in the solution log
	tuple<int, vector<double>, double> lookup_row(const vector<int> &); // retrieves feasibility status, constraint function elements, and objective value of a given solution
	void update_row(const vector<int> &, int, const vector<double> &, double); // modifies the feasibility status, constraints, and constraint time for a logged solution
};
