#include "mpi.h"
#include <algorithm>
#include <cmath>
#include <limits.h>
#include <stdlib.h>
#include <vector>
#include <stdlib.h>
#include <getopt.h>

#include <adiak.hpp>
#include <caliper/cali-manager.h>
#include <caliper/cali.h>

/******************************************************************************
 * Start Helper Function Headers
 ******************************************************************************/
/**
 * checks if the local arrays are sorted
 *
 * @param arr_chunk the local chunk of the array
 * @param arr_size the size of the array
 * @param rank the rank of the calling process
 * @param p the amount of processes
 */
bool is_sorted_p(int* arr_chunk, int arr_size, int rank, int p);

/**
 * get_most_significant_n_bits() gets the most significant n bits starting at
 * the most significant bit thats set of the number num
 *
 * @param num the number to get the most significant bits from
 * @param num_of_bits the number of bits to get
 * @return the most significant n bits
 */
int get_most_significant_n_bits(int num, int num_of_bits);

/**
 * bitonic_merge() merges an array according to the bitonic sort. Merging
 * happens in place
 *
 * @param arr the array to merge
 * @param low the low index to start merging from
 * @param count the number of elements to merge
 * @param direction the direction to merge (increasing or decreasing). 1 for
 * increasing 0 for decreasing
 */
void bitonic_merge(int arr[], int low, int count, bool direction);

/**
 * compare() is a parallel merging function for bitonic sort
 *
 * @param rank_to_send_to the processes rank to MPI_Sendrecv to
<<<<<<< HEAD
 * @param rank the processes rank that is calling the function
 * @param arr the array to be sorted
 * @param num_elements number of elements in the array
=======
 * @param arr the array to be sorted
>>>>>>> c3d17e34b3d03427141a0b3cee64d2717945ceaf
 * @param low the low index to start merging from
 * @param count the number of elements to merge
 * @param direction the direction to merge (increasing or decreasing). 1 for
 * increasing 0 for decreasing
 */
void compare(int rank_to_send_to, int rank, int arr[], int num_elements, int low, int count,
             bool direction);

/**
 * bitonic_sort_p() is a parallel sorting algorithm that implements bitonic sort
 *
 * @param arr the array to be sorted
 * @param num_elements the number of elements in the array
 * @param total_processes the total number of processes
 * @param rank this processes rank in the MPI_COMM_WORLD
 */
void bitonic_sort_p(int arr[], int num_elements, int total_processes, int rank);

/******************************************************************************
 * End Helper Function Headers
 ******************************************************************************/

int main(int argc, char *argv[]) {
/******************************************************************************
 * Start Command Line Option Parsing
 ******************************************************************************/
char* input_type = nullptr;
int num_elements = -1;
int num_procs = -1;
int opt;

while ((opt = getopt(argc, argv, "t:n:p:")) != -1) {
	switch(opt) {
		case 't':
			input_type = optarg;
			break;
		case 'n':
			num_elements = atoi(optarg);
			break;
		case 'p':
			num_procs = atoi(optarg);
			break;
		default:
			std::cout << "Unknown option received, exiting" << std::endl;
			return 1;
	}
}

if (input_type == nullptr || num_elements == -1 || num_procs == -1) {
	std::cerr << "Command line arguments are needed for this program to function. Please indicate the input type: {random, sorted, reverse, perturbed}, the number of elements: {int}, and the number of processes {int}." << std::endl;
	return 1;
}
/******************************************************************************
 * End Command Line Option Parsing
 ******************************************************************************/
/******************************************************************************
 * Start Adiak Metadata
 ******************************************************************************/
  char* adiak_algorithm = "bitonic sort";
  char* adiak_programming_model = "mpi";
  char* adiak_data_type = "int";
  char* adiak_size_of_data_type = "4";
  char* adiak_input_type = input_type;
  char* adiak_input_size;
  sprintf(adiak_input_size, "%d", num_elements);
  char* adiak_num_procs;
  sprintf(adiak_num_procs, "%d", num_procs);
  char* adiak_scalability = "strong";
  char* adiak_group_number = "6";
  char* adiak_implementation_source = "handwritten";

  adiak::init(NULL);
  adiak::launchdate();                  // launch date of the job
  adiak::libraries();                   // Libraries used
  adiak::cmdline();                     // Command line used to launch the job
  adiak::clustername();                 // Name of the cluster
  adiak::value("algorithm", adiak_algorithm); // The name of the algorithm you are
                                        // using (e.g., "merge", "bitonic")
  adiak::value("programming_model", adiak_programming_model); // e.g. "mpi"
  adiak::value(
      "data_type",
      adiak_data_type); // The datatype of input elements (e.g., double, int, float)
  adiak::value("size_of_data_type",
               adiak_size_of_data_type); // sizeof(datatype) of input elements in
                                   // bytes (e.g., 1, 2, 4)
  adiak::value("input_size",
               adiak_input_size); // The number of elements in input dataset (1000)
  adiak::value("input_type",
               adiak_input_type); // For sorting, this would be choices: ("Sorted",
                            // "ReverseSorted", "Random", "1_perc_perturbed")
  adiak::value("num_procs", adiak_num_procs); // The number of processors (MPI ranks)
  adiak::value("scalability",
               adiak_scalability); // The scalability of your algorithm. choices:
                             // ("strong", "weak")
  adiak::value("group_num",
               adiak_group_number); // The number of your group (integer, e.g., 1, 10)
  adiak::value("implementation_source",
               adiak_implementation_source); // Where you got the source code of your
                                       // algorithm. choices: ("online", "ai",
                                       // "handwritten").
/******************************************************************************
 * End Adiak Metadata
 ******************************************************************************/

/******************************************************************************
 * Start MPI Initialization
 ******************************************************************************/
int rank, num_tasks;
MPI_Init(&argc, &argv);
MPI_Comm_rank(MPI_COMM_WORLD, &rank);
MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);

