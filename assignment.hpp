/**
Implementation of the nonlinear-cost Spiess and Florian user assignment model.

Solves the Spiess and Florian model to return the user flows based on a given solution. This involves conducting the Frank-Wolfe algorithm on the nonlinear model, which in turn involves iteratively solving the constant-cost version of the model.
*/

#pragma once
