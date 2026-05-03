#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>   /* uint8_t — MEMORY OPT 1 */
#include <math.h>
#include <time.h>
#include <omp.h>


static void init_road(uint8_t *road, int n, double density, unsigned int *seed)
{
    int    i;
    double r;

    for (i = 0; i < n; i++) {
        r = (double)rand_r(seed) / ((double)RAND_MAX + 1.0);
        road[i] = (r < density) ? 1 : 0;
    }
}

static double simulate(uint8_t *road, uint8_t *road_new, int n,
                       int warmup, int measure)
{
    int      step, i;
    int      left, right;
    int      total_cars  = 0;
    long     moved_total = 0;
    uint8_t *tmp;                 /* MEMORY OPT 1: swap pointer is uint8_t* */

    /* ---- Count total cars ------------------------------------------------ */
    /* OPTION 1: parallel reduction across the uint8_t array.                 */
    /* Byte-wide loads mean more cells fit per cache line (64 cells/line vs   */
    /* 16 with int), so each thread's chunk requires fewer cache line fetches. */
    #pragma omp parallel for \
        private(i) \
        reduction(+:total_cars) \
        schedule(static)
    for (i = 0; i < n; i++) {
        total_cars += road[i];
    }

    if (total_cars == 0) return 1.0;

    /* ------------------------------------------------------------------ */
    /* WARM-UP phase                                                        */
    /* ------------------------------------------------------------------ */
    for (step = 0; step < warmup; step++) {

        #pragma omp parallel for \
            private(i, left, right) \
            schedule(static)
        for (i = 0; i < n; i++) {
            left  = (i - 1 + n) % n;
            right = (i + 1)     % n;

            if (road[i] == 0) {
                road_new[i] = road[left];
            } else {
                road_new[i] = road[right];
            }
        }
        /* Pointer swap: reassigns local variables only, does not move or
         * reallocate the pool memory that main owns. Safe and free.       */
        tmp      = road;
        road     = road_new;
        road_new = tmp;
    }

    /* ------------------------------------------------------------------ */
    /* MEASUREMENT phase                                                    */
    /* ------------------------------------------------------------------ */
    for (step = 0; step < measure; step++) {
        int moved_this_step = 0;

        #pragma omp parallel for \
            private(i, left, right) \
            reduction(+:moved_this_step) \
            schedule(static)
        for (i = 0; i < n; i++) {
            left  = (i - 1 + n) % n;
            right = (i + 1)     % n;

            if (road[i] == 0) {
                road_new[i] = road[left];
            } else {
                road_new[i] = road[right];
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
int main(int argc, char *argv[])
{
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    if (argc < 3) {
        fprintf(stderr, "Usage: %s <N> <density_steps>\n", argv[0]);
        return EXIT_FAILURE;
    }

    int n             = atoi(argv[1]);
    int density_steps = atoi(argv[2]);

    int warmup_steps  = n / 10;
    int measure_steps = n / 10;

    unsigned int master_seed = (unsigned int)time(NULL);

    /* Nested parallelism setup — unchanged from previous version.
     * Outer level (density loop) uses all cores.
     * Inner level (cell loop) uses 1 thread per outer thread.            */
    omp_set_nested(1);
    omp_set_max_active_levels(2);

    int outer_threads = omp_get_max_threads();
    int inner_threads = 1;

    uint8_t *road_pool     = (uint8_t *)malloc((size_t)outer_threads * n * sizeof(uint8_t));
    uint8_t *road_new_pool = (uint8_t *)malloc((size_t)outer_threads * n * sizeof(uint8_t));

    if (!road_pool || !road_new_pool) {
        fprintf(stderr, "Error: could not allocate road pools.\n");
        free(road_pool);
        free(road_new_pool);
        return EXIT_FAILURE;
    }

    #pragma omp parallel for \
        schedule(dynamic) \
        num_threads(outer_threads) \
        default(none) \
        shared(n, density_steps, warmup_steps, measure_steps, \
               master_seed, inner_threads, road_pool, road_new_pool, stderr)
    for (int d = 0; d <= density_steps; d++) {

        /* MEMORY OPT 3: each thread carves its slice out of the pool.
         * No allocation cost, no lock contention, no heap fragmentation. */
        int      tid      = omp_get_thread_num();
        uint8_t *road     = road_pool     + (size_t)tid * n;
        uint8_t *road_new = road_new_pool + (size_t)tid * n;

        /* Unique seed per density point: independent random sequences
         * across threads without any shared state.                       */
        unsigned int seed = master_seed + (unsigned int)d;

        double density = (double)d / (double)density_steps;
        double avg_vel;

        /* Set inner thread count for nested Option 1 cell-loop regions   */
        omp_set_num_threads(inner_threads);

        init_road(road, n, density, &seed);

        avg_vel = simulate(road, road_new, n, warmup_steps, measure_steps);

        /* Uncomment to print results:
         * printf("%.6f  %.6f\n", density, avg_vel);
         */
        (void)avg_vel;
    }


    /* MEMORY OPT 3: single free per pool — 2 calls total regardless of
     * how many density steps or threads were used.                       */
    free(road_pool);
    free(road_new_pool);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed = (end.tv_sec  - start.tv_sec) +
                     (end.tv_nsec - start.tv_nsec) / 1e9;
    printf("%.6f,", elapsed);

    return EXIT_SUCCESS;
}