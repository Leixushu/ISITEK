//////////////////////////////////////////////////////////////////

#include "isitek.h"
#include "constants.h"
#include "linear.h"

void node_read_geometry(FILE *file, struct NODE *node);
void face_read_geometry(FILE *file, struct FACE *face, struct NODE *node);
void element_read_geometry(FILE *file, struct ELEMENT *element, struct FACE *face);

void node_write_case(FILE *file, struct NODE *node);
void node_read_case(FILE *file, struct NODE *node);
void face_write_case(FILE *file, int n_variables, int *n_basis, int n_gauss, struct NODE *node, struct FACE *face, struct ELEMENT *element, struct BOUNDARY *boundary);
void face_read_case(FILE *file, int n_variables, int *n_basis, int n_gauss, struct NODE *node, struct FACE *face, struct ELEMENT *element, struct BOUNDARY *boundary);
void element_write_case(FILE *file, int n_variables, int *n_basis, int n_gauss, int n_hammer, struct FACE *face, struct ELEMENT *element);
void element_read_case(FILE *file, int n_variables, int *n_basis, int n_gauss, int n_hammer, struct FACE *face, struct ELEMENT *element);
void boundary_write_case(FILE *file, struct FACE *face, struct BOUNDARY *boundary);
void boundary_read_case(FILE *file, struct FACE *face, struct BOUNDARY *boundary);

void constants_input(FILE *file, char *constants);
int add_geometry_to_expression_string(char *string);

#define MAX_N_INDICES 1000

#define MAX_N_BOUNDARIES 20
#define MAX_BOUNDARY_N_FACES 1000
#define BOUNDARY_LABEL "boundary"
#define BOUNDARY_FORMAT "siss"

#define MAX_N_TERMS 25
#define MAX_TERM_N_VARIABLES 10
#define TERM_LABEL "term"
#define TERM_FORMAT "icdsssss"

#define MAX_N_CONSTANTS 20
#define CONSTANT_LABEL "constant"
#define CONSTANT_FORMAT "s"

//////////////////////////////////////////////////////////////////

void read_geometry(FILE *file, int *n_nodes, struct NODE **node, int *n_faces, struct FACE **face, int *n_elements, struct ELEMENT **element)
{
	char *line = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	exit_if_false(line != NULL,"allocating line string");

	int i;

	while(fgets(line, MAX_STRING_LENGTH, file) != NULL)
	{
		if(strncmp(line,"NODES",5) == 0)
		{
			exit_if_false(sscanf(&line[6],"%i",n_nodes) == 1,"reading the number of nodes");
			*node = allocate_nodes(*n_nodes);
			exit_if_false(*node != NULL,"allocating the nodes");
			for(i = 0; i < *n_nodes; i ++) node_read_geometry(file, &(*node)[i]);
		}
		if(strncmp(line,"FACES",5) == 0)
		{
			exit_if_false(sscanf(&line[6],"%i",n_faces) == 1,"reading the number of faces");
			*face = allocate_faces(*n_faces);
			exit_if_false(*face != NULL,"allocating the faces");
			for(i = 0; i < *n_faces; i ++) face_read_geometry(file, &(*face)[i], *node);
		}
		if(strncmp(line,"CELLS",5) == 0 || strncmp(line,"ELEMENTS",8) == 0)
		{
			exit_if_false(sscanf(&line[6],"%i",n_elements) == 1,"reading the number of elements");
			*element = allocate_elements(*n_elements);
			exit_if_false(*element != NULL,"allocating the elements");
			for(i = 0; i < *n_elements; i ++) element_read_geometry(file, &(*element)[i], *face);
		}
	}

	exit_if_false(*n_nodes > 0,"finding nodes");
	exit_if_false(*n_faces > 0,"finding faces");
	exit_if_false(*n_elements > 0,"finding elements");

	free(line);
}

//--------------------------------------------------------------//

void node_read_geometry(FILE *file, struct NODE *node)
{
	int info = fscanf(file,"%lf %lf\n",&(node->x[0]),&(node->x[1]));
	exit_if_false(info == 2, "reading a node's coordinates");
}

//--------------------------------------------------------------//

void face_read_geometry(FILE *file, struct FACE *face, struct NODE *node)
{
	int i;

	//temporary storage
	int *index, count, offset;
	char *line, *temp;
	index = (int *)malloc(MAX_FACE_N_NODES * sizeof(int));
	line = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	temp = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	exit_if_false(index != NULL && line != NULL && temp != NULL ,"allocating temporary storage");

	//read a line
	exit_if_false(fgets(line, MAX_STRING_LENGTH, file) != NULL, "reading a face line");

	//strip newlines and whitespace off the end of the line
	for(i = strlen(line)-1; i >= 0; i --) if(line[i] != ' ' && line[i] != '\n') break;
	line[i+1] = '\0';

	//sequentially read the integers on the line
	count = offset = 0;
	while(offset < strlen(line))
	{
		sscanf(&line[offset],"%s",temp);
		sscanf(temp,"%i",&index[count]);
		count ++;
		offset += strlen(temp) + 1;
		while(line[offset] == ' ') offset ++;
	}

	//number of faces
	face->n_nodes = count;

	//allocate the faces
	exit_if_false(face->node = allocate_face_node(face),"allocating face nodes");

	//node pointers
	for(i = 0; i < count; i ++) face->node[i] = &node[index[i]];

	//clean up
	free(index);
	free(line);
	free(temp);
}

//--------------------------------------------------------------//

