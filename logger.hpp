/**
Logger objects for reading and writing various files required by the main solution algorithm.
*/

#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>
#include "definitions.hpp"

using namespace std;

/**
Event logger.

Reports the events that occur during each iteration of the search process and prints them to an output log for later review.
*/
struct EventLog
{
	// Public attributes

	// Public methods
	EventLog() {};
};

/**
Memory logger.

Reads and writes the TS/SA algorithm's memory structures from and to an output file. This allows the search process to be halted and resumed later.

Most of the memory structures associated with the search process are stored as public attributes of the main search's memory log object, meaning that the memory log object should be involved whenever these values are accessed or modified.
*/
struct MemoryLog
{
	// Public attributes

	// Public methods
	MemoryLog(bool); // constructor initializes internal search parameters associated with memory file
	~MemoryLog(); // destructor automatically calls the memory writing method
	void load_memory() {}; // reads contents of memory log file into memory log object
	void reset_memory() {}; // sets memory structures according to the initial values from the search parameter file
	void save_memory() {}; // writes contents of memory log object to the memory log file
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
	pair<vector<int>, double> get_initial_solution(); // returns the initial solution vector and objective value
	string vec2str(const vector<int> &); // returns string version of integer vector
	vector<int> str2vec(string); // returns an integer vector for a given solution string
};
