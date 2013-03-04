// classic.cpp : "Textbook" implementation of matrix multiply

// Author:  Paul J. Drongowski
// Address: Boston Design Center
//          Advanced Micro Devices, Inc.
//          Boxborough, MA 01719
// Date:    28 November 2007
//
// Copyright (c) 2005-2007 Advanced Micro Devices, Inc.

// The purpose of this program is to demonstrate measurement
// and analysis of program performance using AMD CodeAnalyst(tm).
// All engineers are familiar with simple matrix multiplication,
// so this example should be easy to understand.
//
// This implementation of matrix multiplication is a direct
// translation of the "classic" textbook formula for matrix multiply.
// Performance of the classic implementation is affected by an
// inefficient data access pattern, which we should be able to
// identify using CodeAnalyst(TM).

#include <cstdlib>
#include <cstdio>
#include <ctime>
#include <unistd.h>

static const int ROWS = 1000 ;     // Number of rows in each matrix
static const int COLUMNS = 1000 ;  // Number of columns in each matrix

#include "classic2.h"

float matrix_a[ROWS][COLUMNS] ;    // Left matrix operand
float matrix_b[ROWS][COLUMNS] ;    // Right matrix operand
float matrix_r[ROWS][COLUMNS] ;    // Matrix result

FILE *result_file ;

void initialize_matrices()
{
    // Define initial contents of the matrices
    for (int i = 0 ; i < ROWS ; i++) {
        for (int j = 0 ; j < COLUMNS ; j++) {
            matrix_a[i][j] = (float) rand() / RAND_MAX ;
	        matrix_b[i][j] = (float) rand() / RAND_MAX ;
		matrix_r[i][j] = 0.0 ;
	}
    }
}

void print_result()
{
    // Print the result matrix
    for (int i = 0 ; i < ROWS ; i++) {
	for (int j = 0 ; j < COLUMNS ; j++) {
	    fprintf(result_file, "%6.4f ", matrix_r[i][j]) ;
	}
    fprintf(result_file, "\n") ;
    }
}


void print_elapsed_time()
{
    double elapsed ;
    double resolution ;

    // Obtain and display elapsed execution time
    elapsed = (double) clock() / CLOCKS_PER_SEC ;
    resolution = 1.0 / CLOCKS_PER_SEC ;

    fprintf(result_file, 
	    "Elapsed time: %8.4f sec (%6.4f sec resolution)\n",
	    elapsed, resolution) ;
}

int main(int argc, char* argv[])
{
    pid_t my_pid ;

    if ((result_file = fopen("classic.txt", "w")) == NULL) {
	fprintf(stderr, "Couldn't open result file\n") ;
	perror("classic") ;
	return( EXIT_FAILURE ) ;
    }

    fprintf(result_file, "Classic matrix multiplication\n") ;

    my_pid = getpid() ;
    fprintf(result_file, "Process ID:   %d\n", my_pid) ;

    initialize_matrices() ;
    inlined_multiply_matrices(matrix_a,matrix_b,matrix_r) ;
    print_elapsed_time() ;

    fclose(result_file) ;

    return( 0 ) ;
}
