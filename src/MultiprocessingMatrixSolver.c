#include <stdio.h>
#include <stdlib.h>
#include <sys/resource.h>
#include <sys/time.h>
#include <unistd.h>      // fork(), exit()
#include <sys/wait.h>    // wait()
#include <sys/mman.h>    // mmap(), munmap()

#define NUM_PROCESSES 4

// Function to allocate memory for a matrix (2D array)
int** create_matrix(int rows, int cols) {
    int** matrix = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        matrix[i] = (int*)malloc(cols * sizeof(int));
    }
    return matrix;
}

// Function to free the memory used by a matrix
void free_matrix(int** matrix, int rows) {
    for (int i = 0; i < rows; i++) {
        free(matrix[i]);
    }
    free(matrix);
}

// Function to fill a matrix with values from user input
void input_matrix(int** matrix, int rows, int cols, char name) {
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = rand() % 100;
        }
    }
}

// Function to print a matrix
void print_matrix(int** matrix, int rows, int cols, char name) {
    printf("\nMatrix %c (%dx%d):\n", name, rows, cols);
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            printf("%d ", matrix[i][j]);
        }
        printf("\n");
    }
}

// --- Forked matrix multiplication ---
// Spawns NUM_PROCESSES children, each handling a contiguous chunk of rows of C.
// C must point into a shared memory region (mmap MAP_SHARED) so child writes
// are visible to the parent after the children exit.
void multiply_matrices_forked(int** A, int rows_A, int cols_A,
                               int** B, int cols_B,
                               int** C) {
    int rows_per_proc = rows_A / NUM_PROCESSES;

    for (int p = 0; p < NUM_PROCESSES; p++) {
        int start_row = p * rows_per_proc;
        // Last process absorbs any remainder rows (mirrors threaded version)
        int end_row = (p == NUM_PROCESSES - 1) ? rows_A : (p + 1) * rows_per_proc;

        pid_t pid = fork();

        if (pid == 0) {
            // --- Child process ---
            // Computes C[i][j] for i in [start_row, end_row)
            for (int i = start_row; i < end_row; i++) {
                for (int j = 0; j < cols_B; j++) {
                    C[i][j] = 0;
                    for (int k = 0; k < cols_A; k++) {
                        C[i][j] += A[i][k] * B[k][j];
                    }
                }
            }
            exit(0);
        }
        // Parent continues to spawn the next child immediately
    }

    // Parent waits for all children to finish before returning
    for (int p = 0; p < NUM_PROCESSES; p++) {
        wait(NULL);
    }
}

int main(int argc, char* argv[]) {
    struct rusage start, end;
    getrusage(RUSAGE_SELF, &start);

    int rows = atoi(argv[1]);
    int cols = atoi(argv[2]);

    int** A = create_matrix(rows, cols);
    int** B = create_matrix(rows, cols);

    // C must live in shared memory so that writes from child processes
    // are visible in the parent after they exit.
    // mmap with MAP_SHARED | MAP_ANONYMOUS gives all forked children
    // a view into the same physical pages.
    int* C_data = mmap(NULL,
                       rows * cols * sizeof(int),
                       PROT_READ | PROT_WRITE,
                       MAP_SHARED | MAP_ANONYMOUS,
                       -1, 0);

    // Build the row-pointer array locally (each process gets its own copy
    // after fork, but every pointer still points into the shared C_data).
    int** C = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        C[i] = C_data + i * cols;
    }

    input_matrix(A, rows, cols, 'A');
    input_matrix(B, rows, cols, 'B');

    multiply_matrices_forked(A, rows, cols, B, cols, C);

    //print_matrix(C, rows, cols, 'C');

    free_matrix(A, rows);
    free_matrix(B, rows);
    free(C);                                      // free row-pointer array
    munmap(C_data, rows * cols * sizeof(int));    // release shared memory

    getrusage(RUSAGE_SELF, &end);
    double user_time = (end.ru_utime.tv_sec  - start.ru_utime.tv_sec) +
                       (end.ru_utime.tv_usec - start.ru_utime.tv_usec) / 1e6;
    printf("%.6f,", user_time);

    return 0;
}
