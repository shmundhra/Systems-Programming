#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <math.h>

#define NPROC 8
#define X_INIT 8
#define INT_SIZE sizeof(int)
#define INT_MAX 100

int lcg_random_generator(int *state)
{
    int A = 75;
    int M = 65537;
    int x = (A * (*state)) % M;
    return *state = x;
}

// Merges two subarrays of arr[].
// First subarray is arr[l..m]
// Second subarray is arr[m+1..r]
void merge(int* arr, int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 =  r - m;

    /* create temp arrays */
    int *L, *R;
    L = (int *)calloc(n1, sizeof(int));
    R = (int *)calloc(n2, sizeof(int));

    /* Copy data to temp arrays L[] and R[] */
    for (i = 0; i < n1; i++)
        L[i] = arr[l + i];
    for (j = 0; j < n2; j++)
        R[j] = arr[m + 1+ j];

    /* Merge the temp arrays back into arr[l..r]*/
    i = 0; // Initial index of first subarray
    j = 0; // Initial index of second subarray
    k = l; // Initial index of merged subarray
    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j]){
            arr[k++] = L[i++];
        } else {
            arr[k++] = R[j++];
        }
    }

    /* Copy the remaining elements of L[], if there are any */
    while (i < n1){
        arr[k++] = L[i++];
    }

    /* Copy the remaining elements of R[], if there are any */
    while (j < n2){
        arr[k++] = R[j++];
    }
}

// Sorts arr[l..r]
void mergeSort(int arr[], int l, int r)
{
    if (l < r)
    {
        int m = l + (r-l)/2;
        mergeSort(arr, l, m);
        mergeSort(arr, m+1, r);
        merge(arr, l, m, r);
    }
}

void debug(int nproc, int id, int array[], int size)
{
    int i, j;
    for(i = 0; i < nproc; i++) {
        if(i == id){
            for(j = 0; j < size; j++) fprintf(stdout, "%5d ", array[j]); fprintf(stdout, "\n\n");
        }
        MPI_Barrier(MPI_COMM_WORLD);
    }
    if(id == 0) fprintf(stdout, "\n");
}

int main(int argc, char *argv[]){

    int NUMBER;
    int nproc, id;
    int i , j;
    int left, right;
    int* array;
    int* temp_left;
    int* temp_right;

    if(argc < 1) {
        fprintf(stderr, "Need Number of Integers as CLA\n");
        exit(EXIT_FAILURE);
    }
    if (sscanf(argv[1], "%d", &NUMBER) < 0) {
        fprintf(stderr, "Need Integer as CLA[1]\n");
        exit(EXIT_FAILURE);
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &nproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &id);
    if(nproc < NPROC){
        fprintf(stderr, "Insufficient Number of Nodes - %d/&d for Process\n", nproc, NPROC);
        exit(EXIT_FAILURE);
    }
    NUMBER = (NUMBER/nproc)*nproc;

    int state;
    array = (int*)calloc(NUMBER/nproc, INT_SIZE);

    if(id == 0){
        state = X_INIT;
    } else {
        MPI_Recv(&state, 1, MPI_INT, id-1, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
    }

    for(j = 0; j < NUMBER/nproc; j++){
        array[j] = lcg_random_generator(&state);
    }

    if(id < nproc-1){
        MPI_Send(&state, 1, MPI_INT, id+1, 0, MPI_COMM_WORLD);
    }

    // debug(nproc, id, array, NUMBER/nproc);

    for(i = 0; i < nproc; i++){
        mergeSort(array, 0, NUMBER/nproc - 1);
    }
    MPI_Barrier(MPI_COMM_WORLD);

    // debug(nproc, id, array, NUMBER/nproc);

//_________________________________________________________________________________________
    int iterations = (int)ceil(log2(nproc));
    for(i = 0; i < iterations; i++)
    {
        int modulo = 1<<(i+1);
        int combined_size = (NUMBER * modulo) / nproc;
        left = right = INT_MAX;

        int rem = id % modulo;
        if(rem == 0){
            left  = id;
            right = id + (1<<i);
            // fprintf(stderr, "0 %d : %d <-> %d\n", id, left, right);
        }
        else if(rem == (1<<i))
        {
            left  = id - (1<<i);
            right = id;
            // fprintf(stderr, "1 %d : %d <-> %d\n", id, left, right);
        }

        if(id == left){
            // fprintf(stderr, "%d : %d <-> %d\n", id, left, right);
            temp_left = (int*)calloc(combined_size/2, INT_SIZE);
            for(j = 0; j < combined_size/2; j++) temp_left[j] = array[j];
            free(array);

            temp_right = (int*)calloc(combined_size/2, INT_SIZE);
            MPI_Recv(temp_right, combined_size/2, MPI_INT, right, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

            array = (int*)calloc(combined_size, INT_SIZE);
            for(j = 0; j < combined_size/2; j++) array[j] = temp_left[j];
            for(j = 0; j < combined_size/2; j++) array[j + combined_size/2] = temp_right[j];
            free(temp_left), free(temp_right);

            merge(array, 0, combined_size/2 - 1, combined_size-1);
        }
        else if(id == right){
            // fprintf(stderr, "%d : %d <-> %d\n", id, left, right);
            MPI_Send(array, combined_size/2, MPI_INT, left, 0, MPI_COMM_WORLD);
            free(array);
        }
        MPI_Barrier(MPI_COMM_WORLD);

        // debug(nproc, id, array, combined_size);
    }

    if(id == 0){
        // for(j = 0; j < NUMBER; j++) fprintf(stdout, "%5d ", array[j]); fprintf(stdout, "\n");
        for(j = 0; j < NUMBER-1; j++) if(array[j] > array[j+1]) exit(EXIT_FAILURE);
        fprintf(stdout, "SUCCESS!\n");
    }

    MPI_Finalize();
}