void element_read_geometry(FILE *file, struct ELEMENT *element, struct FACE *face)
{
        int i;

        //temporary storage
        int *index, count, offset;
        char *line, *temp;
        index = (int *)malloc(MAX_ELEMENT_N_FACES * sizeof(int));
        line = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
        temp = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
        exit_if_false(index != NULL && line != NULL && temp != NULL ,"allocating temporary storage");

        //read the line
        exit_if_false(fgets(line, MAX_STRING_LENGTH, file) != NULL, "reading an element line");

        //eat up whitespace and newlines
        for(i = strlen(line)-1; i >= 0; i --) if(line[i] != ' ' && line[i] != '\n') break;
        line[i+1] = '\0';

        //sequentially read the integers on the line
        count = offset = 0;
        while(offset < strlen(line))
        {
                sscanf(&line[offset],"%s",temp);
                sscanf(temp,"%i",&index[count]);
                count ++;
                offset += strlen(temp) + 1;
                while(line[offset] == ' ') offset ++;
        }

        //number of faces
        element->n_faces = count;

        //allocate the faces
        exit_if_false(element->face = allocate_element_face(element),"allocating element faces");

        //face pointers
        for(i = 0; i < count; i ++) element->face[i] = &face[index[i]];

        //clean up
        free(index);
        free(line);
        free(temp);
}

//////////////////////////////////////////////////////////////////

void write_case(FILE *file, int n_variables, int *variable_order, int n_nodes, struct NODE *node, int n_faces, struct FACE *face, int n_elements, struct ELEMENT *element, int n_boundaries, struct BOUNDARY *boundary)
{
        int i;

        // number of variables and orders
        exit_if_false(fwrite(&n_variables, sizeof(int), 1, file) == 1, "writing the number of variables");
        exit_if_false(fwrite(variable_order, sizeof(int), n_variables, file) == n_variables, "writing variable orders");

	int max_variable_order = 0;
	for(i = 0; i < n_variables; i ++) max_variable_order = MAX(max_variable_order,variable_order[i]);
	int *n_basis = (int *)malloc(n_variables * sizeof(int));
	exit_if_false(n_basis != NULL,"allocating n_basis");
	for(i = 0; i < n_variables; i ++) n_basis[i] = ORDER_TO_N_BASIS(variable_order[i]);

        // numbers of structures
        exit_if_false(fwrite(&n_nodes, sizeof(int), 1, file) == 1, "writing the number of nodes");
        exit_if_false(fwrite(&n_faces, sizeof(int), 1, file) == 1, "writing the number of faces");
        exit_if_false(fwrite(&n_elements, sizeof(int), 1, file) == 1, "writing the number of elements");
        exit_if_false(fwrite(&n_boundaries, sizeof(int), 1, file) == 1, "writing the number of boundaries");

	int n_gauss = ORDER_TO_N_GAUSS(max_variable_order), n_hammer = ORDER_TO_N_HAMMER(max_variable_order);

        // structures
        for(i = 0; i < n_nodes; i ++) node_write_case(file,&node[i]);
        for(i = 0; i < n_faces; i ++) face_write_case(file,n_variables,n_basis,n_gauss,node,&face[i],element,boundary);
        for(i = 0; i < n_elements; i ++) element_write_case(file,n_variables,n_basis,n_gauss,n_hammer,face,&element[i]);
        for(i = 0; i < n_boundaries; i ++) boundary_write_case(file,face,&boundary[i]);

	free(n_basis);
}

//--------------------------------------------------------------//

void read_case(FILE *file, int *n_variables, int **variable_order, int *n_nodes, struct NODE **node, int *n_faces, struct FACE **face, int *n_elements, struct ELEMENT **element, int *n_boundaries, struct BOUNDARY **boundary)
{
        int i;

        // number of variables
        exit_if_false(fread(n_variables, sizeof(int), 1, file) == 1, "reading the number of variables");
	exit_if_false(*variable_order = (int *)realloc(*variable_order, *n_variables * sizeof(int)),"allocting variable orders");
        exit_if_false(fread(*variable_order, sizeof(int), *n_variables, file) == *n_variables, "reading variable orders");

	int max_variable_order = 0;
	for(i = 0; i < *n_variables; i ++) max_variable_order = MAX(max_variable_order,(*variable_order)[i]);
	int *n_basis = (int *)malloc(*n_variables * sizeof(int));
	exit_if_false(n_basis != NULL,"allocating n_basis");
	for(i = 0; i < *n_variables; i ++) n_basis[i] = ORDER_TO_N_BASIS((*variable_order)[i]);

        // numbers of structures
        exit_if_false(fread(n_nodes, sizeof(int), 1, file) == 1, "reading the number of nodes");
        exit_if_false(fread(n_faces, sizeof(int), 1, file) == 1, "reading the number of faces");
        exit_if_false(fread(n_elements, sizeof(int), 1, file) == 1, "reading the number of elements");
        exit_if_false(fread(n_boundaries, sizeof(int), 1, file) == 1, "reading the number of boundaries");

        exit_if_false((*node = allocate_nodes(*n_nodes)) != NULL,"allocating nodes");
        exit_if_false((*face = allocate_faces(*n_faces)) != NULL,"allocating faces");
        exit_if_false((*element = allocate_elements(*n_elements)) != NULL,"allocating elements");
        exit_if_false((*boundary = allocate_boundaries(*n_boundaries)) != NULL,"allocating boundaries");

	int n_gauss = ORDER_TO_N_GAUSS(max_variable_order), n_hammer = ORDER_TO_N_HAMMER(max_variable_order);

        // structures
        for(i = 0; i < *n_nodes; i ++) node_read_case(file,&(*node)[i]);
        for(i = 0; i < *n_faces; i ++) face_read_case(file,*n_variables,n_basis,n_gauss,*node,&(*face)[i],*element,*boundary);
        for(i = 0; i < *n_elements; i ++) element_read_case(file,*n_variables,n_basis,n_gauss,n_hammer,*face,&(*element)[i]);
        for(i = 0; i < *n_boundaries; i ++) boundary_read_case(file,*face,&(*boundary)[i]);

	free(n_basis);
}

//--------------------------------------------------------------//

