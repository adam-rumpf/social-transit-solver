/**
Main driver of the TS/SA solution algorithm.

Called by the main() function after all subroutine objects have been initialized, and uses them to conduct the search process.

Includes a signal handler to allow for keyboard halt via [Ctrl]+[C].
*/

#pragma once

#include <csignal>
#include <iostream>
#include "definitions.hpp"

using namespace std;

// Global variables
bool stopping = false; // whether a stop has been requested via keyboard

// Function prototypes
void stop_request(int signum);

/////// Note: If we end a loop due to stopping == true, we should safely quit with exit(KEYBOARD_HALT).
