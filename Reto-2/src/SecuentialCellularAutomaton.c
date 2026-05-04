#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

static void init_road(int *road, int n, double density, unsigned int *seed)
{
    /* Fill the road array with cars randomly so that the expected occupancy
    * equals `density`.  A simple LCG (linear congruential generator) is used
    * via rand_r so there is no global state dependency.*/

    int i;
    double r;

    for (i = 0; i < n; i++) {
        /* rand_r returns a value in [0, RAND_MAX]; scale to [0.0, 1.0) */
        r = (double)rand_r(seed) / ((double)RAND_MAX + 1.0);
        road[i] = (r < density) ? 1 : 0;
    }
}

static double simulate(int *road, int *road_new, int n,
                       int warmup, int measure)
{
    int    step, i;
    int    left, right;          /* neighbour indices                        */
    int    total_cars  = 0;      /* cars counted once (constant by design)   */
    long   moved_total = 0;      /* cumulative moved cars over measure steps */
    int   *tmp;                  /* pointer swap helper                      */

    /* Count total cars (conserved quantity – count once) */
    for (i = 0; i < n; i++) {
        total_cars += road[i];
    }

    /* Safety: if the road is empty avoid division by zero later */
    if (total_cars == 0) return 1.0;  /* convention: empty road, speed = 1 */

    /* ------------------------------------------------------------------ */
    /* WARM-UP phase: Let the system forget its artificial initial conditions and reach a physically meaningful steady state */
    /* ------------------------------------------------------------------ */
    for (step = 0; step < warmup; step++) {
        for (i = 0; i < n; i++) {
            left  = (i - 1 + n) % n;
            right = (i + 1)     % n;

            if (road[i] == 0) {
                /* Empty cell: a car arrives only if the left cell has one */
                road_new[i] = road[left];
            } else {
                /* Full cell: car stays only if the cell ahead is also full */
                road_new[i] = road[right];
            }
        }
        /* Swap old and new arrays */
        tmp      = road;
        road     = road_new;
        road_new = tmp;
    }

    /* ------------------------------------------------------------------ */
    /* MEASUREMENT phase: record the number of cars that move each step   */
    /* ------------------------------------------------------------------ */
    for (step = 0; step < measure; step++) {
        int moved_this_step = 0;

        for (i = 0; i < n; i++) {
            left  = (i - 1 + n) % n;
            right = (i + 1)     % n;

            if (road[i] == 0) {
                road_new[i] = road[left];
            } else {
                road_new[i] = road[right];
                /*
                 * A car moves forward when old[i]=1 and the cell ahead
                 * is empty (old[right]=0). 
                 */
                if (road[right] == 0) {
                    moved_this_step++;
                }
            }
        }

        moved_total += moved_this_step;

        tmp      = road;
        road     = road_new;
        road_new = tmp;
    }

    return (double)moved_total / ((double)total_cars * (double)measure);
}

/* =========================================================================
 * main
 * ========================================================================= */

int main(int argc, char* argv[])
{
    
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    int n = atoi(argv[1]);  /* Road length (number of cells)                 */
    int density_steps = atoi(argv[2]);  

    int warmup_steps = n/10;  /* Steps discarded to allow settling to steady state */
    int measure_steps = n/10; /* Steps used to compute the asymptotic velocity */

    int    *road     = NULL;
    int    *road_new = NULL;
    unsigned int seed;
    double density, avg_vel;
    int    d;

    /* Allocate the two road arrays (old state and new state) */
    road     = (int *)malloc(n * sizeof(int));
    road_new = (int *)malloc(n * sizeof(int));
    if (!road || !road_new) {
        fprintf(stderr, "Error: could not allocate road arrays.\n");
        free(road);
        free(road_new);
        return EXIT_FAILURE;
    }

    seed = (unsigned int)time(NULL);

    /*
    TO PRINT AND TO CHECK IF IT IS WORKING

    printf("# Traffic Flow Cellular Automaton\n");
    printf("# Road length N = %d\n", n);
    printf("# Warm-up steps = %d  |  Measurement steps = %d\n",
           warmup_steps, measure_steps);
    printf("#\n");
    printf("# density   avg_velocity\n");*/

    /*
     * Sweep density from 0 to 1 inclusive.
     * We include the endpoint density = 1.0 by running one extra iteration.
     */
    for (d = 0; d <= density_steps; d++) {

        density = (double)d / (double)density_steps;

        /*
        // Edge cases: analytical results are exact 
        if (density <= 0.0) {
            printf("%.6f  %.6f\n", 0.0, 1.0);   // empty road – no cars 
            continue;
        }
        if (density >= 1.0) {
            printf("%.6f  %.6f\n", 1.0, 0.0);   // full road – all blocked
            continue;
        }
        */

        /* Initialise road with random car placement at given density */
        init_road(road, n, density, &seed);

        /* Run simulation and obtain asymptotic average velocity */
        avg_vel = simulate(road, road_new, n, warmup_steps, measure_steps);

        //printf("%.6f  %.6f\n", density, avg_vel);
    }

    free(road);
    free(road_new);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec - start.tv_sec) +
                        (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%.6f,", elapsed);

    return EXIT_SUCCESS;
}