void node_write_case(FILE *file, struct NODE *node)
{
	exit_if_false(fwrite(node->x, sizeof(double), 2, file) == 2, "writing the node location");
}

//--------------------------------------------------------------//

void node_read_case(FILE *file, struct NODE *node)
{
	exit_if_false(fread(node->x, sizeof(double), 2, file) == 2, "reading the node location");
}

//--------------------------------------------------------------//

void face_write_case(FILE *file, int n_variables, int *n_basis, int n_gauss, struct NODE *node, struct FACE *face, struct ELEMENT *element, struct BOUNDARY *boundary)
{
	int i, j, n;
		
	int *index = (int *)malloc(MAX_N_INDICES * sizeof(int));
	exit_if_false(index != NULL,"allocating temporary storage");

	exit_if_false(fwrite(&(face->n_nodes), sizeof(int), 1, file) == 1, "writing the number of face nodes");
	for(i = 0; i < face->n_nodes; i ++) index[i] = (int)(face->node[i] - &node[0]);
	exit_if_false(fwrite(index, sizeof(int), face->n_nodes, file) == face->n_nodes, "writing the face nodes");

	exit_if_false(fwrite(&(face->n_borders), sizeof(int), 1, file) == 1, "writing the number of face borders");
	for(i = 0; i < face->n_borders; i ++) index[i] = (int)(face->border[i] - &element[0]);
	exit_if_false(fwrite(index, sizeof(int), face->n_borders, file) == face->n_borders, "writing the face borders");

	exit_if_false(fwrite(face->n_boundaries, sizeof(int), n_variables, file) == n_variables,"writing the number of face boundaries");
	for(i = 0; i < n_variables; i ++)
	{
		if(face->n_boundaries[i])
		{
			for(j = 0; j < face->n_boundaries[i]; j ++) index[j] = (int)(face->boundary[i][j] - &boundary[0]);
			exit_if_false(fwrite(index, sizeof(int), face->n_boundaries[i], file) == face->n_boundaries[i],"writing face boundaries");
		}
	}

	exit_if_false(fwrite(face->normal, sizeof(double), 2, file) == 2, "writing the face normal");
	exit_if_false(fwrite(face->centre, sizeof(double), 2, file) == 2, "writing the face centre");
	exit_if_false(fwrite(&(face->size), sizeof(double), 1, file) == 1, "writing the face size");

	exit_if_false(fwrite(face->X[0], sizeof(double), 2*n_gauss, file) == 2*n_gauss,"writing the face integration locations");
	exit_if_false(fwrite(face->W, sizeof(double), n_gauss, file) == n_gauss,"writing the face integration weights");

	for(i = 0; i < n_variables; i ++)
	{
		n = n_basis[i]*(face->n_borders*n_basis[i] + face->n_boundaries[i])*n_gauss;
		exit_if_false(fwrite(face->Q[i][0][0], sizeof(double), n, file) == n,"writing face interpolation");
	}

	free(index);
}

//--------------------------------------------------------------//

void face_read_case(FILE *file, int n_variables, int *n_basis, int n_gauss, struct NODE *node, struct FACE *face, struct ELEMENT *element, struct BOUNDARY *boundary)
{
	int i, j, n;
		
	int *index = (int *)malloc(MAX_N_INDICES * sizeof(int));
	exit_if_false(index != NULL,"allocating temporary storage");

	exit_if_false(fread(&(face->n_nodes), sizeof(int), 1, file) == 1, "reading the number of face nodes");
	exit_if_false(face->node = allocate_face_node(face),"allocating face nodes");
	exit_if_false(fread(index, sizeof(int), face->n_nodes, file) == face->n_nodes, "reading the face nodes");
	for(i = 0; i < face->n_nodes; i ++) face->node[i] = &node[index[i]];

	exit_if_false(fread(&(face->n_borders), sizeof(int), 1, file) == 1, "reading the number of face borders");
	exit_if_false(face->border = allocate_face_border(face),"allocating face borders");
	exit_if_false(fread(index, sizeof(int), face->n_borders, file) == face->n_borders, "reading the face borders");
	for(i = 0; i < face->n_borders; i ++) face->border[i] = &element[index[i]];

	exit_if_false(face->n_boundaries = allocate_face_n_boundaries(face,n_variables),"allocating the face numbers of boundaries");
	exit_if_false(fread(face->n_boundaries, sizeof(int), n_variables, file) == n_variables,"reading the number of face boundaries");
	exit_if_false(face->boundary = allocate_face_boundary(face,n_variables),"allocating the face boundaries");
	for(i = 0; i < n_variables; i ++)
	{
		if(face->n_boundaries[i])
		{
			exit_if_false(fread(index, sizeof(int), face->n_boundaries[i], file) == face->n_boundaries[i],"reading face boundaries");
			for(j = 0; j < face->n_boundaries[i]; j ++) face->boundary[i][j] = &boundary[index[i]];
		}
	}

	exit_if_false(fread(face->normal, sizeof(double), 2, file) == 2, "reading the face normal");
	exit_if_false(fread(face->centre, sizeof(double), 2, file) == 2, "reading the face centre");
	exit_if_false(fread(&(face->size), sizeof(double), 1, file) == 1, "reading the face size");

	exit_if_false(face->X = allocate_face_x(face,n_gauss),"allocating face integration locations");
	exit_if_false(fread(face->X[0], sizeof(double), 2*n_gauss, file) == 2*n_gauss,"reading the face integration locations");
	exit_if_false(face->W = allocate_face_w(face,n_gauss),"allocating face integration weights");
	exit_if_false(fread(face->W, sizeof(double), n_gauss, file) == n_gauss,"reading the face integration weights");

	exit_if_false(face->Q = allocate_face_q(face, n_variables, n_basis, n_gauss),"allocating face interpolation");
	for(i = 0; i < n_variables; i ++)
	{
		n = n_basis[i]*(face->n_borders*n_basis[i] + face->n_boundaries[i])*n_gauss;
		exit_if_false(fread(face->Q[i][0][0], sizeof(double), n, file) == n,"reading face interpolation");
	}

	free(index);
}

