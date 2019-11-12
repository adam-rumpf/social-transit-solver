/**
Global definitions used throughout various submodules.
*/

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
#define OPERATOR_COST_FILE "data/operator_cost_data.txt"
#define ASSIGNMENT_FILE "data/assignment_data.txt"
#define FLOW_FILE "data/initial_flows.txt"
#define SEARCH_FILE "data/search_parameters.txt"

// Output file names
#define METRIC_FILE "log/metrics.txt"
#define SOLUTION_LOG_FILE "log/solution.txt"
#define EVENT_LOG_FILE "log/event.txt"
#define MEMORY_LOG_FILE "log/memory.txt"
#define FINAL_SOLUTION_FILE "log/final.txt"
#define OBJECTIVE_LOG_FILE "log/objective.txt"

// Exit codes
#define SUCCESSFUL_EXIT 0
#define FILE_NOT_FOUND 1
#define KEYBOARD_HALT 2

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

// Other technical definitions
#define EPSILON 0.00000001 // very small positive value