#include "network.hpp"

/**
Network constructor to automatically build network from data files.

Reads the contents of these files and uses them to fill its own line, node, and arc lists, while also initializing those objects.
*/
Network::Network()
{
	// Read problem file to get time horizon
	double horizon = 1440.0; // default to whole 24 hours
	ifstream problem_file;
	problem_file.open(PROBLEM_FILE);
	if (problem_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(problem_file, line); // skip comment 
		getline(problem_file, line); // skip elements line
		getline(problem_file, line); // get time horizon line

		stringstream stream(line);
		getline(stream, piece, '\t'); // Name
		getline(stream, piece, '\t'); // Horizon
		horizon = stod(piece); // get time horizon value

		problem_file.close();
	}
	else
	{
		cout << "Problem file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}

	// Read node file and create node lists
	ifstream node_file;
	node_file.open(NODE_FILE);
	if (node_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(node_file, line); // skip comment line

		while (node_file.eof() == false)
		{
			// Get whole line as a string stream
			getline(node_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);

			// Go through each piece of the line
			getline(stream, piece, '\t'); // ID
			int node_id = stoi(piece);
			getline(stream, piece, '\t'); // Name
			getline(stream, piece, '\t'); // Type
			int node_type = stoi(piece);
			getline(stream, piece, '\t'); // Line
			getline(stream, piece, '\t'); // Value
			double node_value = stod(piece);

			// Create a node object and add it to the appropriate network lists
			Node * new_node = new Node(node_id, node_value);
			nodes.push_back(new_node);
			switch (node_type)
			{
				case STOP_NODE:
					stop_nodes.push_back(new_node);
					core_nodes.push_back(new_node);
					break;
				case BOARDING_NODE:
					boarding_nodes.push_back(new_node);
					core_nodes.push_back(new_node);
					break;
				case POPULATION_NODE:
					population_nodes.push_back(new_node);
					break;
				case FACILITY_NODE:
					facility_nodes.push_back(new_node);
					break;
			}
		}

		node_file.close();
	}
	else
	{
		cout << "Node file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}

	// Read vehicle file and record the information required to define the vehicle types and lines
	
	ifstream vehicle_file;
	vehicle_file.open(VEHICLE_FILE);
	if (vehicle_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(vehicle_file, line); // skip comment line

		while (vehicle_file.eof() == false)
		{
			// Get whole line as a string stream
			getline(vehicle_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);

			// Go through each piece of the line
			getline(stream, piece, '\t'); // Type
			getline(stream, piece, '\t'); // Name
			getline(stream, piece, '\t'); // UB
			int vehicle_bound = stoi(piece);
			getline(stream, piece, '\t'); // Seating
			double vehicle_cap = stod(piece);
			getline(stream, piece, '\t'); // Cost

			// Create a new vehicle type
			Vehicle * new_vehicle = new Vehicle(vehicle_bound, vehicle_cap);
			vehicles.push_back(new_vehicle);
		}

		vehicle_file.close();
	}
	else
	{
		cout << "Vehicle file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}

	// Read transit file and create line list
	ifstream transit_file;
	transit_file.open(TRANSIT_FILE);
	if (transit_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(transit_file, line); // skip comment line

		while (transit_file.eof() == false)
		{
			// Get whole line as a string stream
			getline(transit_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);

			// Go through each piece of the line
			getline(stream, piece, '\t'); // ID
			getline(stream, piece, '\t'); // Name
			getline(stream, piece, '\t'); // Type
			int vehicle_type = stoi(piece);
			getline(stream, piece, '\t'); // Fleet
			getline(stream, piece, '\t'); // Circuit
			double circuit_time = stod(piece);
			getline(stream, piece, '\t'); // Scaling
			double day_fraction = stod(piece);
			getline(stream, piece, '\t'); // LB
			getline(stream, piece, '\t'); // UB
			getline(stream, piece, '\t'); // Fare
			getline(stream, piece, '\t'); // Frequency
			getline(stream, piece, '\t'); // Capacity

			// Create a line object and add it to the list
			Line * new_line = new Line(vehicle_type, circuit_time, vehicles[vehicle_type]->capacity, day_fraction, horizon);
			lines.push_back(new_line);
		}

		transit_file.close();
	}
	else
	{
		cout << "Transit file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}

	// Read arc file and create arc lists
	ifstream arc_file;
	arc_file.open(ARC_FILE);
	if (arc_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(arc_file, line); // skip comment line

		while (arc_file.eof() == false)
		{
			// Get whole line as a string stream
			getline(arc_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);

			// Go through each piece of the line
			getline(stream, piece, '\t'); // ID
			int arc_id = stoi(piece);
			getline(stream, piece, '\t'); // Type
			int arc_type = stoi(piece);
			getline(stream, piece, '\t'); // Line
			int arc_line = stoi(piece);
			getline(stream, piece, '\t'); // Tail
			int arc_tail = stoi(piece);
			getline(stream, piece, '\t'); // Head
			int arc_head = stoi(piece);
			getline(stream, piece, '\t'); // Time
			double arc_time = stod(piece);

			// Create an arc object and add it to the appropriate network, node, and line lists
			Arc * new_arc = new Arc(arc_id, nodes[arc_tail], nodes[arc_head], arc_time, arc_line, arc_type);
			if (arc_type == ACCESS_ARC)
			{
				// An access arc goes into the main access arc list, its tail's outgoing access arc set, and its head's incoming access arc set
				access_arcs.push_back(new_arc);
				nodes[arc_tail]->access_out.push_back(new_arc);
			}
			else
			{
				// A non-access arc goes into the main core arc list, its tail's outgoing/incoming core arc sets, and its head's incoming core arc set
				core_arcs.push_back(new_arc);
				nodes[arc_tail]->core_out.push_back(new_arc);
				nodes[arc_head]->core_in.push_back(new_arc);
				if (arc_type == LINE_ARC)
				{
					// A line arc additionally goes into the network's line arc list and its line's line arc list
					line_arcs.push_back(new_arc);
					lines[arc_line]->in_vehicle.push_back(new_arc);
				}
				if (arc_type == BOARDING_ARC)
					// A boarding arc additionally goes into its line's boarding arc list
					lines[arc_line]->boarding.push_back(new_arc);
				if (arc_type == WALKING_ARC)
					// A walking arc additionally goes into the network's walking arc list
					walking_arcs.push_back(new_arc);
			}

			// Add a very small cost to boarding and alighting arcs
			if ((arc_type == BOARDING_ARC) || (arc_type == ALIGHTING_ARC))
				new_arc->cost += EPSILON;
		}

		arc_file.close();
	}
	else
	{
		cout << "Arc file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}

	// Initialize empty stop file travel demand lists
	for (int i = 0; i < stop_nodes.size(); i++)
		stop_nodes[i]->incoming_demand.resize(stop_nodes.size(), 0.0);

	// Read OD file and create travel demand lists for nodes
	ifstream od_file;
	od_file.open(OD_FILE);
	if (od_file.is_open())
	{
		string line, piece; // whole line and line element being read
		getline(od_file, line); // skip comment line

		while (od_file.eof() == false)
		{
			// Get whole line as a string stream
			getline(od_file, line);
			if (line.size() == 0)
				// Break for blank line at file end
				break;
			stringstream stream(line);

			// Go through each piece of the line
			getline(stream, piece, '\t'); // ID
			getline(stream, piece, '\t'); // Origin
			int origin_node = stoi(piece);
			getline(stream, piece, '\t'); // Destination
			int destination_node = stoi(piece);
			getline(stream, piece, '\t'); // Volume
			double travel_volume = stod(piece);

			// Read travel volume from the origin into its destination's travel demand list
			nodes[destination_node]->incoming_demand[origin_node] = travel_volume;
		}
	}
	else
	{
		cout << "OD file failed to open." << endl;
		exit(FILE_NOT_FOUND);
	}
}

/// Network destructor deletes all Node, Arc, and Line objects created by the constructor.
Network::~Network()
{
	for (int i = 0; i < lines.size(); i++)
		delete lines[i];

	for (int i = 0; i < nodes.size(); i++)
		delete nodes[i];

	for (int i = 0; i < core_arcs.size(); i++)
		delete core_arcs[i];

	for (int i = 0; i < access_arcs.size(); i++)
		delete access_arcs[i];
}

/// Node constructor that sets default value to -1.
Node::Node()
{
	id = -1;
	value = -1;
}

/// Node constructor to specify value.
Node::Node(int id_in, double value_in)
{
	id = id_in;
	value = value_in;
}

/// Arc constructor specifies its ID, tail/head node pointers, its cost, its line, and whether it is boarding.
Arc::Arc(int id_in, Node * tail_in, Node * head_in, double cost_in, int line_in, int type_in)
{
	id = id_in;
	tail = tail_in;
	head = head_in;
	cost = cost_in;
	line = line_in;
	if (type_in == BOARDING_ARC)
		boarding = true;
	else
		boarding = false;
}

/// Line constructor specifies its vehicle type, circuit time, seating capacity, active fraction of day, and daily time horizon.
Line::Line(int vehicle_in, double circuit_in, double seating_in, double fraction_in, double horizon_in)
{
	vehicle_id = vehicle_in;
	circuit = circuit_in;
	seating = seating_in;
	day_fraction = fraction_in;
	day_horizon = horizon_in;
}

/// Returns line frequency resulting from a given fleet size.
double Line::frequency(int fleet)
{
	return fleet / circuit;
}

/// Returns average line headway resulting from a given fleet size.
double Line::headway(int fleet)
{
	if (fleet > 0)
		return circuit / fleet;
	else
		return INFINITY;
}

/// Returns line capacity resulting from a given fleet size.
double Line::capacity(int fleet)
{
	return frequency(fleet) * day_fraction * day_horizon * seating;
}

/// Vehicle constructor specifies its fleet size limit and capacity.
Vehicle::Vehicle(int ub, double cap)
{
	max_fleet = ub;
	capacity = cap;
}