//--------------------------------------------------------------//

void element_write_case(FILE *file, int n_variables, int *n_basis, int n_gauss, int n_hammer, struct FACE *face, struct ELEMENT *element)
{
	int i, n;

	int *index = (int *)malloc(MAX_N_INDICES * sizeof(int));
	exit_if_false(index != NULL,"allocating temporary storage");

	exit_if_false(fwrite(&(element->n_faces), sizeof(int), 1, file) == 1,"writing the number of element faces");
	for(i = 0; i < element->n_faces; i ++) index[i] = (int)(element->face[i] - &face[0]);
	exit_if_false(fwrite(index, sizeof(int), element->n_faces, file) == element->n_faces,"writing the element faces");

	exit_if_false(fwrite(element->centre, sizeof(double), 2, file) == 2,"writing the element centre");
	exit_if_false(fwrite(&(element->size), sizeof(double), 1, file) == 1,"writing the element size");

	for(i = 0; i < n_variables; i ++) exit_if_false(fwrite(element->unknown[i], sizeof(int), n_basis[i], file) == n_basis[i],"writing element unknowns");

	int n_points = (element->n_faces-2)*n_hammer;

	exit_if_false(fwrite(element->X[0], sizeof(double), 2*n_points, file) == 2*n_points,"writing the element integration locations");
	exit_if_false(fwrite(element->W, sizeof(double), n_points, file) == n_points,"writing the element integration weights");

	int max_n_basis = 0;
	for(i = 0; i < n_variables; i ++) max_n_basis = MAX(max_n_basis,n_basis[i]);
	
	n = max_n_basis*max_n_basis*n_points;
	exit_if_false(fwrite(element->P[0][0], sizeof(double), n, file) == n,"writing element interior interpolaton");
	n = element->n_faces*max_n_basis*n_gauss;
	exit_if_false(fwrite(element->Q[0][0], sizeof(double), n, file) == n,"writing element exterior interpolaton");

	for(i = 0; i < n_variables; i ++) 
	{
		n = n_points*n_basis[i];
		exit_if_false(fwrite(element->I[i][0], sizeof(double), n, file) == n,"writing element initialisation");
	}

	n = max_n_basis*element->n_faces;
	exit_if_false(fwrite(element->V[0], sizeof(double), n, file) == n,"writing element limit vertex interpolation");

	for(i = 0; i < n_variables; i ++) 
	{
		n = n_basis[i]*n_basis[i];
		exit_if_false(fwrite(element->L[i][0], sizeof(double), n, file) == n,"writing element limitng");
	}

	free(index);
}

//--------------------------------------------------------------//

void element_read_case(FILE *file, int n_variables, int *n_basis, int n_gauss, int n_hammer, struct FACE *face, struct ELEMENT *element)
{
	int i, n;

	int *index = (int *)malloc(MAX_N_INDICES * sizeof(int));
	exit_if_false(index != NULL,"allocating temporary storage");

	exit_if_false(fread(&(element->n_faces), sizeof(int), 1, file) == 1,"reading the number of element faces");
	exit_if_false(element->face = allocate_element_face(element),"allocating element faces");
	exit_if_false(fread(index, sizeof(int), element->n_faces, file) == element->n_faces,"reading the element faces");
	for(i = 0; i < element->n_faces; i ++) element->face[i] = &face[index[i]];

	exit_if_false(fread(element->centre, sizeof(double), 2, file) == 2,"reading the element centre");
	exit_if_false(fread(&(element->size), sizeof(double), 1, file) == 1,"reading the element size");

	exit_if_false(element->unknown = allocate_element_unknown(element,n_variables,n_basis),"allocating element unknowns");
	for(i = 0; i < n_variables; i ++) exit_if_false(fread(element->unknown[i], sizeof(int), n_basis[i], file) == n_basis[i],"reading element unknowns");

	int n_points = (element->n_faces-2)*n_hammer;

        exit_if_false(element->X = allocate_element_x(element,n_points),"allocating element integration locations");
	exit_if_false(fread(element->X[0], sizeof(double), 2*n_points, file) == 2*n_points,"reading the element integration locations");
        exit_if_false(element->W = allocate_element_w(element,n_points),"allocating element integration weights");
	exit_if_false(fread(element->W, sizeof(double), n_points, file) == n_points,"reading the element integration weights");

	int max_n_basis = 0;
	for(i = 0; i < n_variables; i ++) max_n_basis = MAX(max_n_basis,n_basis[i]);

	n = max_n_basis*max_n_basis*n_points;
	exit_if_false(element->P = allocate_element_p(element, max_n_basis, n_points),"allocating element interior interpolaton");
	exit_if_false(fread(element->P[0][0], sizeof(double), n, file) == n,"reading element interior interpolaton");
	n = element->n_faces*max_n_basis*n_gauss;
	exit_if_false(element->Q = allocate_element_q(element, max_n_basis, n_gauss),"allocating element exterior interpolaton");
	exit_if_false(fread(element->Q[0][0], sizeof(double), n, file) == n,"reading element exterior interpolaton");

	exit_if_false(element->I = allocate_element_i(element, n_variables, n_basis, n_points),"allocating element initialisation");
	for(i = 0; i < n_variables; i ++) 
	{
		n = n_points*n_basis[i];
		exit_if_false(fread(element->I[i][0], sizeof(double), n, file) == n,"reading element initialisation");
	}

	exit_if_false(element->V = allocate_element_v(element,max_n_basis),"allocating element vertex interpolaton");
	n = max_n_basis*element->n_faces;
	exit_if_false(fread(element->V[0], sizeof(double), n, file) == n,"writing element vertex interpolaton");

	exit_if_false(element->L = allocate_element_l(element, n_variables, n_basis),"allocating element limiting");
	for(i = 0; i < n_variables; i ++) 
	{
		n = n_basis[i]*n_basis[i];
		exit_if_false(fread(element->L[i][0], sizeof(double), n, file) == n,"reading element limiting");
	}

	free(index);
}

