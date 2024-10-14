#include "mpi.h"
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <math.h>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>


int is_sorted(double arr[], int arr_size){
    for (int i = 0; i < arr_size-1; i++) {
        if (arr[i] > arr[i + 1]) {
            return 0;
        }
    }
    return 1;
}

void merge(double arr[], double left[], double right[], int left_size, int right_size) {
    int i = 0;
    int j = 0;
    int pos = 0;    

    while (i < left_size && j < right_size) {
        if (left[i] < right[j]) {
             arr[pos] = left[i];
             i++;
        } else {
            arr[pos] = right[j];
            j++;
        }
        
        pos++;
    }

    while (i < left_size) {
        arr[pos] = left[i];
        i++;
        pos++;
    }
    
    while (j < right_size) {
        arr[pos] = right[j];
        j++;
        pos++;
    }


}


void mergesort(double arr[], int arr_size) {
    if (arr_size <= 1) {
        return;
    }
    
   int middle = arr_size / 2;
   
    // split arr in two
    double left[middle];
    double right[arr_size-middle];
    for (int i = 0; i < middle; i++) {
        left[i] = arr[i];
        right[i] = arr[i+middle];
    }

    mergesort(left, middle);
    mergesort(right, arr_size-middle);

    merge(arr, left, right, middle, arr_size-middle);
}




int main (int argc, char * argv[])
{
    CALI_MARK_BEGIN("main");
    
    cali::ConfigManager mgr;
    mgr.start();

    int inputSize;
    char inputType;
    
    if (argc == 3) {
        inputSize = atoi(argv[1]);
        inputType = *argv[2];
    } else {
        printf("\n Please provide the input size and type");
        return 0;
    }
    
    int numtasks,
        taskid;

    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &taskid);
    MPI_Comm_size(MPI_COMM_WORLD, &numtasks);
    
    int elements_per_proc = inputSize / numtasks;
    

    adiak::init(NULL);
    adiak::launchdate();    // launch date of the job
    adiak::libraries();     // Libraries used
    adiak::cmdline();       // Command line used to launch the job
    adiak::clustername();   // Name of the cluster
    adiak::value("algorithm", "merge"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
    adiak::value("programming_model", "mpi"); // e.g. "mpi"
    adiak::value("data_type", "double"); // The datatype of input elements (e.g., double, int, float)
    adiak::value("size_of_data_type", sizeof(double)); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
    adiak::value("input_size", inputSize); // The number of elements in input dataset (1000)
    adiak::value("input_type", "Random"); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
    adiak::value("num_procs", numtasks); // The number of processors (MPI ranks)
    adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
    adiak::value("group_num", "6"); // The number of your group (integer, e.g., 1, 10)
    adiak::value("implementation_source", "online"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").

    /*************** Input Validation *******************/
    
    CALI_MARK_BEGIN("data_init_runtime");

    double input[inputSize];
    
    for (int i = 0; i < inputSize; i++) {
        input[i] = rand() % 2000;
       // printf("\n %f", input[i]);
    }    

    double local[inputSize]; 
    MPI_Scatter(input, elements_per_proc, MPI_DOUBLE, local, elements_per_proc, MPI_DOUBLE, 0, MPI_COMM_WORLD);
    for (int i = 0; i < elements_per_proc; i++) {
       // printf("\n %d: %f", taskid, local[i]);
    }
    
    CALI_MARK_END("data_init_runtime");

    /****************** Merge Sort **********************/   
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");    
    mergesort(local, elements_per_proc);
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");    

    int log_procs = log2(numtasks);
    int active_procs = numtasks;    
    CALI_MARK_BEGIN("comm");
    
    // not all procs recv so the avg time is scewed for that reason
    CALI_MARK_BEGIN("comm_large");    

    for (int i = 0; i < log_procs; i++) {
        if (taskid < active_procs /2) {
            double recv[inputSize/active_procs];
            MPI_Recv(recv, inputSize/active_procs, MPI_DOUBLE, taskid + active_procs/2, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            double new_local[2 * inputSize/active_procs];  
            merge(new_local, local, recv, inputSize / active_procs, inputSize / active_procs);
            for (int i = 0; i < 2 * inputSize / active_procs; i++) {
                local[i] = new_local[i];
               // printf("\n %d: %f", taskid, local[i]);   
            }   
        } else if (taskid <= active_procs) {
            int dest = taskid - active_procs / 2;
            MPI_Send(local, inputSize / active_procs, MPI_DOUBLE, dest, 0, MPI_COMM_WORLD);
        }
        
        active_procs /= 2;
    }        
    
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");  


    /*************** Output Validation ******************/

    CALI_MARK_BEGIN("correctness_check");

    if (taskid == 0) {
        for (int i = 0; i < inputSize; i++) {
           // printf("\n %d: %f", taskid, local[i]);
         }
        printf("\n Is Sorted: %d", is_sorted(local, inputSize));   
    } 
    
    CALI_MARK_END("correctness_check");    
    CALI_MARK_END("main");
      
    mgr.stop();
    mgr.flush();
           
    MPI_Finalize(); 
}
