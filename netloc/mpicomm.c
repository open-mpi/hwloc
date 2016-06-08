/*
 * Copyright © 2016 Inria.  All rights reserved.
 *
 * $COPYRIGHT$
 *
 * Additional copyrights may follow
 * See COPYING in top-level directory.
 *
 * $HEADER$
 */


#define _GNU_SOURCE         /* See feature_test_macros(7) */
#include <stdio.h>
#include <stdlib.h>

#include <netloc.h>
#include <private/netloc.h>

int netloc_build_comm_mat(char *filename, int *pn, double ***pmat, double **psum_row)
{
    FILE *input = fopen(filename, "r");

    if (!input ) {
        perror("fopen");
        return NETLOC_ERROR;
    }
    char *line = NULL;
    size_t linesize = 0;

    char *ptr= NULL;
    int i,j;

    j = -1;
    i = 0;

    /* Get the number of elements in a line to find the size of the matrix */
    netloc_get_line(&line, &linesize, input);
    int n = 0;
    while ((ptr = netloc_line_get_next_token(&line, ' '))) {
        if (!strlen(ptr))
            break;
        n++;
    }
    rewind(input);


    double *mat_values = (double *)malloc(n*n*sizeof(double));
    double **mat = (double **)malloc(n*sizeof(double));
    for (int i = 0; i < n; i++) {
        mat[i] = &mat_values[i*n];
    }
    double *sum_row = (double *)malloc(n*sizeof(double));

    while (netloc_get_line(&line, &linesize, input) != -1) {
        char *remain_line = line;
        j = 0;
        sum_row[i] = 0;
        while ((ptr = netloc_line_get_next_token(&remain_line, ' '))){
            if (!strlen(ptr))
                break;
            mat[i][j] = atof(ptr);
            sum_row[i] += mat [i][j];
            if (mat[i][j] < 0) {
                fprintf(stderr, "Warning: negative value in comm matrix "
                        "(mat[%d][%d] = %f)\n", i, j, mat[i][j]);
            }
            j++;
        }
        if (j != n) {
            fprintf(stderr, "Error at %d %d (%d!=%d). "
                    "Too many columns for %s\n", i, j, j, n, filename);
            goto error;
        }
        i++;
    }


    if (i != n) {
        fprintf(stderr,"Error at %d %d. Too many rows for %s\n",i,j,filename);
        goto error;
    }

    fclose (input);

    *pn = n;
    *pmat = mat;
    *psum_row = sum_row;

    return NETLOC_SUCCESS;

error:
    free(mat_values);
    free(mat);
    free(sum_row);
    *pmat = NULL;
    *psum_row = NULL;
    *pn = 0;
    fclose (input);
    return NETLOC_ERROR;
}
