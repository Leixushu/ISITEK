#ifndef ELEMENT_H
#define ELEMENT_H

typedef struct s_ELEMENT * ELEMENT;

#include "node.h"
#include "face.h"
#include "sparse.h"

#define ELEMENT_SUCCESS 1
#define ELEMENT_FAIL 0
#define ELEMENT_READ_ERROR -1
#define ELEMENT_MEMORY_ERROR -2
#define ELEMENT_LOGIC_ERROR -3

#define ELEMENT_MAX_N_FACES 4

ELEMENT element_new(int index);
int element_index(ELEMENT element);
int element_read_faces(FILE *file, FACE *face, ELEMENT element);
int element_n_faces(ELEMENT element);
void element_face(ELEMENT element, FACE * face);
int element_add_border(ELEMENT element);
int element_calculate_quadrature(ELEMENT element);
int element_n_quadrature(ELEMENT element);
void element_quadrature_x(ELEMENT element, double **x);
void element_quadrature_w(ELEMENT element, double *w);
void element_calculate_size(ELEMENT element);
double element_size(ELEMENT element);
int element_calculate_centre(ELEMENT element);
void element_centre(ELEMENT element, double *x);
int element_calculate_unknowns(ELEMENT element, int n_elements);
void element_unknown(ELEMENT element, int ** unknown);
int element_add_to_system(ELEMENT element, SPARSE system);
void element_print(ELEMENT element);
void element_plot(ELEMENT element);
void element_free(ELEMENT element);

int element_interpolation_start();
void element_interpolation_end();
int element_interpolation_calculate(ELEMENT element);

#endif
