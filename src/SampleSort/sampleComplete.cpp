#include <stdlib.h>
#include <stdio.h>
#include <mpi.h>
#include <float.h>
#include <sys/time.h>
#include <limits.h>
#include <stdbool.h>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>
#define SEED 100
#define OUTPUT 1
#define CHECK 1

double get_time() {
   struct timeval tv; 
   int ok = gettimeofday(&tv, NULL);
   if (ok < 0) { 
       printf("gettimeofday error");  
   }
   return (tv.tv_sec * 1.0 + tv.tv_usec * 1.0E-6); 
}

int compare_func(const void *num1, const void *num2) {
    unsigned long long* n1 = (unsigned long long*)num1;
    unsigned long long* n2 = (unsigned long long*)num2;
    return (*n1 > *n2) - (*n1 < *n2);
}

int main(int argc, char *argv[]) {
    int i, j, input_size, bucket_count, total_count;
    double start_time, end_time;
    unsigned long long *split_points, *elements, *samples, **buckets; 
    unsigned long long *my_elements, *my_samples;
    int my_rank, total_procs;
    unsigned long long check, check_before, global_check;
    int *bucket_sizes;
    int *recv_count, *displ;
    bool checkMax, global_checkMax;

    adiak::init(NULL);  // Pass NULL for MPI_COMM_WORLD

    cali::ConfigManager mgr;
    mgr.add("runtime-report");
    mgr.start();

    CALI_MARK_BEGIN("main");

    if (argc != 2) {
        fprintf(stderr, "Usage: %s <input_size>\n", argv[0]);
        return -1;
    }

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &my_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &total_procs);

    bucket_count = total_procs;
    input_size = atoi(argv[1]);

    if (my_rank == 0) {
        adiak::value("algorithm", "SampleSort");
        adiak::value("programming_model", "MPI");
        adiak::value("data_type", "unsigned long long");
        adiak::value("input_size", input_size);
        adiak::value("num_procs", total_procs);
        adiak::value("scalability", "strong");
    }

    if(my_rank == 0) {
        CALI_MARK_BEGIN("data_init_runtime");
        elements = (unsigned long long*)malloc(sizeof(unsigned long long) * input_size);
        recv_count = (int*)malloc(sizeof(int) * bucket_count);
        displ = (int*)malloc(sizeof(int) * bucket_count);
        srand(SEED);
        for(i = 0; i < input_size; i++) {
            elements[i] = rand() % 100;
        }
        CALI_MARK_END("data_init_runtime");
    }

    my_elements = (unsigned long long*)malloc(sizeof(unsigned long long) * (input_size / bucket_count));
    my_samples = (unsigned long long*)malloc(sizeof(unsigned long long) * (bucket_count - 1));
    split_points = (unsigned long long*)malloc(sizeof(unsigned long long) * bucket_count);
    samples = (unsigned long long*)malloc(sizeof(unsigned long long) * bucket_count * (bucket_count - 1));
    buckets = (unsigned long long**)malloc(sizeof(unsigned long long*) * bucket_count);
    
    for(i = 0; i < bucket_count; i++) {
        buckets[i] = (unsigned long long*)malloc(sizeof(unsigned long long) * 2 * input_size / bucket_count);
    }
    bucket_sizes = (int*)malloc(sizeof(int) * bucket_count);
    for(i = 0; i < bucket_count; i++) {
        bucket_sizes[i] = 0;
    }

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    MPI_Scatter(elements, input_size / bucket_count, MPI_UNSIGNED_LONG_LONG, my_elements, input_size / bucket_count, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    if(my_rank == 0) {
        check_before = 0;
        for(i = 0; i < input_size; i++) {
            check_before ^= elements[i];
        }
    }

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_small");
    MPI_Bcast(&check_before, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_small");
    CALI_MARK_END("comm");

    MPI_Barrier(MPI_COMM_WORLD);
    start_time = get_time();

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    qsort(my_elements, input_size / bucket_count, sizeof(unsigned long long), compare_func);
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    for(j = 0; j < bucket_count - 1; j++) {
        my_samples[j] = my_elements[input_size / bucket_count / bucket_count * (j + 1)];
    }

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_small");
    MPI_Allgather(my_samples, bucket_count - 1, MPI_UNSIGNED_LONG_LONG, samples, bucket_count - 1, MPI_UNSIGNED_LONG_LONG, MPI_COMM_WORLD);
    CALI_MARK_END("comm_small");
    CALI_MARK_END("comm");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");
    qsort(samples, bucket_count * (bucket_count - 1), sizeof(unsigned long long), compare_func);
    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");

    for(i = 1; i < bucket_count; i++) {
        split_points[i - 1] = samples[i * (bucket_count - 1)];
    }
    split_points[bucket_count - 1] = ULLONG_MAX;

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    for(i = 0; i < input_size / bucket_count; i++) {
        j = 0;
        while(j < bucket_count) {
            if(my_elements[i] < split_points[j]) {
                buckets[j][bucket_sizes[j]] = my_elements[i];
                bucket_sizes[j]++;
                j = bucket_count;
            }
            j++;
        }
    }
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    for(i = 0; i < bucket_count; i++) {
        qsort(buckets[i], bucket_sizes[i], sizeof(unsigned long long), compare_func);
    }
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    MPI_Barrier(MPI_COMM_WORLD);

    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    for(i = 0; i < bucket_count; i++) {
        MPI_Gather(&bucket_sizes[i], 1, MPI_INT, recv_count, 1, MPI_INT, 0, MPI_COMM_WORLD);
        if(my_rank == 0) {
            displ[0] = 0;
            for(j = 1; j < bucket_count; j++) {
                displ[j] = displ[j - 1] + recv_count[j - 1];
            }
        }
        MPI_Gatherv(buckets[i], bucket_sizes[i], MPI_UNSIGNED_LONG_LONG, elements, recv_count, displ, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
    }
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");

    if(my_rank == 0) {
        CALI_MARK_BEGIN("comp");
        CALI_MARK_BEGIN("comp_large");
        qsort(elements, input_size, sizeof(unsigned long long), compare_func);  
        CALI_MARK_END("comp_large");
        CALI_MARK_END("comp");

        CALI_MARK_BEGIN("correctness_check");
        FILE *outputFile = fopen("sorted_output.txt", "w");
        if(outputFile != NULL) {
            for(i = 0; i < input_size; i++) {
                fprintf(outputFile, "%llu\n", elements[i]);
            }
            fclose(outputFile);
        }
        end_time = get_time();
        printf("Time: %lf\n", (end_time - start_time));
        CALI_MARK_END("correctness_check");
    }

    free(my_elements);
    free(my_samples);
    free(split_points);
    free(samples);
    free(bucket_sizes);
    for(i = 0; i < bucket_count; i++) {
        free(buckets[i]);
    }
    free(buckets);

    if(my_rank == 0) {
        free(elements);
        free(recv_count);
        free(displ);
    }

    CALI_MARK_END("main");

    mgr.stop();
    mgr.flush();

    MPI_Finalize();
    return 0;
}
