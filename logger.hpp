/**
Logger objects for reading and writing various files required by the main solution algorithm.
*/

#pragma once

#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "definitions.hpp"

using namespace std;

// Function prototypes
string solution_string(const vector<int> &); // returns string version of integer vector
vector<int> solution_string_vector(string); // returns an integer vector for a given solution string

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
*/
struct SolutionLog
{
	// Public attributes

	// Public methods
	SolutionLog() {};
};
