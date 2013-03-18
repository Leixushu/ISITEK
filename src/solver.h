#ifndef SOLVER_H
#define SOLVER_H

#define SOLVER_SUCCESS 1
#define SOLVER_MEMORY_ERROR 0

void solver_residual(double *u, double **r);
void solver_jacobian(double *u, double ***j);

int solver_start();
void solver_end();

int solver_n_variables();
const int * solver_variable_order();
int solver_variable_max_order();
const int * solver_variable_n_bases();
int solver_variable_max_n_bases();
int solver_variable_sum_n_bases();

int solver_n_gauss();
int solver_n_hammer();
int solver_n_interpolations();
const int * solver_interpolation_variable();
const int * solver_interpolation_differential();
const char * solver_interpolation_method();

int solver_n_constants();
int solver_constant_set_value(char *name, double value);
int solver_constants_set();
int solver_constants_print(char *string);

#endif
