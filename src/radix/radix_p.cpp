#include <mpi.h>
#include <iostream>
#include <vector>

#define MASTER 0
#define K 4 // the subset size each processor gets
#define P 4 // the number of processors 


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

    std::vector<std::vector<int>> send_buffers;

    std::vector<MPI_Request> send_requests(K);
    
    for(int i = 0; i < arr_size; i++){
        int digit = (arr_chunk[i] / exp) % 10;
        int start = gathered_cnt[rank][digit]++;
        // find out which process we are placing this element in
        int p = start / arr_size; 
        int rel_pos = start % arr_size;

        send_buffers.push_back({p, rel_pos, arr_chunk[i]});
    }

    // wait for all processes to get work
    MPI_Barrier(MPI_COMM_WORLD);

    // signal other processes jobs to perform
    for (int i = 0; i < K; i++) {
        int dest_p = send_buffers[i][0];
        MPI_Isend(send_buffers[i].data(), send_buffers[i].size(), MPI_INT, dest_p, 0, MPI_COMM_WORLD, &send_requests[i]);
    }
    // wait for all processes to send work on wire
    MPI_Barrier(MPI_COMM_WORLD);

    int recv_buffer[K][3] = {0};

    MPI_Request recv_requests[K];

    for (int i = 0; i < K; i++) {
        MPI_Irecv(recv_buffer[i], 3, MPI_INT, MPI_ANY_SOURCE, 0, MPI_COMM_WORLD, &recv_requests[i]);
    }

    // Wait for all receive operations to complete
    MPI_Waitall(K, send_requests.data(), MPI_STATUSES_IGNORE);
    MPI_Waitall(K, recv_requests, MPI_STATUSES_IGNORE);

    int output[arr_size] = {0};
    for(int i = 0; i < K; i++){
        int pos = recv_buffer[i][1];
        int elm = recv_buffer[i][2];
        output[pos] = elm;
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // once everyone is done update our current array
    for(int i = 0; i < arr_size; i++){
        arr_chunk[i] = output[i];
    }

    MPI_Barrier(MPI_COMM_WORLD);
}


int main(int argc, char** argv){
    MPI_Init(&argc, &argv);  // Initialize MPI

    int rank, size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    int arr[P][K] = {0};
    arr[0][0] = 14;
    arr[0][1] = 3;
    arr[0][2] = 14;
    arr[0][3] = 3;

    arr[1][0] = 5;
    arr[1][1] = 5;
    arr[1][2] = 7;
    arr[1][3] = 5;

    arr[2][0] = 14;
    arr[2][1] = 1;
    arr[2][2] = 14;
    arr[2][3] = 1;

    arr[3][0] = 0;
    arr[3][1] = 0;
    arr[3][2] = 1;
    arr[3][3] = 0;

    int max_val = 14;

    int sorted_arr[P*K] = {-99999};

    for (int exp = 1; max_val / exp > 0; exp *= 10) {
        counting_sort(arr[rank], K, exp, rank);
    }

    // Wait for all processes before final output
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Gather(arr[rank], K, MPI_INT, sorted_arr, K, MPI_INT, 0, MPI_COMM_WORLD);

    if(rank == MASTER){
        for(int n: sorted_arr){
            std::cout << n << " ";
        }
        std::cout << std::endl;
    }

    // wait for master to display sorted result
    MPI_Barrier(MPI_COMM_WORLD);

    MPI_Finalize();  // Finalize MPI

    return 0;
}