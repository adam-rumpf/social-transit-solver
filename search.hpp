/**
Main driver of the TS/SA solution algorithm.

Called by the main() function after all subroutine objects have been initialized, and uses them to conduct the search process.
*/

#pragma once

#include <csignal>
#include <ctime>
#include <fstream>
#include <iostream>
#include "definitions.hpp"
#include "constraints.hpp"
#include "logger.hpp"
#include "network.hpp"
#include "objective.hpp"

using namespace std;

/**
Search object.

Contains search-related attributes and pointers to the main subroutine objects, and carries out the main solution algorithm.
*/
struct Search
{
	// Public attributes
	Network * Net; // pointer to main network object
	Objective * Obj; // pointer to main objective object
	Constraint * Con; // pointer to main constraint object
	EventLog * ELog; // pointer to event log object
	MemoryLog * MLog; // pointer to memory log object
	SolutionLog * SLog; // pointer to solution log object
	bool started = false; // whether or not the solve() method has been called
	bool keyboard_halt = false; // whether or not to stop due to a keyboard halt
	bool pickup; // whether or not to continue a search from its saved data files (if false, log files are wiped clean)
	int max_iterations; // maximum number of search iterations
	double temp_init; // initial simulated annealing temperature
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

	// Public methods
	Search(); // constructor initializes network, objective, constraint, and various logger objects
	~Search(); // destructor deletes network, objective, and constraint objects
	void solve(); // main driver of the solution algorithm
};
