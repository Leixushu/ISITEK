////////////////////////////////////////////////////////////////////////////////

#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include "element.h"
#include "linear.h"
#include "memory.h"
#include "numerics.h"
#include "solver.h"

////////////////////////////////////////////////////////////////////////////////

#include "element.r"

////////////////////////////////////////////////////////////////////////////////

static double **vertex_x;

static int lds, ldm, ldd, lda, sizel, sizem;
static double **S, **M, **D, **A;

static int info, *pivot, int_1 = 1;
static char trans[2] = "NT";
static double dbl_0 = 0.0, dbl_1 = 1.0;

////////////////////////////////////////////////////////////////////////////////

int element_interpolation_start()
{
	int i;

	// locations
	vertex_x = matrix_double_new(NULL,2,ELEMENT_MAX_N_FACES);
	if(vertex_x == NULL) return ELEMENT_MEMORY_ERROR;

	// working matrices
	lds = solver_variable_max_n_bases();
	ldm = solver_variable_max_n_bases();
	ldd = solver_variable_max_n_bases();
	lda = solver_variable_max_n_bases();
	sizel = 0;
	for(i = 0; i < solver_n_variables(); i ++) sizel += solver_variable_n_bases()[i]*solver_variable_n_bases()[i];
	sizem = solver_variable_max_n_bases() * solver_variable_max_n_bases();
	S = matrix_double_new(NULL,(ELEMENT_MAX_N_FACES-2)*numerics_n_hammer(solver_variable_max_order()),lds);
	M = matrix_double_new(NULL,solver_variable_max_n_bases(),ldm);
	D = matrix_double_new(NULL,solver_variable_max_n_bases(),ldd);
	A = matrix_double_new(NULL,solver_variable_max_n_bases(),lda);
	if(S == NULL || M == NULL || D == NULL || A == NULL) return ELEMENT_MEMORY_ERROR;

	// lu pivot
	pivot = (int *)malloc((solver_variable_max_n_bases() + 2) * sizeof(int));
	if(pivot == NULL) return ELEMENT_MEMORY_ERROR;

	return ELEMENT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////

void element_interpolation_end()
{
	matrix_free((void *)vertex_x);
	matrix_free((void *)S);
	matrix_free((void *)M);
	matrix_free((void *)D);
	matrix_free((void *)A);
	free(pivot);
}

////////////////////////////////////////////////////////////////////////////////

int element_interpolation_calculate(ELEMENT element)
{
	int i, j;
	int taylor_power[2];
	int max_n_bases = solver_variable_max_n_bases();
	int sum_face_n_quadrature = 0;
	for(i = 0; i < element->n_faces; i ++) sum_face_n_quadrature += face_n_quadrature(element->face[i]);

	// locations
	for(i = 0; i < element->n_faces; i ++)
	{
		for(j = 0; j < 2; j ++)
		{
			vertex_x[j][i] = node_x(face_node(element->face[i])[face_border(element->face[i])[0] != element])[j];
		}
	}

	// internal interpolation
	element->P = tensor_double_new(element->P,solver_variable_max_n_bases(),solver_variable_max_n_bases(),element->n_quadrature);
	if(element->P == NULL) return ELEMENT_MEMORY_ERROR;

	for(i = 0; i < solver_variable_max_n_bases(); i ++)
	{
		for(j = 0; j < 2; j ++) taylor_power[j] = numerics_taylor_power(i,j);
		for(j = 0; j < solver_variable_max_n_bases(); j ++)
			numerics_basis(element->n_quadrature,element->P[i][j],element->X,element->centre,element->size,j,taylor_power);
	}

	// external interpolation
	element->Q = (double ***)malloc(element->n_faces * sizeof(double **));
	if(element->Q == NULL) return FACE_MEMORY_ERROR;
	element->Q[0] = (double **)malloc(element->n_faces * solver_variable_max_n_bases() * sizeof(double *));
	if(element->Q[0] == NULL) return FACE_MEMORY_ERROR;
	element->Q[0][0] = (double *)malloc(solver_variable_max_n_bases() * sum_face_n_quadrature * sizeof(double));
	if(element->Q[0][0] == NULL) return FACE_MEMORY_ERROR;
	for(i = 1; i < element->n_faces; i ++) element->Q[i] = element->Q[i-1] + solver_variable_max_n_bases();
	for(i = 1; i < element->n_faces; i ++) element->Q[i][0] = element->Q[i-1][0] + solver_variable_max_n_bases() * face_n_quadrature(element->face[i-1]);
	for(i = 0; i < element->n_faces; i ++) for(j = 1; j < solver_variable_max_n_bases(); j ++) element->Q[i][j] = element->Q[i][j-1] + face_n_quadrature(element->face[i]);

	for(j = 0; j < 2; j ++) taylor_power[j] = 0;
	for(i = 0; i < element->n_faces; i ++)
	{
		for(j = 0; j < solver_variable_max_n_bases(); j ++)
		{
			numerics_basis(face_n_quadrature(element->face[i]),element->Q[i][j],face_quadrature_x(element->face[i]),element->centre,element->size,j,taylor_power);
		}
	}

	// vertex interpolation
	element->V = matrix_double_new(NULL,solver_variable_max_n_bases(),element->n_faces);
	if(element->V == NULL) return ELEMENT_MEMORY_ERROR;
	for(j = 0; j < 2; j ++) taylor_power[j] = 0;
	for(i = 0; i < solver_variable_max_n_bases(); i ++)
	{
		numerics_basis(element->n_faces,element->V[i],vertex_x,element->centre,element->size,i,taylor_power);
	}

	// mass matrix
	for(i = 0; i < solver_variable_max_n_bases(); i ++) dcopy_(&element->n_quadrature,element->P[numerics_power_taylor(0,0)][i],&int_1,&S[0][i],&lds);
	for(i = 0; i < element->n_quadrature; i ++) dscal_(&max_n_bases,&element->W[i],S[i],&int_1);
	dgemm_(&trans[0],&trans[0],
			&max_n_bases,&max_n_bases,&element->n_quadrature,
			&dbl_1,
			S[0],&lds,
			element->P[numerics_power_taylor(0,0)][0],&element->n_quadrature,
			&dbl_0,
			M[0],&ldm);

	// initialise matrices
	element->I = (double ***)malloc(solver_n_variables() * sizeof(double **));
	if(element->I == NULL) return FACE_MEMORY_ERROR;
	element->I[0] = (double **)malloc(solver_n_variables() * element->n_quadrature * sizeof(double *));
	if(element->I[0] == NULL) return FACE_MEMORY_ERROR;
	element->I[0][0] = (double *)malloc(element->n_quadrature * solver_variable_sum_n_bases() * sizeof(double));
	if(element->I[0][0] == NULL) return FACE_MEMORY_ERROR;
	for(i = 1; i < solver_n_variables(); i ++) element->I[i] = element->I[i-1] + element->n_quadrature;
	for(i = 1; i < solver_n_variables(); i ++) element->I[i][0] = element->I[i-1][0] + element->n_quadrature * solver_variable_n_bases()[i-1];
	for(i = 0; i < solver_n_variables(); i ++) for(j = 1; j < element->n_quadrature; j ++) element->I[i][j] = element->I[i][j-1] + solver_variable_n_bases()[i];

	for(i = 0; i < solver_n_variables(); i ++)
	{
		for(j = 0; j < solver_variable_n_bases()[i]; j ++) dcopy_(&element->n_quadrature,&S[0][j],&lds,&element->I[i][0][j],&solver_variable_n_bases()[i]);
		dcopy_(&sizem,M[0],&int_1,A[0],&int_1);
		dgesv_(&solver_variable_n_bases()[i],&element->n_quadrature,A[0],&lda,pivot,element->I[i][0],&solver_variable_n_bases()[i],&info);
	}

	// limiting matrices
	element->L = (double ***)malloc(solver_n_variables() * sizeof(double **));
	if(element->L == NULL) return FACE_MEMORY_ERROR;
	element->L[0] = (double **)malloc(solver_variable_sum_n_bases() * sizeof(double *));
	if(element->L[0] == NULL) return FACE_MEMORY_ERROR;
	element->L[0][0] = (double *)malloc(sizel * sizeof(double));
	if(element->L[0][0] == NULL) return FACE_MEMORY_ERROR;
	for(i = 1; i < solver_n_variables(); i ++) element->L[i] = element->L[i-1] + solver_variable_n_bases()[i-1];
	for(i = 1; i < solver_n_variables(); i ++) element->L[i][0] = element->L[i-1][0] + solver_variable_n_bases()[i-1]*solver_variable_n_bases()[i-1];
	for(i = 0; i < solver_n_variables(); i ++) for(j = 1; j < solver_variable_n_bases()[i]; j ++) element->L[i][j] = element->L[i][j-1] + solver_variable_n_bases()[i];
	
	if(solver_variable_max_n_bases() > 1)
	{
		// diffusion matrix
		for(i = 0; i < solver_variable_max_n_bases(); i ++) dcopy_(&element->n_quadrature,element->P[numerics_power_taylor(1,0)][i],&int_1,&S[0][i],&lds);
		for(i = 0; i < element->n_quadrature; i ++) dscal_(&max_n_bases,&element->W[i],S[i],&int_1);
		dgemm_(&trans[0],&trans[0],
				&max_n_bases,&max_n_bases,&element->n_quadrature,
				&dbl_1,
				S[0],&lds,
				element->P[numerics_power_taylor(1,0)][0],&element->n_quadrature,
				&dbl_0,
				D[0],&ldd);
		for(i = 0; i < solver_variable_max_n_bases(); i ++) dcopy_(&element->n_quadrature,element->P[numerics_power_taylor(0,1)][i],&int_1,&S[0][i],&lds);
		for(i = 0; i < element->n_quadrature; i ++) dscal_(&max_n_bases,&element->W[i],S[i],&int_1);
		dgemm_(&trans[0],&trans[0],
				&max_n_bases,&max_n_bases,&element->n_quadrature,
				&dbl_1,
				S[0],&lds,
				element->P[numerics_power_taylor(0,1)][0],&element->n_quadrature,
				&dbl_1,
				D[0],&ldd);

		// limiting matrices
		for(i = 0; i < solver_n_variables(); i ++)
		{
			if(solver_variable_n_bases()[i] == 1) continue;
			dcopy_(&sizem,M[0],&int_1,A[0],&int_1);
			for(j = 0; j < solver_variable_n_bases()[i]; j ++) dcopy_(&solver_variable_n_bases()[i],D[j],&int_1,element->L[i][j],&int_1);
			dgesv_(&solver_variable_n_bases()[i],&solver_variable_n_bases()[i],A[0],&lda,pivot,element->L[i][0],&solver_variable_n_bases()[i],&info);
		}
	}

	return ELEMENT_SUCCESS;
}

////////////////////////////////////////////////////////////////////////////////