if (num_procs != num_tasks) {
	std::cerr << "num procs passed in as a cmdline arg doesn't equal MPI tasks" << std::endl;
	return 1;
}
/******************************************************************************
 * End MPI Initialization
 ******************************************************************************/

/******************************************************************************
 * Start Data Generation
 ******************************************************************************/
int elements_per_proc = num_elements / num_tasks;

int input[num_elements];

for (int i = 0; i < num_elements; i++) {
	input[i] = rand() % 2000;
}

int local[num_elements];
MPI_Scatter(input, elements_per_proc, MPI_INT, local, elements_per_proc, MPI_INT, 0, MPI_COMM_WORLD);

/******************************************************************************
 * End Data Generation
 ******************************************************************************/
/******************************************************************************
 * Start Bitonic Sort
 ******************************************************************************/
bitonic_sort_p(local, elements_per_proc, num_tasks, rank);
/******************************************************************************
 * End Bitonic Sort
 ******************************************************************************/
/******************************************************************************
 * Start Correctness Check
 ******************************************************************************/
MPI_Barrier(MPI_COMM_WORLD);
bool sorted = is_sorted_p(local, num_elements, rank, num_tasks);
if (rank == 0 && sorted) {
	std::cout << "SUCCESS" << std::endl
}
else if (rank == 0 && !sorted) {
	std::cerr << "Failed sorting" << std::endl;
	return 1;
}
/******************************************************************************
 * End Correctness Check
 ******************************************************************************/
MPI_Barrier(MPI_COMM_WORLD);
MPI_Finalize();
return 0;
}

/******************************************************************************
 * Start Helper Function Definitions
 ******************************************************************************/
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

int get_most_significant_n_bits(int num, int num_of_bits) {
  if (num == 0) {
    return 0;
  }

  int most_significant_set_bit = std::floor(log2(num));
  int shift_amount = most_significant_set_bit - num_of_bits + 1;

  if (shift_amount < 0) {
    shift_amount = 0;
  }

  int result = num >> shift_amount;
  result &= (1 << num_of_bits) - 1;

  return result;
}

void bitonic_merge(int arr[], int low, int count, bool direction) {
  int k;
  if (count <= 1) {
    return;
  }

  k = count / 2;

  for (int i = low; i < low + k; i++) {
    if (direction == (arr[i] > arr[i + k])) {
      std::swap(arr[i], arr[i + k]);
    }
  }

  bitonic_merge(arr, low, k, direction);
  bitonic_merge(arr, low + k, k, direction);
}

void compare(int rank_to_send_to, int rank, int arr[], int num_elements, int low, int count,
             bool direction) {
  int send[num_elements];
  int recv[num_elements];
  MPI_Sendrecv(send, num_elements, MPI_INT, rank_to_send_to, 0, recv, num_elements, MPI_INT, rank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  bitonic_merge(arr, low, count, direction);
}

void bitonic_sort_p(int arr[], int num_elements, int total_processes, int rank) {
  int dimension = static_cast<int>(log2(total_processes));
  std::sort(arr[0], arr[num_elements]);

  for (int i = 1; i < dimension; i++) {
    int num_bits = dimension - i;
    int window_id = get_most_significant_n_bits(rank, num_bits);

    for (int j = i - 1; j > 0; j--) {
      if (((window_id % 2 == 0) && (rank >> j == 0)) ||
          (window_id % 2 == 1) && (rank >> j == 1)) {
        compare(j, arr, 0, num_elements, 0);
      } else {
        compare(j, arr, 0, num_elements, 1);
      }
    }
  }
}

/******************************************************************************
 * End Helper Function Definitions
 ******************************************************************************/