//--------------------------------------------------------------//

void boundary_write_case(FILE *file, struct FACE *face, struct BOUNDARY *boundary)
{
	int i;

	int *index = (int *)malloc(MAX_N_INDICES * sizeof(int));
	exit_if_false(index != NULL,"allocating temporary storage");

	exit_if_false(fwrite(&(boundary->n_faces), sizeof(int), 1, file) == 1,"writing the number of boundary faces");
	for(i = 0; i < boundary->n_faces; i ++) index[i] = (int)(boundary->face[i] - &face[0]);
	exit_if_false(fwrite(index, sizeof(int), boundary->n_faces, file) == boundary->n_faces,"writing the boundary faces");

	exit_if_false(fwrite(&(boundary->variable), sizeof(int), 1, file) == 1,"writing the boundary variable");
	exit_if_false(fwrite(boundary->condition, sizeof(int), 2, file) == 2,"writing the boundary condition");

	free(index);
}

//--------------------------------------------------------------//

void boundary_read_case(FILE *file, struct FACE *face, struct BOUNDARY *boundary)
{
	int i;

	int *index = (int *)malloc(MAX_N_INDICES * sizeof(int));
	exit_if_false(index != NULL,"allocating temporary storage");

	exit_if_false(fread(&(boundary->n_faces), sizeof(int), 1, file) == 1,"reading the number of boundary faces");
	exit_if_false(boundary->face = allocate_boundary_face(boundary),"allocating the boundary faces");
	exit_if_false(fread(index, sizeof(int), boundary->n_faces, file) == boundary->n_faces,"reading the boundary faces");
	for(i = 0; i < boundary->n_faces; i ++) boundary->face[i] = &face[index[i]];

	exit_if_false(fread(&(boundary->variable), sizeof(int), 1, file) == 1,"reading the boundary variable");
	exit_if_false(fread(boundary->condition, sizeof(int), 2, file) == 2,"reading the boundary condition");

	free(index);
}

//////////////////////////////////////////////////////////////////

void generate_numbered_file_path(char *file_path, char *base_path, int number)
{
	char *sub = strchr(base_path, '?');
	exit_if_false(sub != NULL,"finding substitute character \"?\" in %s",base_path);

	*sub = '\0';
	sprintf(file_path, "%s%09i%s", base_path, number, sub + 1);
	*sub = '?';
}

//////////////////////////////////////////////////////////////////

void write_data(FILE *file, int n_u, double *u, int number)
{
	exit_if_false(fwrite(&number, sizeof(int), 1, file) == 1,"writing the file number");
	exit_if_false(fwrite(&n_u, sizeof(int), 1, file) == 1,"writing the number of values");
	exit_if_false(fwrite(u, sizeof(double), n_u, file) == n_u,"writing the values");
}

//--------------------------------------------------------------//

void read_data(FILE *file, int *n_u, double **u, int *number)
{
	exit_if_false(fread(number, sizeof(int), 1, file) == 1,"reading the file number");
	exit_if_false(fread(n_u, sizeof(int), 1, file) == 1,"reading the number of values");
	exit_if_false(*u = (double *)realloc(*u, *n_u * sizeof(double)),"allocating the values");
	exit_if_false(fread(*u, sizeof(double), *n_u, file) == *n_u,"reading the values");
}

//////////////////////////////////////////////////////////////////

void boundaries_input(FILE *file, int n_faces, struct FACE *face, int *n_boundaries, struct BOUNDARY **boundary)
{
        // counters
	int i, j, n = 0, info;

	// fetch the data from the file
	FETCH fetch = fetch_new(BOUNDARY_FORMAT, MAX_N_BOUNDARIES);
	exit_if_false(fetch != NULL,"allocating boundary input");
	int n_fetch = fetch_read(file, BOUNDARY_LABEL, fetch);
	exit_if_false(n_fetch > 0,"finding boundaries");
	warn_if_false(n_fetch < MAX_N_BOUNDARIES,"maximum number of boundaries reached");

	// allocate boundaries
	struct BOUNDARY *b = allocate_boundaries(n_fetch);
	exit_if_false(b != NULL,"allocating boundaries");

	// temporary storage
	int offset, index[2];
	char *ind_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	char *val_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	char *cst_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	char *temp = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	exit_if_false(ind_string != NULL && val_string != NULL && cst_string != NULL && temp != NULL,"allocating temporary storage");

	// get the constants
	constants_input(file,cst_string);

	// consider each feteched line
	for(i = 0; i < n_fetch; i ++)
	{
		// initialise
		b[n].n_faces = MAX_BOUNDARY_N_FACES;
		b[n].face = allocate_boundary_face(&b[n]);
		exit_if_false(b->face != NULL,"allocating boundary faces");
		b[n].n_faces = 0;

		// get the indices
		fetch_get(fetch, i, 0, ind_string);

		// convert comma delimiters to whitespace
		for(j = 0; j < strlen(ind_string); j ++) if(ind_string[j] == ',') ind_string[j] = ' ';

		// read the ranges
		offset = info = 0;
		while(offset < strlen(ind_string))
		{
			//read the range from the string
			info = sscanf(&ind_string[offset],"%s",temp) == 1;
			info *= sscanf(temp,"%i:%i",&index[0],&index[1]) == 2;
			warn_if_false(info,"skipping boundary with unrecognised range");
			if(!info) break;

			// store boundary in the elements in the range
			for(j = index[0]; j <= index[1]; j ++) b[n].face[b[n].n_faces ++] = &face[j];

			// move to the next range in the string
			offset += strlen(temp) + 1;
		}
		if(!info) continue;

		// re-allocate
		b[n].face = allocate_boundary_face(&b[n]);
		exit_if_false(b[n].face != NULL,"re-allocating boundary faces");

		// get the variable
		fetch_get(fetch, i, 1, &b[n].variable);

		// get the condition
		fetch_get(fetch, i, 2, temp);
		for(j = 0; j < 2; j ++) b[n].condition[j] = 0;
		if(strcmp(temp,"d") != 0)
		{
			for(j = 0; j < strlen(temp); j ++)
			{
				if(temp[j] == 'n') b[n].condition[0] ++;
				else if(temp[j] == 't') b[n].condition[1] ++;
				else { info = 0; break; }
			}
			warn_if_false(info,"skipping boundary with unrecognised condition");
		}
		if(!info) continue;

		// get the value expression
		fetch_get(fetch, i, 3, val_string);
		sprintf(temp,"%s;%s",cst_string,val_string);
		info = add_geometry_to_expression_string(temp);
		info *= (b[n].value = expression_generate(temp)) != NULL;
		warn_if_false(info,"skipping boundary with unrecognised value expression");
		if(!info) continue;

		// increment boundary
		n ++;
	}

	// check numbers
	warn_if_false(n == n_fetch,"skipping boundaries with unrecognised formats");

	// copy pointers
	*boundary = b;
	*n_boundaries = n;

	// clean up
	fetch_destroy(fetch);
	free(ind_string);
	free(val_string);
	free(cst_string);
	free(temp);
}

