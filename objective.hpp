/**
Objective function calculation.

The objective function is implemented as a class equipped with its own attributes and methods.
*/

#pragma once

#include <fstream>
#include <iomanip>
#include <iostream>
#include <math.h>
#include <ppl.h>
#include <queue>
#include <sstream>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>
#include "definitions.hpp"
#include "network.hpp"

using namespace std;
using namespace concurrency;

extern string FILE_BASE;

typedef pair<double, int> dist_pair; // min-priority queue of distance/ID pairs ordered by first element

/**
Objective function class.

A variety of local attributes are used to store information required for calculating the objective function.

Methods are used to execute different steps of the objective function calculation process, much of which is related to
distance calculation, and much of which is done in parallel.
*/
struct Objective
{
	// Public attributes
	Network * Net; // pointer to the main transit network object
	int lowest_metrics = 1; // size of lowest metric set to use for calculating the objective value
	double gravity_exponent = 1.0; // gravity metric distance falloff exponent (will be made negative for calculations)
	double multiplier = 1.0; // multiplication factor for metric values
	int pop_size; // number of population nodes
	int fac_size; // number of facility nodes

	// Public methods
	Objective(Network *); // constructor that reads objective function data and sets network object pointer
	double calculate(const vector<int> &); // calculates objective value
	vector<double> all_metrics(const vector<int> &); // calculates gravity metrics for all population centers
	void population_to_all_facilities(int, const vector<double> &, vector<double> &); // distance from given source pop
	double facility_metric(int, vector<vector<double>> &); // calculates gravity metric for a given facility
	double population_metric(int, vector<vector<double>> &, vector<double> &); // gravity metric, distance mat, fac met
	void save_metrics(const vector<int> &); // calculates gravity metrics for population centers and prints to output
};
