#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>
#include <caliper/cali.h>  // For basic Caliper instrumentation
#include <adiak.hpp>
#define MASTER 0
// randomly fills the processor arrays with data
void generate_input_random(int arr[], int K, int& global_max){
     // START OF DATA INIT
    CALI_MARK_BEGIN("data_init_runtime"); 
    std::srand(static_cast<unsigned int>(std::time(nullptr)));
    
    int local_max = 0;

    for(int j = 0; j < K; j++){
        arr[j] = std::rand() % 2000;
        local_max = std::max(local_max, arr[j]);
    }

    MPI_Reduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);

    MPI_Bcast(&global_max, 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("data_init_runtime");
    // END OF DATA INIT

}

void generate_input_reversed(int arr[], int K, int& global_max) {
    CALI_MARK_BEGIN("data_init_runtime");
   
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
   
    int total_elements = K * size;
    int start_value = total_elements - (rank * K) - 1;
   
    for (int j = 0; j < K; j++) {
        arr[j] = start_value - j;
    }
   
    int local_max = arr[0];
    
    MPI_Reduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Bcast(&global_max, 1, MPI_INT, 0, MPI_COMM_WORLD);
   
    CALI_MARK_END("data_init_runtime");
}

void generate_input_sorted(int arr[], int K, int& global_max){
    CALI_MARK_BEGIN("data_init_runtime");
    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int start_value = rank * K;
    for (int j = 0; j < K; j++) {    
        arr[j] = start_value + j;
    }
    int local_max = arr[K - 1];
    MPI_Reduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Bcast(&global_max, 1, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("data_init_runtime");

} 

int positive_modulo(int i, int n) { return (i % n + n) % n; }

void generate_input_1percent(int arr[], int K, int& global_max){
    CALI_MARK_BEGIN("data_init_runtime");
    int elements_per_proc = K;
    int* local = arr;
    int rank, size;
    
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    int num_elements = K*size;
    int num_procs = size;
    int range = rank * elements_per_proc;
    for (int i = 0; i < elements_per_proc; i++) {
       local[i] = range++;
    }
    
    int local_max = local[K-1];
    
    MPI_Reduce(&local_max, &global_max, 1, MPI_INT, MPI_MAX, 0, MPI_COMM_WORLD);
    MPI_Bcast(&global_max, 1, MPI_INT, 0, MPI_COMM_WORLD);
   

    int one_percent = num_elements / 100;
    int num_local_swaps = one_percent / 4;
    int num_nonlocal_swaps = (one_percent * 3) / 4;
    

     for (int i = 0; i < num_local_swaps; i++) {
        std::swap(local[rand() % elements_per_proc],
              local[rand() % elements_per_proc]);
     }


     int curr;
     int other;
     int other_rank;
     int gap = 1;


   for (int i = 0; i < num_nonlocal_swaps; i++) {
    int index = rand() % elements_per_proc;
    curr = local[index];


    if (rank % 2 == 0) {
      other_rank = positive_modulo((rank + gap), num_procs);
      MPI_Send(&curr, 1, MPI_INT, other_rank, 0, MPI_COMM_WORLD);
      MPI_Recv(&other, 1, MPI_INT, other_rank, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
    } else {
      other_rank = positive_modulo((rank - gap), num_procs);
      MPI_Send(&curr, 1, MPI_INT, other_rank, 0, MPI_COMM_WORLD);
      MPI_Recv(&other, 1, MPI_INT, other_rank, 0, MPI_COMM_WORLD,
               MPI_STATUS_IGNORE);
    }
    gap += 2;

    local[index] = other;
    }

    CALI_MARK_END("data_init_runtime");

}

bool is_sorted_p(int* arr_chunk, int arr_size, int rank, int p){
    // verify each local arr_chunk is sorted then the border or p+1 first element
    for (int i = 0; i < arr_size - 1; i++) {
        if (arr_chunk[i] > arr_chunk[i + 1]) {
            return 0;
        }
    }

    int last_elements[p];
    MPI_Gather(&arr_chunk[arr_size - 1], 1, MPI_INT, last_elements, 1, MPI_INT, 0, MPI_COMM_WORLD);
    if (rank == 0) {
        // check the last element of each segment to ensure the overall order
        for (int i = 0; i < p - 1; i++) {
            if (last_elements[i] > last_elements[i + 1]) {
                return 0;
            }
        }
        return 1;
    }
    return 0;
}

void counting_sort(int* arr_chunk, int arr_size, int P, int exp, int rank){
    
    // count local occurances of each digit for the processor current rank
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    int local_cnt[10] = {0};
    for(int i = 0; i < arr_size; i++){
        int digit = (arr_chunk[i] / exp) % 10;
        local_cnt[digit]++;
    }
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");
    MPI_Barrier(MPI_COMM_WORLD);
    // gather the results of all other parts into local_cnt to calculate start
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_small");
    int gathered_cnt[P][10] = {0};
    MPI_Gather(local_cnt, 10, MPI_INT, gathered_cnt, 10, MPI_INT, 0, MPI_COMM_WORLD);
    CALI_MARK_END("comm_small");
    CALI_MARK_END("comm");
    // construct the start array and broadcast to all processes
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_small");
    if(rank == MASTER){
        int base = 0;
        for(int digit = 0; digit < 10; digit++){
            for(int p = 0; p < P; p++){
                int offset = gathered_cnt[p][digit];
                gathered_cnt[p][digit] = base;
                base += offset;
            }
        }
        
    }
    CALI_MARK_END("comp_small");
    CALI_MARK_END("comp");
    
    // once finished, give all processes the gathered_cnt for starts
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_small");
    MPI_Bcast(gathered_cnt, P * 10, MPI_INT, MASTER, MPI_COMM_WORLD);
    
    CALI_MARK_END("comm_small");
    CALI_MARK_END("comm");
    // each process will now construct a "job list" of (process, pos_in_part, value)
    // and send it over the wire to the correct processor

    // std::vector<std::vector<int>> send_buffers;
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    int send_buffers[arr_size][3] = {0};
    int output[arr_size] = {0};
    int send_count = 0;
    for (int i = 0; i < arr_size; i++) {
        int digit = (arr_chunk[i] / exp) % 10;
        int start = gathered_cnt[rank][digit]++;
        int p = start / arr_size; 
        int rel_pos = start % arr_size;

        // Only fill send buffers for different ranks
        if (p != rank) {
            send_buffers[send_count][0] = p; // destination process
            send_buffers[send_count][1] = rel_pos; // relative position
            send_buffers[send_count][2] = arr_chunk[i]; // value
            send_count++;
        } else {
            output[rel_pos] = arr_chunk[i]; // place directly in output
        }
    }
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

    // wait for all processes to get work
    // MPI_Barrier(MPI_COMM_WORLD);
    // signal other processes jobs to perform
    CALI_MARK_BEGIN("comm");
    CALI_MARK_BEGIN("comm_large");
    std::vector<MPI_Request> send_requests(send_count);

    for (int i = 0; i < send_count; i++) {
        int dest_p = send_buffers[i][0];
        MPI_Isend(send_buffers[i], 3, MPI_INT, dest_p, 0, MPI_COMM_WORLD, &send_requests[i]);
    }
    // wait for all processes to send work on wire
    // MPI_Barrier(MPI_COMM_WORLD);

    int recv_buffer[arr_size][3] = {0};

    // MPI_Request recv_requests[arr_size];
    std::vector<MPI_Request> recv_requests(arr_size);

    for (int i = 0; i < send_count; i++) {
        MPI_Irecv(recv_buffer[i], 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &recv_requests[i]);
    }

    // Wait for all receive operations to complete
    MPI_Waitall(send_count, send_requests.data(), MPI_STATUSES_IGNORE);
    MPI_Waitall(send_count, recv_requests.data(), MPI_STATUSES_IGNORE);
    CALI_MARK_END("comm_large");
    CALI_MARK_END("comm");
    // int output[arr_size] = {0};
    CALI_MARK_BEGIN("comp");
    CALI_MARK_BEGIN("comp_large");
    for(int i = 0; i < send_count; i++){
        int pos = recv_buffer[i][1];
        int elm = recv_buffer[i][2];
        output[pos] = elm;
    }

    // MPI_Barrier(MPI_COMM_WORLD);

    // once everyone is done update our current array
    for(int i = 0; i < arr_size; i++){
        arr_chunk[i] = output[i];
    }
    CALI_MARK_END("comp_large");
    CALI_MARK_END("comp");

}


int main(int argc, char** argv){
    

    CALI_MARK_FUNCTION_BEGIN;
    
    MPI_Init(&argc, &argv);  // Initialize MPI
    
    int P = atoi(argv[1]);
    int K = atoi(argv[2]);

    adiak::init(NULL);
	adiak::launchdate();    // launch date of the job
	adiak::libraries();     // Libraries used
	adiak::cmdline();       // Command line used to launch the job
	adiak::clustername();   // Name of the cluster
	adiak::value("algorithm", "radix"); // The name of the algorithm you are using (e.g., "merge", "bitonic")
	adiak::value("programming_model", "mpi"); // e.g. "mpi"
	adiak::value("data_type", "int"); // The datatype of input elements (e.g., double, int, float)
	adiak::value("size_of_data_type", 4); // sizeof(datatype) of input elements in bytes (e.g., 1, 2, 4)
	adiak::value("input_size", K*P); // The number of elements in input dataset (1000)
	adiak::value("input_type", argv[3]); // For sorting, this would be choices: ("Sorted", "ReverseSorted", "Random", "1_perc_perturbed")
	adiak::value("num_procs", P); // The number of processors (MPI ranks)
	adiak::value("scalability", "strong"); // The scalability of your algorithm. choices: ("strong", "weak")
	adiak::value("group_num", 6); // The number of your group (integer, e.g., 1, 10)
	adiak::value("implementation_source", "handwritten"); // Where you got the source code of your algorithm. choices: ("online", "ai", "handwritten").



    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    
    int arr[K] = {0};
    int global_max = 0;

    if(rank == MASTER){
        std::cout << "STARTING RADIX SORT... K: "<< K << ", " << P <<" ," << argv[3]<< std::endl; 
    }
    if(strcmp(argv[3], "Random") == 0){
        // START OF DATA INIT
        CALI_MARK_BEGIN("data_init_runtime"); 
        generate_input_random(arr, K, global_max);
        CALI_MARK_END("data_init_runtime");
        // END OF DATA INIT
    } else if(strcmp(argv[3], "Sorted") == 0){
        CALI_MARK_BEGIN("data_init_runtime"); 
        generate_input_sorted(arr, K, global_max);
        CALI_MARK_END("data_init_runtime");
    } else if(strcmp(argv[3], "Reversed") == 0){
        CALI_MARK_BEGIN("data_init_runtime"); 
        generate_input_reversed(arr, K, global_max);
        CALI_MARK_END("data_init_runtime");
    } else {
        CALI_MARK_BEGIN("data_init_runtime"); 
        generate_input_1percent(arr, K, global_max);
        CALI_MARK_END("data_init_runtime");
    }
    // wait for everyone to generate data
    MPI_Barrier(MPI_COMM_WORLD);
    if(rank == MASTER){
	std::cout <<"Found Global Max: " << global_max << std::endl;
    }
    // START OF SORT
    for (int exp = 1; global_max / exp > 0; exp *= 10) {
        counting_sort(arr, K, P, exp, rank);
        if(rank == MASTER){
            std::cout << "Finished exp stage: " << exp << std::endl;
        }
    }
    // END OF SORT

    // Wait for all processes before final output
    MPI_Barrier(MPI_COMM_WORLD);
    
    // START OF CORRECTNESS CHECK
    CALI_MARK_BEGIN("correctness_check");
    bool is_sorted = is_sorted_p(arr, K, rank, P);
    CALI_MARK_END("correctness_check");
    // END OF CORRECTNESS CHECK

    if(rank == MASTER && !is_sorted){
        std::cout<<"ERROR: not sorted." << std::endl;
    } else if(rank == MASTER && is_sorted){
        std::cout<<"SUCCESS: sorted." << std::endl;
    }
    
    CALI_MARK_FUNCTION_END;
    
    // wait for master to display sorted result
    MPI_Barrier(MPI_COMM_WORLD);
    adiak::fini();
    MPI_Finalize();  // Finalize MPI

    return 0;
}
