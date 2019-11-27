/// Global definitions used throughout various submodules.

#pragma once

// Input file names
#define NODE_FILE "data/node_data.txt"
#define ARC_FILE "data/arc_data.txt"
#define OD_FILE "data/od_data.txt"
#define TRANSIT_FILE "data/transit_data.txt"
#define VEHICLE_FILE "data/vehicle_data.txt"
#define OBJECTIVE_FILE "data/objective_data.txt"
#define PROBLEM_FILE "data/problem_data.txt"
#define USER_COST_FILE "data/user_cost_data.txt"
#define ASSIGNMENT_FILE "data/assignment_data.txt"
#define FLOW_FILE "data/initial_flows.txt"
#define SEARCH_FILE "data/search_parameters.txt"
#define INPUT_SOLUTION_LOG_FILE "data/initial_solution_log.txt"

// Output file names
#define METRIC_FILE "log/metrics.txt"
#define OUTPUT_SOLUTION_LOG_FILE "log/solution.txt"
#define EVENT_LOG_FILE "log/event.txt"
#define MEMORY_LOG_FILE "log/memory.txt"
#define FINAL_SOLUTION_FILE "log/final.txt"

// Exit codes
#define SUCCESSFUL_EXIT 0
#define KEYBOARD_HALT 1
#define FILE_NOT_FOUND 2
#define INCORRECT_FILE 3

// Node and arc type IDs
#define STOP_NODE 0
#define BOARDING_NODE 1
#define POPULATION_NODE 2
#define FACILITY_NODE 3
#define LINE_ARC 0
#define BOARDING_ARC 1
#define ALIGHTING_ARC 2
#define WALKING_ARC 3
#define ACCESS_ARC 4
#define NO_ID -1

// Feasibility codes
#define FEAS_TRUE 1
#define FEAS_FALSE 0
#define FEAS_UNKNOWN -1

// Pickup codes
#define CONTINUE_SEARCH 1
#define NEW_SEARCH 0

// Solution log tuple elements
#define SOL_LOG_FEAS 0
#define SOL_LOG_UC 1
#define SOL_LOG_CON_TIME 2
#define SOL_LOG_OBJ 3
#define SOL_LOG_OBJ_TIME 4

// Event log codes
#define EVENT_IMPROVEMENT 1
#define EVENT_NONIMP_PASS 2
#define EVENT_NONIMP_FAIL 3
#define EVENT_EXHAUSTIVE 4

// Fixed parameters
#define UC_COMPONENTS 3 // number of components of the user cost vector
#define EVENT_LOG_COLUMNS 24 // number of columns in the event log
#define DELIMITER '_' // delimiter to use for defining solution log names

// Other technical definitions
#define EPSILON 0.00000001 // very small positive value
#define LARGE 10e20 // very large positive value