//////////////////////////////////////////////////////////////////

void terms_input(FILE *file, int *n_terms, struct TERM **term)
{
	int i, j, n = 0, info;

	// fetch the data
	FETCH fetch = fetch_new(TERM_FORMAT,MAX_N_TERMS);
	exit_if_false(fetch != NULL,"allocating fetch");
	int n_fetch = fetch_read(file,TERM_LABEL,fetch);
	exit_if_false(n_fetch > 0,"finding terms");
	warn_if_false(n_fetch < MAX_N_TERMS,"maximum number of terms reached");

	// allocate pointers
	struct TERM *t = allocate_terms(n_fetch);
	exit_if_false(t != NULL,"allocating terms");

	// temporary storage
	char *cst_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	char *var_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	char *dif_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	char *mth_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	char *val_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	char *jac_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	char *temp = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	exit_if_false(temp != NULL && 
			cst_string != NULL && var_string != NULL && dif_string != NULL &&
			mth_string != NULL && val_string != NULL && jac_string != NULL,
			"allocating temporary strings");
	int n_var_string, n_dif_string, n_mth_string, n_jac_string;
	int var_offset, dif_offset, mth_offset, jac_offset;
	int dif[2];

	// get the constants
	constants_input(file,cst_string);

	// loop over fetched data
	for(i = 0; i < n_fetch; i ++)
	{
		// initial allocation
		t[n].n_variables = MAX_TERM_N_VARIABLES;
		exit_if_false(t[n].variable = allocate_term_variable(&t[n]),"allocating term variables");
		exit_if_false(t[n].differential = allocate_term_differential(&t[n]),"allocating term differentials");
		exit_if_false(t[n].method = allocate_term_method(&t[n]),"allocating term methods");
		exit_if_false(t[n].weight = allocate_term_weight(&t[n]),"allocating term weights");
		exit_if_false(t[n].jacobian = allocate_term_jacobian(&t[n]),"allocating term jacobians");
		t[n].n_variables = 0;

		// equation
		fetch_get(fetch, i, 0, &t[n].equation);

		// type
		fetch_get(fetch, i, 1, &t[n].type);
		info = t[n].type == 's' || t[n].type == 'x' || t[n].type == 'y';
		warn_if_false(info,"skipping term with unrecognised type");
		if(!info) continue;

		// implicit fraction
		fetch_get(fetch, i, 2, &t[n].implicit);

		// value expression string
		fetch_get(fetch, i, 6, val_string);
		sprintf(temp,"%s;%s",cst_string,val_string);
		info = add_geometry_to_expression_string(temp);
		info *= (t[n].residual = expression_generate(temp)) != NULL;
		warn_if_false(info,"skipping term with unrecognised residual expression");
		if(!info) continue;

		// get the variable, differential, method and jacobian expression strings
		fetch_get(fetch, i, 3, var_string); n_var_string = strlen(var_string);
		fetch_get(fetch, i, 4, dif_string); n_dif_string = strlen(dif_string);
		fetch_get(fetch, i, 5, mth_string); n_mth_string = strlen(mth_string);
		fetch_get(fetch, i, 7, jac_string); n_jac_string = strlen(jac_string);
		for(j = 0; j < n_var_string; j ++) if(var_string[j] == ',') var_string[j] = '\0';
		for(j = 0; j < n_dif_string; j ++) if(dif_string[j] == ',') dif_string[j] = '\0';
		for(j = 0; j < n_mth_string; j ++) if(mth_string[j] == ',') mth_string[j] = '\0';
		for(j = 0; j < n_jac_string; j ++) if(jac_string[j] == ',') jac_string[j] = '\0';

		// read each variable in turn
		var_offset = dif_offset = mth_offset = jac_offset = t[n].n_variables = 0;
		while(var_offset < n_var_string)
		{
			info = 1;

			// read the variable indices
			info *= sscanf(&var_string[var_offset],"%i",&t[n].variable[t[n].n_variables]) == 1;
			var_offset += strlen(&var_string[var_offset]) + 1;
			warn_if_false(info,"skipping term with unrecognised variable index");
			if(!info) break;

			// read the x and y differentials and convert to a differential index
			info *= sscanf(&dif_string[dif_offset],"%s",temp) == 1;
			j = dif[0] = dif[1] = 0;
			while(info && temp[j] != '\0')
			{
				dif[0] += (temp[j] == 'x');
				dif[1] += (temp[j] == 'y');
				j ++;
			}
			t[n].differential[t[n].n_variables] = powers_taylor[dif[0]][dif[1]];
			dif_offset += strlen(&dif_string[dif_offset]) + 1;
			warn_if_false(info,"skipping term with unrecognised differential");
			if(!info) break;

			// read the methods
			info *= sscanf(&mth_string[mth_offset],"%c",&t[n].method[t[n].n_variables]) == 1;
			if(t[n].method[t[n].n_variables] == 'w')
			{
				info *= sprintf(temp,"%s;%s",cst_string,&mth_string[mth_offset+1]) > 0;
				info *= add_geometry_to_expression_string(temp);
				info *= (t[n].weight[t[n].n_variables] = expression_generate(temp)) != NULL;
			}
			mth_offset += strlen(&mth_string[mth_offset]) + 1;
			warn_if_false(info,"skipping term with unrecognised method");
			if(!info) break;

			// read the jacobian expressions
			info *= sprintf(temp,"%s;%s",cst_string,&jac_string[jac_offset]) > 0;
			info *= add_geometry_to_expression_string(temp);
			info *= (t[n].jacobian[t[n].n_variables] = expression_generate(temp)) != NULL;
			jac_offset += strlen(&jac_string[jac_offset]) + 1;
			warn_if_false(info,"skipping term with unrecognised jacobian");
			if(!info) break;

			// next variable
			if(info) t[n].n_variables ++;
		}
		if(!info) continue;

		// re-allocate
		exit_if_false(t[n].variable = allocate_term_variable(&t[n]),"re-allocating term variables");
		exit_if_false(t[n].differential = allocate_term_differential(&t[n]),"re-allocating term differentials");
		exit_if_false(t[n].method = allocate_term_method(&t[n]),"re-allocating term methods");
		exit_if_false(t[n].jacobian = allocate_term_jacobian(&t[n]),"re-allocating term jacobians");

		// increment the number of terms
		n ++;
	}

	// check numbers
	warn_if_false(n_fetch == n,"skipping terms with unrecognised formats");

	// copy pointers
	*term = t;
	*n_terms = n;

	// clean up
	fetch_destroy(fetch);
	free(cst_string);
	free(var_string);
	free(dif_string);
	free(mth_string);
	free(val_string);
	free(jac_string);
	free(temp);
}

