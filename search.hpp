/**
Main driver of the TS/SA solution algorithm.

Called by the main() function after all subroutine objects have been initialized, and uses them to conduct the search process.
*/

#pragma once

#include <csignal>
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
	bool keyboard_halt = false; // whether or not to stop due to a keyboard halt

	// Public methods
	Search(); // constructor initializes network, objective, and constraint objects
	void solve(); // main driver of the solution algorithm
};





/////// Note: If we end a loop due to stopping == true, we should safely quit with exit(KEYBOARD_HALT).
