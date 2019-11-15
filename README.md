# social-transit-solver

The main tabu search/simulated annealing hybrid solution algorithm for use in a research project of mine dealing with a public transit design model with social access objectives.

This program shares a large amount of code with a [social-transit-solver-single](https://github.com/adam-rumpf/social-transit-solver-single), including its objective function, constraint function, and assignment model. This program largely consists of a search process that uses these modules as subroutines.

Note that this program may take an extremely long time to run for large problem instances. A search parameter file allows the user to specify the number of search iterations. The program can also be terminated early by pressing `[Ctrl]+[C]` during its execution, which causes the it to safely quit at the end of the current iteration (which may still take a fair amount of time).

I would not expect this program to be of much use to anyone outside of our research group, but it is provided here for anyone interested.

## Output Logs

This program writes outputs to a local `log/` folder. The following files are produced:

* `event.txt`: A log explaining what occurred during each iteration of the solution process, including the contents of the search neighborhood, the time spent searching, the selected move, the current objective value, and other events.
* `final.txt`: Includes the best known solution vector along with its objective value.
* `memory.txt`: The memory structures associated with the tabu search/simulated annealing hybrid search process. Used to continue a halted search process.
* `metrics.txt`: Accessibility metrics of each population center for the best known solution.
* `objective.txt`: Log of the current and best objective values in each iteration of the search process.
* `solution.txt`: Log of all previously-searched solutions along with their feasibility status, constraint function elements, objective values, and evaluation times. Used to maintain a solution dictionary in order to avoid having to process searched solutions a second time. Due to the unordered map used to store solutions internally during execution, the order of the rows is arbitrary and may change between executions.

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

Information related to all arcs.

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

Information related to all nodes.

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

* `Elements`: Number of parameters listed on the following rows. Currently set to `15`.
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
* `1`: Intentional early termination from keyboard stop request ([Ctrl]+[C]).
* `2`: Early termination due to a missing input file (see above for what is required).
* `3`: Early termination due to errors within an input file (see above for required format).
* `4`: Early termination due to incompatible elements within data files (for example, a mismatch between vector sizes or number of arguments).
