// File in inc_ml, created by Thien Le in July 2018


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <omp.h>

#include "dist.h"
#include "utilities.h"
#include "options.h"
#include "tools.h"

// omp
// omp_set_num_threads(4);e


// Internal functions to compute distances
int countStateChanges (char *s, char *t, int length, char c1, char c2, int *filter);
int seqCharMatches (char *s, int length, char c, int *filter);
int matrixCharMatches (char **s, int numSeqs, int length, char c, int *filter);
int factorial (int n);
int *nextPerm (int *p, int index, int size, int length);
double permDiagProduct (double P[4][4], int *p, int d);
int *initPerm (int size);
double det (double P[4][4], int d);
int count_selected(int ** gapFilterWarn, int numSites, char ** data);
double compute_logdet_distance (char ** data, int numSites);
double compute_jc_distance (char ** data, int numSites);

// Internal functions
int read_phylip(INC_GRP * meta, MAP_GRP * map, char * filename);

/* Initialization matrix-related fields (number of taxon, etc)
 * Input:       meta        meta-variables including the distance matrix
 *              map         maps, including a naming map (since the taxa are digitized later)
 *              options     options set by users, including input file
 * Output:      0 on success, ERROR otherwise
 * Effect:      open a file, try to mallocate some structure
 */
int parse_distance_matrix(INC_GRP * meta, MAP_GRP * map, ml_options * options){
    FILE * f;
    // int n;
    char buf[1000050];
    STR_CLR(buf);

    // if(meta->master_ml_options->use_distance_matrix){
        f = fopen(options->init_d_name, "r");
        if(!f)                          PRINT_AND_RETURN("cannot open input file in parse_distance_matrix", OPEN_ERROR);

        // Read the first word of the file to see whether it is a FASTA file or a PHYLIP distance matrix
        if(fscanf(f, "%s", buf) < 0)    PRINT_AND_RETURN("empty input file", GENERAL_ERROR); 
        fclose(f);

        return read_phylip(meta, map, options->init_d_name);
    // } else {
        // Initialize other fields wihtout the distance matrix

    //     f = fopen(options->input_name);

    //     n = 0;
    //     while(fscanf(f, "%s", buf) >= 0){
    //         n++;
    //     }
    //     n /= 2;
    //     fclose(f);


    //     meta->n_taxa    = n;
    //     map->n_taxa     = n; 
    //     map->master_to_name         = malloc(n * sizeof(char*));
    //     meta->aln       = malloc(n * sizeof(char*));
    //     if(!map->master_to_name)        PRINT_AND_RETURN("malloc failed to allocate name map in read_phylip", MALLOC_ERROR);
    //     if(!meta->aln)                  PRINT_AND_RETURN("malloc failed to allocate aln in read_phylip". MALLOC_ERROR);

    //     f = fopen(options->input_name);
    //     for(i = 0; i < n; i++){
    //         // Allocation
    //         map->master_to_name[i]  = malloc(MAX_NAME_SIZE * sizeof(char));
            

    //         if(!map->master_to_name[i]) PRINT_AND_RETURN("malloc failed to allocate name map in read_phylip", MALLOC_ERROR);

    //         // Initialization
    //         fscanf(f, ">%s", map->master_to_name[i]);
    //         fscanf(f, "%s", buf);

    //         meta->aln[i] = malloc((strlen(buf) + 10) * sizeof(char)); 
    //         if(!map->aln[i]) PRINT_AND_RETURN("malloc failed to allocate aln in read_phylip", MALLOC_ERROR);
    //         strcpy(map->aln[i], buf);
    //     }
    //     fclose(f);
    // }
}

// INTERNAL IMPLEMENTATION
/* Reading a phylip distance matrix files and initialize meta variables
 * Input:       meta        meta-variables including the distance matrix
 *              map         maps, including a naming map (since the taxa are digitized later)
 *              filename    name of the phylip distance matrix file
 * Output:      0 on success, ERROR otherwise
 * Effect:      open a file, try to mallocate some structure, modify meta and map
 */
