#include <mpi.h>
#define MASTER 0
#define K 2 // the subset size each processor gets
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
        // once finished, give all processes the gathered_cnt for starts
        MPI_Bcast(gathered_cnt, P * 10, MPI_INT, MASTER, MPI_COMM_WORLD);
    }

    // have all processes wait until master is done.
    MPI_Barrier(MPI_COMM_WORLD);

    // each process will now construct a "job list" of (pos_in_part, value) pairs
    // and send it over the wire to the correct processor
    int output[arr_size] = {0};
    for(int i = 0; i < arr_size; i++){
        int digit = (arr_chunk[i] / exp) % 10;
        int start = gathered_cnt[rank][digit]++;
        // find out which process we are placing this element in
        int p = start / arr_size; 
        int rel_pos = start % arr_size;
        

    }
    // wait for all processes to communicate work
    MPI_Barrier(MPI_COMM_WORLD);

    // process the work that was sent from other processes

    // wait for everyone to finish then move onto the next exp
    MPI_Barrier(MPI_COMM_WORLD);
}


int main(int argc, char** argv){

    



    return 0;
}