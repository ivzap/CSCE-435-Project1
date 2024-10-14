#include <mpi.h>
#include <iostream>
#include <vector>
#include <cstdlib>
#include <ctime>

#define MASTER 0
#define K 100000 // the subset size each processor gets
#define P 10 // the number of processes 

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

void counting_sort(int* arr_chunk, int arr_size, int exp, int rank){
    
    // count local occurances of each digit for the processor current rank
    int local_cnt[10] = {0};
    for(int i = 0; i < arr_size; i++){
        int digit = (arr_chunk[i] / exp) % 10;
        local_cnt[digit]++;
    }

    // gather the results of all other parts into local_cnt to calculate start
    int gathered_cnt[P][10] = {0};
    MPI_Gather(local_cnt, 10, MPI_INT, gathered_cnt, 10, MPI_INT, 0, MPI_COMM_WORLD);

    // construct the start array and broadcast to all processes
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
    
    // once finished, give all processes the gathered_cnt for starts
    MPI_Bcast(gathered_cnt, P * 10, MPI_INT, MASTER, MPI_COMM_WORLD);

    // each process will now construct a "job list" of (process, pos_in_part, value)
    // and send it over the wire to the correct processor

    // std::vector<std::vector<int>> send_buffers;
    int send_buffers[arr_size][3] = {0};
    // std::vector<MPI_Request> send_requests(K);
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

    // wait for all processes to get work
    // MPI_Barrier(MPI_COMM_WORLD);

    // signal other processes jobs to perform
    std::vector<MPI_Request> send_requests(send_count);

    for (int i = 0; i < send_count; i++) {
        int dest_p = send_buffers[i][0];
        MPI_Isend(send_buffers[i], 3, MPI_INT, dest_p, 0, MPI_COMM_WORLD, &send_requests[i]);
    }
    // wait for all processes to send work on wire
    // MPI_Barrier(MPI_COMM_WORLD);

    int recv_buffer[K][3] = {0};

    // MPI_Request recv_requests[arr_size];
    std::vector<MPI_Request> recv_requests(arr_size);

    for (int i = 0; i < send_count; i++) {
        MPI_Irecv(recv_buffer[i], 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &recv_requests[i]);
    }

    // Wait for all receive operations to complete
    MPI_Waitall(send_count, send_requests.data(), MPI_STATUSES_IGNORE);
    MPI_Waitall(send_count, recv_requests.data(), MPI_STATUSES_IGNORE);

    // int output[arr_size] = {0};
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

}


int main(int argc, char** argv){
    MPI_Init(&argc, &argv);  // Initialize MPI

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    // Seed the random number generator
    std::srand(static_cast<unsigned int>(std::time(nullptr)));

    int arr[P][K] = {0};
    int max_val = 0;
    // Fill the array with random values between 0 and 20
    for (int i = 0; i < P; i++) {
        for (int j = 0; j < K; j++) {
            arr[i][j] = std::rand() % 2000; // Random number between 0 and 20
            max_val = std::max(max_val, arr[i][j]);
        }
    }

    for (int exp = 1; max_val / exp > 0; exp *= 10) {
        counting_sort(arr[rank], K, exp, rank);
        std::cout << exp <<std::endl;
    }

    // Wait for all processes before final output
    MPI_Barrier(MPI_COMM_WORLD);

    bool is_sorted = is_sorted_p(arr[rank], K, rank, P);

    if(rank == MASTER && !is_sorted){
        std::cout<<"ERROR: not sorted." << std::endl;
    } else if(rank == MASTER && is_sorted){
        std::cout<<"SUCCESS: sorted." << std::endl;
    }
    
    // wait for master to display sorted result
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();  // Finalize MPI

    return 0;
}