int read_phylip(INC_GRP * meta, MAP_GRP * map, char * filename){
    FILE * f;   // file object
    int n = 0;      // number of sequence
    int i, j;   // loop counters
    // float min;

    printf("%s\n", filename);
    // Open file
    f = fopen(filename, "r");
    if(!f)                          PRINT_AND_RETURN("cannot open input file in parse_distance_matrix", OPEN_ERROR);

    // Record number of sequence
    if(fscanf(f, "%d", &n) < 0)         PRINT_AND_RETURN("input phylip file is empty", GENERAL_ERROR);
    meta->n_taxa    = n;
    map->n_taxa     = n; 
                                                                                            #if DEBUG 
                                                                                                printf("debug: number of taxa is %d %d (these 2 should be the same)\n", meta->n_taxa, map->n_taxa); 
                                                                                            #endif
    // Mallocation sequence
    map->master_to_name         = malloc(n * sizeof(char*));
    meta->d                     = malloc(n * sizeof(float *));

    if(!map->master_to_name)        PRINT_AND_RETURN("malloc failed to allocate name map in read_phylip", MALLOC_ERROR);
    if(!meta->d)                    PRINT_AND_RETURN("malloc failed to allocate distance matrix in read_phylip", MALLOC_ERROR);


    for(i = 0; i < n; i++){
        // Allocation
        map->master_to_name[i]  = malloc(MAX_NAME_SIZE * sizeof(char));
        meta->d[i]              = malloc(n * sizeof(float));

        if(!map->master_to_name[i]) PRINT_AND_RETURN("malloc failed to allocate name map in read_phylip", MALLOC_ERROR);
        if(!meta->d[i])             PRINT_AND_RETURN("malloc failed to allocate distance matrix in read_phylip", MALLOC_ERROR);

        // Initialization
        fscanf(f, "%s", map->master_to_name[i]);
        for(j = 0; j < n; j++){
            fscanf(f, "%f", &(meta->d[i][j]));
        }
    }
    fclose(f);

    // Corrected 2kp (for 0 entries)
    // min = 1.0 * INT_MAX; 
    // for(i = 0; i < meta->n_taxa; i++)
    //     for(j = 0; j < meta->n_taxa; j++)
    //         if(min - meta->d[i][j] > EPS && meta->d[i][j] > EPS)
    //             min = meta->d[i][j];

    // min /= 2;
    // for(i = 0; i < meta->n_taxa; i++)
    //     for(j = 0; j < meta->n_taxa; j++)
    //         if(meta->d[i][j] < EPS && i != j)
    //             meta->d[i][j] = min;


                                                                                                #if LARGE_DEBUG 
                                                                                                    printf("debug: this prints out the list of name\n");
                                                                                                    for(i = 0; i < meta->n_taxa; i++)
                                                                                                        printf("%d:%s ", i, map->master_to_name[i]); 
                                                                                                    printf("\n");
                                                                                                #endif 
                                                                                                #if DEBUG
                                                                                                    printf("debug: this tests whether all the distances are positive\n");
                                                                                                    int k;
                                                                                                    k = 0;
                                                                                                    for(i = 0; i < meta->n_taxa; i++)
                                                                                                        for(j = 0; j < meta->n_taxa; j++)
                                                                                                            if(meta->d[i][j] > 1e-10)
                                                                                                                k++;
                                                                                                            else if(i != j) printf("debug: problem is %f, at %d %d, name is %s %s\n", meta->d[i][j], i, j, map->master_to_name[i], map->master_to_name[j]);
                                                                                                    printf("debug: double check, num sequence is %d\n", meta->n_taxa);
                                                                                                    printf("debug: there are %d positive entries\n", k);
                                                                                                    // for(i = 0; i < meta->n_taxa; i++){
                                                                                                    //     for(j = 0; j < meta->n_taxa; j++){
                                                                                                    //         printf("%f ", meta->d[i][j]);
                                                                                                    //     }
                                                                                                    //     printf("\n");
                                                                                                    // }
                                                                                                    // while(1);
                                                                                                    // printf("1 45 %f 1 57 %f 1 27 %f 45 57 %f 45 27 %f 57 27 %f\n", meta->d[0][44], meta->d[0][56], meta->d[0][26], meta->d[44][56], meta->d[44][26],meta->d[56][26]);
                                                                                                #endif
    return 0;
}

double dist_from_msa(msa_t * msa, DIST_MOD distance_model, int i, int j, double correction){
    double tmp_dist;
    char * tmp_dist_data[2];

    tmp_dist_data[0] = malloc(msa->N);
    tmp_dist_data[1] = malloc(msa->N);

    strcpy(tmp_dist_data[0], msa->msa[i]);
    strcpy(tmp_dist_data[1], msa->msa[j]);

    tmp_dist = distance_model == D_JC ? compute_jc_distance(tmp_dist_data, msa->N) : compute_logdet_distance(tmp_dist_data, msa->N); 
    tmp_dist = tmp_dist >= 0.0 ? tmp_dist : correction;

    return tmp_dist;
}


// This part is modified from FastME
int countStateChanges (char *s, char *t, int length, char c1, char c2,
    int *filter)
{
    int i;
    int matches = 0;

    #pragma omp parallel for private(i)
    for (i=0; i<length; i++){

        if ((c1 == s[i]) && (c2 == t[i]))
            matches += filter[i];
    }

    return (matches);
}

int seqCharMatches (char *s, int length, char c, int *filter)
{
    int i;
    int matches = 0;

    #pragma omp parallel for private(i)
    for (i=0; i<length; i++){
        // printf("wadw%d %d\n", omp_get_num_threads(), length);
        if (c == s[i])
            matches += filter[i];
    }

    return (matches);
}

/* called when calculating stationary probabilities */
int matrixCharMatches (char **s, int numSeqs, int length, char c, int *filter)
{
    int i;
    int matches = 0;
    // printf("wda\n");

    #pragma omp parallel for private(i)
    for (i=0; i<numSeqs; i++){
        // printf("wadw%d\n", omp_get_num_threads());
        // printf("i is %d, %s, %d, %d, %d\n", i, s[i], numSeqs, length, filter[0]);
        matches += seqCharMatches (s[i], length, c, filter);
    }
    // printf("wda\n");

    return (matches);
}