//////////////////////////////////////////////////////////////////

void constants_input(FILE *file, char *constants)
{
	int i, n = 0;

	// initialise
	constants[0] = '\0';

	// fetch the constants
	FETCH fetch = fetch_new(CONSTANT_FORMAT,MAX_N_CONSTANTS);
	exit_if_false(fetch != NULL,"allocating fetch constants");
	int n_fetch = fetch_read(file,CONSTANT_LABEL,fetch);

	// concatenate the constants onto one semicolon-delimited string
	for(i = 0; i < n_fetch; i ++)
	{
		fetch_get(fetch, i, 0, &constants[n]);
		n = strlen(constants);
		constants[n++] = ';';
	}

	// terminate the string
	if(n) constants[--n] = '\0';

	// clean up
	fetch_destroy(fetch);
}

//////////////////////////////////////////////////////////////////

int add_geometry_to_expression_string(char *string)
{
	int i, j, s;
	char *label, location_labels[] = EXPRESSION_LOCATION_LABELS, normal_labels[] = EXPRESSION_NORMAL_LABELS;
	char *temp = (char *)malloc(MAX_STRING_LENGTH * sizeof(char));
	exit_if_false(temp != NULL,"allocating temporary string");

	for(i = 0; i < strlen(string); i ++)
	{
		if(string[i] == '$')
		{
			if(sscanf(&string[i+1],"%i",&s) == 1)
			{
				s += EXPRESSION_VARIABLE_INDEX;
				j = i + 1;
				while('0' <= string[j] && string[j] <= '9') j ++;
			}
			else if((label = strchr(location_labels,string[i+1])) != NULL)
			{
				s = (int)(label - location_labels) + EXPRESSION_LOCATION_INDEX;
				j = i + 2;
			}
			else if((label = strchr(normal_labels,string[i+1])) != NULL)
			{
				s = (int)(label - normal_labels) + EXPRESSION_NORMAL_INDEX;
				j = i + 2;
			}
			else return 0;

			sprintf(temp,"$%i%s",s,&string[j]);
			strcpy(&string[i],temp);
		}
	}

	free(temp);

	return 1;
}

//////////////////////////////////////////////////////////////////

int initial_input(FILE *file, int n_variables, EXPRESSION **initial)
{
	int v;

	char *temp, *constant_string, **initial_string;
	exit_if_false(temp = (char *)malloc(MAX_STRING_LENGTH * sizeof(char)),"allocating the temporary string");
	exit_if_false(constant_string = (char *)malloc(MAX_STRING_LENGTH * sizeof(char)),"allocating the constant string");
	exit_if_false(initial_string = allocate_character_matrix(NULL,n_variables,MAX_STRING_LENGTH),"allocating initial strings");

	constants_input(file,constant_string);

	if(fetch_vector(file,"variable_initial_value",'s',n_variables,initial_string) != FETCH_SUCCESS) return 0;

	exit_if_false(*initial = allocate_initial(n_variables),"allocating initial expressions");

	for(v = 0; v < n_variables; v ++)
	{
		sprintf(temp,"%s;%s",constant_string,initial_string[v]);
		add_geometry_to_expression_string(temp);
		exit_if_false((*initial)[v] = expression_generate(temp),"generating initial expression");
	}

	free(temp);
	free(constant_string);
	destroy_matrix((void *)initial_string);

	return 1;
}

//////////////////////////////////////////////////////////////////

