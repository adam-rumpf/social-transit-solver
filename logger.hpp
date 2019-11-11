/**
Event logger.

Reports the events that occur during each iteration of the search process and prints them to an output log for later review.
*/

/**
Memory logger.

Writes the TS/SA algorithm's memory structures to an output file. This allows the search process to be halted and resumed later.
*/

/**
Solution logger.

Records information about every solution encountered during the search process. This allows us to simply look up previously-generated solutions instead of having to reevaluate the objective and constraint functions.
*/

#pragma once
