/**
Logger objects for reading and writing various files required by the main solution algorithm.
*/

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <tuple>
#include <unordered_map>
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

Writes the TS/SA algorithm's memory structures to an output file. This allows the search process to be halted and resumed later.
*/
struct MemoryLog
{
	// Public attributes

	// Public methods
	MemoryLog() {};
};

/**
Solution logger.

Records information about every solution encountered during the search process. This allows us to simply look up previously-generated solutions instead of having to reevaluate the objective and constraint functions.

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
	SolutionLog(); // constructor reads the solution log file and initializes the solution memory structure
	~SolutionLog(); // destructor automatically calls the log writing method
	void write_solution(); // writes the current solution log to the log file
	string solution_string(const vector<int> &); // returns string version of integer vector
	vector<int> solution_string_vector(string); // returns an integer vector for a given solution string
};
