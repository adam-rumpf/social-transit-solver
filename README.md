# social-transit-solver

The main tabu search/simulated annealing hybrid solution algorithm for use in a research project of mine dealing with a public transit design model with social access objectives.

This program shares a large amount of code with a [social-transit-solver-single](https://github.com/adam-rumpf/social-transit-solver-single), including its objective function, constraint function, and assignment model. It largely consists of a search process that uses these modules as subroutines.

This program requires an input folder called `data/` and an output folder called `log/`, both of which will be explained below. By default the program will expect them in its local directory, but it can be passed a command line argument to specify a different base directory for where to find `data/` and `log/`.

Note that this program may take an extremely long time to run for large problem instances. A search parameter file allows the user to specify the number of search iterations. The program can also be terminated early by pressing `[Ctrl]+[C]` during its execution, which causes the it to safely quit at the end of the current iteration (which may still take a fair amount of time).

I would not expect this program to be of much use to anyone outside of our research group, but it is provided here for anyone interested.

## Output Logs

This program writes outputs to a local `log/` folder. The following files are produced:

* [`event.txt`](#eventtxt): A log giving a summary of the events during each iteration of the solution process. See below for details.
* `final.txt`: Includes the best known solution vector along with its objective value.
* [`memory.txt`](#memorytxt): The memory structures associated with the tabu search/simulated annealing hybrid search process. Used to continue a halted search process. Not meant meant to be easily interpreted, but details are included below just in case.
* `metrics.txt`: Accessibility metrics of each population center for the best known solution.
* `solution.txt`: Log of all previously-searched solutions along with their feasibility status, constraint function elements, objective values, and evaluation times. Used to maintain a solution dictionary in order to avoid having to process searched solutions a second time. Its format is the same as that of the input file [`initial_solution_log.txt`](#initial_solution_logtxt), but due to the unordered map used to store solutions internally during execution the order of the rows is arbitrary and may change between executions.

The program also prints to the command line as it runs in order to report the main algorithm iteration number and other major events. During the neighborhood search, which is the most time-consuming part of the process, it prints a sequence of characters as an indication that it is still working (specifically, it prints `|` when starting or restarting the first pass, `a` whenever considering a new ADD move during the first pass, `d` for a DROP move, `*` when beginning a constraint calculation, and `.` for each iteration of Frank-Wolfe during constraint calculation).

### `event.txt`

The event log is formatted as a tab-separated table to facilitate automatic processing or insertion into a spreadsheet. Each row includes information about the results at the end of the current search iteration. The first row (iteration `0`) indicates the initial solution, for which most of the search result columns display a default value of `-1`.

If the search is halted via keyboard stop request, a row consisting entirely of `-1` will be included to indicate the break.

Includes the following columns:
* `Iteration`: Iteration number.
* `Obj_Current`: Current objective value.
* `Obj_Best`: Best known objective value.
* `New_Best`: Whether or not the best known objective was improved (`1` if true, `0` if false).
* `Case`: Code indicating the case of the main search loop:
  * `1`: Improvement over the previous solution was found.
  * `2`: Nonimprovement but passed SA criterion.
  * `3`: Nonimprovement but failed SA criterion.
  * `4`: Final exhaustive search iteration.
* `SA_Prob`: Probability of passing the SA criterion (if applicable, and `-1` otherwise).
* `Jump`: Whether or not an attractive solution was jumped to as the result of the inner nonimprovement counter getting too high (`1` if true, `0` if false).
* `Nonimp_In`: Value of inner nonimprovement counter.
* `Nonimp_Out`: Value of outer nonimprovement counter.
* `Tenure`: Current tabu tenures.
* `Temperature`: Current simulated annealing temperature.
* `ADD`: Line ID of any ADD moves made (`-1` if none).
* `DROP`: Line ID of any DROP moves made (`-1` if none).
* `Obj_Lookups`: Number of objective values successfully retrieved from the solution log.
* `Con_Lookups`: Number of constraint values successfully retrieved from the solution log.
* `Obj_Evals`: Number of objective values newly-evaluated for the solution log.
* `Con_Evals`: Number of constraint values newly-evaluated for the solution log.
* `ADD_First`: Number of ADD moves collected during the first pass.
* `DROP_First`: Number of DROP moves collected during the first pass.
* `ADD_Second`: Number of ADD moves collected during the second pass.
* `DROP_Second`: Number of DROP moves collected during the second pass.
* `SWAPs`: Number of SWAP moves collected.
* `Total_Time`: Total time spent on entire iteration (in seconds).
* `Solution`: Current solution vector, expressed as its string from the solution log.

### `memory.txt`

The memory log is meant for internal use by the search algorithm, but since it is formatted as a plain text file it can also be read for diagnostic purposes.

After an initial comment line, it includes the following rows:
* `add_tenure`: Tab-separated list of ADD tabu tenures for all solution vector elements.
* `drop_tenure`: Tab-separated list of DROP tabu tenures for all solution vector elements.
* `sol_current`: Tab-separated list representing current solution vector.
* `sol_best`: Tab-separated list representing best known solution vector.
* `obj_current`: Current objective value.
* `obj_best`: Best known objective value.
* `iteration`: Current iteration number.
* `nonimp_in`: Inner nonimprovement counter.
* `nonimp_out`: Outer nonimprovement counter.
* `tenure`: Current tenure to assign to new tabu moves.
* `temperature`: Current simulated annealing temperature.
* `attractive_objectives`: Tab-separated list of attractive solution objectives.
* `attractive_solutions`: All remaining rows consist of tab-separated lists defining the attractive solution vectors, in the same order as the objectives in the above row.

## Data Folder

This program reads input files from a local `data/` folder. The following data files should be included in this folder:

* [`arc_data.txt`](#arc_datatxt)
* [`assignment_data.txt`](#assignment_datatxt)
* [`initial_flows.txt`](#initial_flowstxt)
* [`initial_solution_log.txt`](#initial_solution_logtxt)
* [`node_data.txt`](#node_datatxt)
* [`objective_data.txt`](#objective_datatxt)
* [`od_data.txt`](#od_datatxt)
* [`problem_data.txt`](#problem_datatxt)
* [`search_parameters.txt`](#search_parameterstxt)
* [`transit_data.txt`](#transit_datatxt)
* [`user_cost_data.txt`](#user_cost_datatxt)
* [`vehicle_data.txt`](#vehicle_datatxt)

The contents of these files will be explained below. Most include IDs for each of their elements. For the purposes of our solution algorithm these are assumed to consecutive integers beginning at `0`, and this is how they will be treated for the purposes of array placement.

Unless otherwise specified, the following units are used:

* cost = dollars
* distance = miles
* time = minutes

### `arc_data.txt`

Information related to all arcs. Due to the internal network object storage the accessibility walking arcs (type `4`) must be listed in a contiguous block at the end of the arc list.

Contains the following columns:

* `ID`: Unique identifying number.
* `Type`: Arc type ID. The types in use are:
  * `0`: line arc
  * `1`: boarding arc
  * `2`: alighting arc
  * `3`: core network walking arc (stop nodes only)
  * `4`: accessibility network walking arc (stop nodes, population centers, and facilities)
* `Line`: Line ID of a line arc, and `-1` otherwise.
* `Tail`: Node ID of the arc's tail.
* `Head`: Node ID of the arc's head.
* `Time`: Constant part of travel time of arc. Boarding arcs, whose travel time is based on the line frequency, have a listed time of `0`.

### `assignment_data.txt`

Information related to the Spiess and Florian assignment model.

Contains the following rows:

* `FW_Error_Epsilon`: Optimality gap threshold to use for ending the Frank-Wolfe algorithm.
* `FW_Flow_Epsilon`: Solution change threshold (inf-norm) to use for ending the Frank-Wolfe algorithm. This bound is for the flow vector.
* `FW_Waiting_Epsilon`: Solution change threshold to use for ending the Frank-Wolfe algorithm. This bound is for the waiting time scalar, which in general is so large compared to the arc flow values that a larger threshold is appropriate.
* `FW_Cutoff`: Iteration cutoff for the Frank-Wolfe algorithm.
* `Elements`: Number of parameters listed on the following rows. Currently set to `2`.
* `alpha`: Alpha parameter of the conical congestion function.
* `beta`: Beta parameter of the conical congestion function. By definition it should equal `(2 alpha - 1)/(2 alpha - 2)`, and is included here only for convenience.

### `initial_flows.txt`

Initial flows on the core network resulting from the initial fleet vector. Use [social-transit-solver-single](https://github.com/adam-rumpf/social-transit-solver-single) to produce this file.

Contains the following columns:

* `ID`: Arc ID. These should match the IDs of all core arcs.
* `Flow`: Initial flow value for the given core arc.

### `initial_solution_log.txt`

Initial version of the solution log file, used by the solver to initialize a solution log in memory. This is essentially just a copy of the `solution.txt` file from the `log/` folder except that it should contain only a single row, corresponding to the initial solution vector. Use [social-transit-solver-single](https://github.com/adam-rumpf/social-transit-solver-single) to generate this file.

Contains the following columns:
* `Solution`: String version of the initial fleet size vector, consisting of a sequence of integers separated by underscores (`_`).
* `Feasible`: Feasibility status of initial solution. By definition this should always be `1`.
* `UC_Riding`: Initial solution total in-vehicle riding time.
* `UC_Walking`: Initial solution total walking time.
* `UC_Waiting`: Initial solution total waiting time.
* `Con_Time`: Time required (in seconds) to calculate the initial solution's constraint function value. This is not used for any calculations and is only included out of interest.
* `Objective`: Initial objective value.
* `Obj_Time`: Time required (in seconds) to calculate the initial solution's objective function value. This is not used for any calculations and is only included out of interest.

### `node_data.txt`

Information related to all nodes. Due to the internal network storage all population center and primary care facility nodes (types `2` and `3`) must be listed in a contiguous block at the end of the node list.

Contains the following columns:

* `ID`: Unique identifying number. Used to reference specific nodes in the other data files.
* `Name`: Name of the node. Most stops are simply called "Stop" followed by their ID number. Boarding nodes also append the name of their line. Population centers and primary care facilities use their real names.
* `Type`: Node type ID. The types in use are:
  * `0`: stop node
  * `1`: boarding node
  * `2`: population center
  * `3`: primary care facility
* `Line`: Line ID of a boarding node, and `-1` otherwise.
* `Value`: Population of a population center, facility weight of a primary care facility, and `-1` otherwise.

### `objective_data.txt`

Information related to defining the objective function and related accessibility metrics.

Contains the following rows:

* `Elements`: Number of parameters listed on the following rows. Currently set to `3`.
* `Lowest`: Number of lowest-metric population centers to take for the objective function.
* `Gravity_Falloff`: Exponent used to define distance falloff in gravity metric. This should be a positive value, and will be treated as negative in the program. A larger value means faster falloff.
* `Multiplier`: Factor by which to multiply the accessibility metrics. This should be chosen to compensate for very small decimal values that would otherwise risk truncation error.

### `od_data.txt`

Origin/destination travel demands. Only nonzero demands are meant to be included.

Contains the following columns:

* `ID`: Unique identifying number.
* `Origin`: Node ID of origin.
* `Destination`: Node ID of destination.
* `Volume`: Number of people wishing to travel from the origin to the destination.

### `problem_data.txt`

Miscellaneous data required to define the problem.

Contains the following rows:

* `Elements`: Number of parameters listed on the following rows. Currently set to `1`.
* `Horizon`: Total daily time horizon.

### `search_parameters.txt`

Parameters for the tabu search/simulated annealing hybrid algorithm.

Contains the following rows:

* `Elements`: Number of parameters listed on the following rows. Currently set to `16`.
* `Continue`: Indicates whether to continue a previously-halted search using its printed log files. Set to `1` to continue the previous search or `0` to start a new search. Note that starting a new search wipes clean all of the files in the `log/` folder (except for the initial row of the solution log file, which is always required for initialization).
* `Iterations`: Number of iterations to perform during the overall local search algorithm (can be safely terminated before this point by pressing `[Ctrl]+[C]`).
* `Temp_Init`: Initial simulated annealing temperature.
* `Temp_Factor`: Factor by which to multiply the temperature when cooling. Should be between `0.0` and `1.0`.
* `Attractive_Max`: Maximum size of attractive solution set.
* `Nbhd_Add_Lim`: Number of ADD moves to collect during first pass.
* `Nbhd_Add_Lim2`: Actual number of desired ADD moves after second pass. Should be less than the previous value.
* `Nbhd_Drop_Lim`: Number of DROP moves to collect during first pass.
* `Nbhd_Drop_Lim2`: Actual number of desired DROP moves after second pass. Should be less than the previous value.
* `Nbhd_Swap_Lim`: Number of SWAP moves to collect during first pass.
* `Tenure_Init`: Initial tabu tenure.
* `Tenure_Factor`: Factor by which to multiply the tabu tenures when they are increased.
* `Nonimp_In_Max`: Maximum value of inner nonimprovement counter (the inner counter roughly determines when the process should start trying to diversify).
* `Nonimp_Out_Max`: Maximum value of outer nonimprovement counter (the outer counter roughly determines when the process should start trying to intensify).
* `Step`: Increment of ADD/DROP moves.
* `Exhaustive`: Indicates whether to conduct an exhaustive local search from the best known solution after the final iteration ends. Set to `1` to finish with an exhaustive search and `0` to skip it. Be aware that the exhaustive search may take a significant amount of time due to the large number of neighbors to evaluate.

### `transit_data.txt`

Information related to each transit line. This includes a variety of fields directly related to the solution vector, such as the initial number of vehicles on each line (which constitutes the initial solution vector).

Contains the following columns:

* ID: Unique identifying number. This should indicate its position in the solution vector. Also used for line-specific references in the other data files.
* `Name`: Name of the line listed in the GTFS files.
* `Type`: Vehicle type ID. Matches one of the IDs listed in the vehicle data file.
* `Fleet`: Initial fleet size.
* `Circuit`: Total time for a vehicle to complete one circuit.
* `Scaling`: Fraction of the day during which the line is active. `1.0` indicates the entire daily time horizon.
* `LB`: Lower bound of allowable fleet size.
* `UB`: Upper bound of allowable fleet size.
* `Fare`: Boarding fare.
* `Frequency`: Initial line frequency, measured only during its active portion of the day. Relatively unimportant since it is recalculated each iteration for the current solution.
* `Capacity`: Initial line capacity, measured as a total number of passengers that can be transported per day. Relatively unimportant since it is recalculated each iteration for the current solution.

### `user_cost_data.txt`

Information related to defining the user cost function.

Contains the following rows:

* `Initial`: User cost of the initial solution. Used for defining the allowable relative increase bounds. Use [social-transit-solver-single](https://github.com/adam-rumpf/social-transit-solver-single) to find this initial value.
* `Percent`: Allowable percentage increase in user cost (expressed as a decimal). Larger values give a larger feasible set. Set to `-1` to ignore the user cost constraints entirely. Note that, due to numerical errors, it is possible that any given solution's calculated user cost may be slightly larger than it should be, and so it is recommended to always set this parameter to at least a very small positive value rather than exactly `0.0`.
* `Elements`: Number of parameters listed on the following rows. Currently set to `3`.
* `Riding`: Weight of in-vehicle riding time.
* `Walking`: Weight of walking time.
* `Waiting`: Weight of waiting time.

### `vehicle_data.txt`

Information related to each vehicle. Each route has a specified vehicle type, and only routes with the same vehicle type may exchange vehicles during the search algorithm.

Contains the following columns:

* `Type`: Vehicle type ID. Referenced in the transit data file to specify the vehicle type of each route.
* `Name`: Name of the vehicle.
* `UB`: Maximum number of this type of vehicle allowed in the network.
* `Capacity`: Seating capacity of this vehicle type. It is assumed that loading factor has already been taken into account. For trains, it is assumed that this includes all cars on a given train.
* `Cost`: Operating cost per unit time.

## Exit Status

A few errors can cause the program to terminate. The following codes may be returned:

* `0`: Successful exit at the end of the `main()` function.
* `1`: Intentional early termination from keyboard stop request (`[Ctrl]+[C]`).
* `2`: Early termination due to a missing input file (see above for what is required).
* `3`: Early termination due to errors within an input file (see above for required format).
* `4`: Early termination due to local search producing no feasible neigbhors.
