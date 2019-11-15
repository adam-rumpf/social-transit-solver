/**
A variety of structures for storing a network representation of the public transit system.

Includes a Network, Arc, Node, and Line class. Objects from these classes are built from the input data, after which they are mostly treated as read-only for use in the objective and constraint calculation functions.
*/

#pragma once

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <unordered_map>
#include <vector>
#include "DEFINITIONS.hpp"

using namespace std;

// Structure declarations
struct Network;
struct Node;
struct Arc;
struct Line;

/**
A class for the network representation of the public transit system.

Its constructor reads the node and arc data files and uses them to define Node and Arc objects. Pointers to these objects are then stored in different lists, partitioned depending on their function, for use in the objective and constraint function calculations.

Most of the network objects are partitioned into a "core" set which is used for all purposes (including stop/boarding nodes and line/boarding/alighting/walking arcs), and an "access" set which is only needed for the primary care access metrics (including population/facility nodes and their associated walking arcs). Only the core set needs to be considered for the constraint calculation, while the access sets must be added in for the objective.
*/
struct Network
{
	// Public attributes
	vector<Line *> lines; // pointers to each line, arranged in the same order as the solution vector
	vector<Node *> nodes; // pointers to all nodes
	vector<Node *> core_nodes; // pointers to all core nodes (stop and boarding)
	vector<Node *> stop_nodes; // pointers to all stop nodes
	vector<Node *> boarding_nodes; // pointers to all boarding nodes
	vector<Node *> population_nodes; // pointers to all population center nodes
	vector<Node *> facility_nodes; // pointers to all primary care facility nodes
	vector<Arc *> core_arcs; // pointers to all core network arcs
	vector<Arc *> line_arcs; // pointers to all core network line arcs
	vector<Arc *> walking_arcs; // pointers to all core network walking arcs
	vector<Arc *> access_arcs; // pointers to access network walking arcs

	// Public methods
	Network(); // constructor uses input data file names from the definition header to automatically build the network
	~Network(); // destructor deletes all Node, Arc, and Line objects
};

/**
A class for the public transit network's nodes.

Stores various node-level attributes, including some sets partitioned into use for the core network and the access network.
*/
struct Node
{
	// Public attributes
	vector<Arc *> core_out; // pointers to outgoing arcs that belong to the core network
	vector<Arc *> core_in; // pointers to incoming arcs that belong to the core network
	vector<Arc *> access_out; // pointers to outgoing arcs that belong to the access network
	vector<double> incoming_demand; // (stop nodes only) travel demands from every other stop node, in same order as network's core node list
	int id; // ID number (should match position in node list)
	double value; // value relevant to node type (population of a population center, weight of a facility)

	// Public methods
	Node(); // default constructor sets id and value to -1
	Node(int, double); // alternate constructor to specify id and value
};

/**
A class for the public transit network's arcs.

Stores various arc-level attributes.
*/
struct Arc
{
	// Public attributes
	Node * tail; // pointer to tail node
	Node * head; // pointer to head node
	int id; // ID number (should match position in arc list)
	double cost; // constant travel time
	int line = -1; // line ID (-1 if N/A)
	bool boarding = false; // whether or not this is a boarding arc

	// Public methods
	Arc(int, Node*, Node*, double, int, int); // constructor sets ID, tail/head endpoints, cost, line, and type
};

/**
A class for the public transit network's lines.

Stores various line-level attributes.

Also includes methods for calculating the frequency and capacity for a given fleet size. This is to avoid having to store fleet sizes internally, since we will be considering many different fleet sizes during the neighborhood searches.
*/
struct Line
{
	//Public attributes
	vector<Arc *> boarding; // pointers to associated boarding arcs
	vector<Arc *> in_vehicle; // pointers to associated line arcs (in-vehicle travel)
	double circuit; // time required for a vehicle to complete one circuit (minutes)
	double seating; // seating capacity of each vehicle used by this line
	double day_fraction; // fraction of day during which the line operates (1.0 indicates full day)
	double day_horizon; // daily time horizon (minutes)

	// Public methods
	Line(double, double, double, double); // constructor sets circuit time, seating capacity, active fraction of day, and daily time horizon
	double frequency(int); // returns frequency resulting from a given fleet size
	double headway(int); // returns average headway resulting from a given fleet size
	double capacity(int); // returns capacity resulting from a given fleet size
};