void write_display(FILE *file, int n_variables, char **variable_name, int *variable_order, int n_elements, struct ELEMENT *element, int n_u, double *u)
{
	int e, f, i, j, n, v;

	char trans[2] = "NT";
	int int_0 = 0, int_1 = 1;
	double dbl_0 = 0, dbl_1 = 1;

	int max_variable_order = 0;
	for(v = 0; v < n_variables; v ++) max_variable_order = MAX(max_variable_order,variable_order[v]);

	int *n_basis, max_n_basis = ORDER_TO_N_BASIS(max_variable_order);
	exit_if_false(n_basis = (int *)malloc(n_variables * sizeof(int)),"allocating numbers of basis functions");
	for(v = 0; v < n_variables; v ++) n_basis[v] = ORDER_TO_N_BASIS(variable_order[v]);

	int n_hammer = ORDER_TO_N_HAMMER(max_variable_order), n_points;

	double *basis_value, *point_value;
	exit_if_false(basis_value = (double *)malloc(max_n_basis * sizeof(double)),"allocating basis values");
	exit_if_false(point_value = (double *)malloc(n_hammer * (MAX_ELEMENT_N_FACES - 2) * sizeof(double)),"allocating point values");

	int n_nodes = 0;
	for(e = 0; e < n_elements; e ++) n_nodes += element[e].n_faces;

	// header
	fprintf(file,"<VTKFile type=\"UnstructuredGrid\" version=\"0.1\" byte_order=\"LittleEndian\">\n");
	fprintf(file,"<UnstructuredGrid>\n");
	fprintf(file,"<Piece NumberOfPoints=\"%i\" NumberOfCells=\"%i\">\n",n_nodes,n_elements);

	// point data
	fprintf(file,"<PointData>\n");
	for(v = 0; v < n_variables; v ++)
	{
		fprintf(file,"<DataArray type=\"Float64\" Name=\"%s\" format=\"ascii\">\n",variable_name[v]);
		for(e = 0; e < n_elements; e ++)
		{
			for(i = 0; i < n_basis[v]; i ++) basis_value[i] = u[element[e].unknown[v][i]];

			dgemv_(&trans[0],&element[e].n_faces,&n_basis[v],
					&dbl_1,
					element[e].V[0],&element[e].n_faces,
					basis_value,&int_1,
					&dbl_0,
					point_value,&int_1);

			for(i = 0; i < element[e].n_faces; i ++) fprintf(file,"%.10e ",point_value[i]);
		}
		fprintf(file,"\n</DataArray>\n");
	}
	fprintf(file,"</PointData>\n");

	// cell averaged data
	fprintf(file,"<CellData>\n");
	for(v = 0; v < n_variables; v ++)
	{
		fprintf(file,"<DataArray type=\"Float64\" Name=\"%s\" format=\"ascii\">\n",variable_name[v]);
		for(e = 0; e < n_elements; e ++)
		{
			n_points = n_hammer * (element[e].n_faces - 2);

			for(i = 0; i < n_basis[v]; i ++) basis_value[i] = u[element[e].unknown[v][i]];

			dgemv_(&trans[0],&n_points,&n_basis[v],
					&dbl_1,
					element[e].P[powers_taylor[0][0]][0],&n_points,
					basis_value,&int_1,
					&dbl_0,
					point_value,&int_1);

			fprintf(file,"%.10e ",ddot_(&n_points,point_value,&int_1,element[e].W,&int_1) / ddot_(&n_points,&dbl_1,&int_0,element[e].W,&int_1));
		}
		fprintf(file,"\n</DataArray>\n");
	}
	fprintf(file,"</CellData>\n");

	// output all the nodes
	fprintf(file,"<Points>\n<DataArray type=\"Float64\" Name=\"Points\" NumberOfComponents=\"3\" format=\"ascii\">\n");
	for(e = 0; e < n_elements; e ++)
	{
		for(i = 0; i < element[e].n_faces; i ++)
		{
			for(j = 0; j < 2; j ++)
			{
				fprintf(file,"%.10e ",element[e].face[i]->node[element[e].face[i]->border[0] != &element[e]]->x[j]);
			}
			fprintf(file,"0.0 ");
		}
	}
	fprintf(file,"\n</DataArray>\n</Points>\n");

	// output the element polygons
	fprintf(file,"<Cells>\n");
	fprintf(file,"<DataArray type=\"Int32\" Name=\"connectivity\" format=\"ascii\">\n");
	n = 0;
	for(e = 0; e < n_elements; e ++)
	{
		f = 0;

		for(i = 0; i < element[e].n_faces; i ++)
		{
			fprintf(file,"%i ",n+f);

			for(j = 0; j < element[e].n_faces; j ++)
			{
				if(j == i) continue;
				if(
						element[e].face[j]->node[element[e].face[j]->border[0] != &element[e]] ==
						element[e].face[i]->node[element[e].face[i]->border[0] == &element[e]]
				  )
				{
					f = j;
					break;
				}
			}

			exit_if_false(j < element[e].n_faces,"finding the next vertex");
		}

		n += element[e].n_faces;
	}
	fprintf(file,"\n</DataArray>\n");
	
	// output the element offsets
	fprintf(file,"<DataArray type=\"Int32\" Name=\"offsets\" format=\"ascii\">\n");
	n = 0;
	for(e = 0; e < n_elements; e ++) fprintf(file,"%i ",n += element[e].n_faces);
	fprintf(file,"\n</DataArray>\n");

	// output the element types
	fprintf(file,"<DataArray type=\"Int32\" Name=\"types\" format=\"ascii\">\n");
	for(e = 0; e < n_elements; e ++) fprintf(file,"7 ");
	fprintf(file,"\n</DataArray>\n");

	// close the xml
	fprintf(file,"</Cells>\n</Piece>\n</UnstructuredGrid>\n</VTKFile>");

	// clean up
	free(n_basis);
	free(basis_value);
	free(point_value);
}

//////////////////////////////////////////////////////////////////