int factorial (int n)
{
    if (1 == n)
        return (n);

    else
        return (n * factorial (n-1));
}

int *nextPerm (int *p, int index, int size, int length)
{
    int temp;
    if (0 ==  index % factorial (size))
    {
        temp = p[length-1];
        p[length-1] = p[length-1-size];
        p[length-1-size] = temp;
        return (p);
    }
    else
        return (nextPerm (p, index, size-1, length));
}

double permDiagProduct (double P[4][4], int *p, int d)
{
    int i;
    double prod = 1.0;

    for (i=0; i<d; i++){
        // printf("%d %d %d %lf\n", i, p[i], d, P[0][0]);
        prod = prod * P[i][p[i]];
    }

    return (prod);
}

int *initPerm (int size)
{
    int *p;
    int i;
    // printf("%d\n", size);
    p = (int *) malloc (size * sizeof (int));

    for (i=0; i<size; i++)
        p[i] = i;

    return (p);
}


double det (double P[4][4], int d)
{
    int *p;
    int signum = 1;
    int i, numPerms;
    double det = 0;

    p = initPerm (d);
    numPerms = factorial (d);

    for (i=0; i<numPerms; i++)
    {
        // printf("%d\n", i);
        det += signum * permDiagProduct (P, p, d);
        p = nextPerm (p, i+1, d-1, d);
        signum = -1 * signum;
    }

    free (p);

    return(det);
}

int count_selected(int ** gapFilterWarn, int numSites, char ** data){ 
    int count = 0;
    int i, j;

    *gapFilterWarn = (int *) malloc(numSites * sizeof (int));

    #pragma omp parallel for private(i)
    for (i=0; i<numSites; i++)
        (*gapFilterWarn)[i] = 1;
                // printf("ưdaw %d\n", numSites);
// printf("wadw%d\n", omp_get_num_threads());

    #pragma omp parallel for private(i, j)
    for (i=0; i<numSites; i++)
        for (j=0; j<2; j++){
            // printf("wadw%d %d\n", omp_get_num_threads(), i);
            if (('*' == data[j][i]) || ('?' == data[j][i]) ||
                    ('-' == data[j][i]))
                (*gapFilterWarn)[i] = 0;
        }
    // while(1);

    #pragma omp parallel for private(i)
    for (i=0; i<numSites; i++){
        // printf("wadw%d %d\n", omp_get_num_threads(), i);
        if ((*gapFilterWarn)[i])
            count++;
    }

    return count;
}

double compute_logdet_distance (char ** data, int numSites){
    // printf("%d\n",omp_get_thread_num());
    int i, j;
    int * gapFilterWarn;
    int count; 

    int alphabetSize = 4;
    char alphabet[] = "ATCG";

    double Pi2[2][alphabetSize];
    double P[alphabetSize][alphabetSize];
    double D, detP;

    count = count_selected(&gapFilterWarn, numSites, data);
    // printf("dwa%s %s %d\n", data[0], data[1], gapFilterWarn[0]);

    // #pragma omp for private (i)
    for (i=0; i<2; i++)
        for (j=0; j<alphabetSize; j++)
            Pi2[i][j] = (double) (matrixCharMatches (&data[i], 1, numSites,
                alphabet[i], gapFilterWarn)) / (1.0 * count);

    // #pragma omp for private (i, j)
    for (i=0; i<alphabetSize; i++)
        for (j=0; j<alphabetSize; j++)
            P[i][j] = (double) (countStateChanges (data[0], data[1], numSites,
                alphabet[i], alphabet[j], gapFilterWarn)) / (double) count;

                // printf("%lf %lf %lf %lf\n", P[3][0], P[3][1], P[3][2], P[3][3]);

    detP = det(P, alphabetSize);

    if (0 >= detP){
        printf("WARNING\n");
        return -2; // saturated 
    }
// printf("here\n");
    D = -0.5 * log(detP);

    for (i=0; i<alphabetSize; i++)
        D += (log(Pi2[0][i]) + log(Pi2[1][i])) / 8;

    free (gapFilterWarn);

    return D;
}

double compute_jc_distance (char ** data, int numSites){
    int i;
    int *gapFilterWarn;
    int count; 

    double b = 0.0;
    
    count = count_selected(&gapFilterWarn, numSites, data);
    
    #pragma omp parallel
    for (i=0; i<numSites; i++)
        if (data[0][i] != data[1][i])
            b += gapFilterWarn[i];

    b /= count;

    if (fabs (b) - 0.0000001 < 0)
        return 0.0;

    b = 1.0 - ( 4.0 * b / 3.0 );

    if (0.0 >= b)
        return -2;

    // if (use_gamma)
    // {
    //     gamma = (gamma < DBL_EPSILON) ? DBL_EPSILON : gamma;
    //     returnValue = gamma * (0.75 * (pow (loc, -1.0 / gamma) - 1.0)) ;
    // }
    // else
    // {
    return -0.75 * (log (b));
    // }            
}


