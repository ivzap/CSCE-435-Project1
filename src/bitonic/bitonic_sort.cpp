#include "mpi.h"
#include <algorithm>
#include <getopt.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

#define MASTER 0

int rank;
int num_procs = -1;
int num_elements = -1;
int elements_per_proc;
int *local;

/**
 * checks if the arrays are sorted using MPI
 *
 * first, locally checked, then gathered all last elements to make sure they are
 * consistent
 *
 * credit to Ivan Zaplatar
 */
bool is_sorted_p() {
  // verify each local arr_chunk is sorted then the border or p+1 first element
  for (int i = 0; i < elements_per_proc - 1; i++) {
    if (local[i] > local[i + 1]) {
      return false;
    }
  }

  int *last_elements = (int *)malloc(num_procs * sizeof(int));
  MPI_Gather(&local[elements_per_proc - 1], 1, MPI_INT, last_elements, 1,
             MPI_INT, MASTER, MPI_COMM_WORLD);
  if (rank == MASTER) {
    // check the last element of each segment to ensure the overall order
    for (int i = 0; i < num_procs - 1; i++) {
      if (last_elements[i] > last_elements[i + 1]) {
        free(last_elements);
        return false;
      }
    }
    free(last_elements);
    return true;
  }
  free(last_elements);
  return false;
}

/**
 * compares and merges at the low end (meaning in ascending order)
 *
 * First the max in my array is sent to the partner, and the min is received
 * from the partner. After that, all numbers above the min are sent, and all
 * numbers below the max are received. A bitonic merge then happens on these
 * numbers so that all of the lower numbers are in this process in ascending
 * order.
 *
 * credit to Sajid Khan
 * https://cse.buffalo.edu/faculty/miller/Courses/CSE702/Sajid.Khan-Fall-2018.pdf
 */
void compare_low(int j) {
  CALI_MARK_BEGIN("comm");
  int min;

  int send_counter = 0;
  CALI_MARK_BEGIN("comm_small");
  int *buffer_send = (int *)malloc((elements_per_proc + 1) * sizeof(int));
  MPI_Send(&local[elements_per_proc - 1], 1, MPI_INT, rank ^ (1 << j), 0,
           MPI_COMM_WORLD);

  int recv_counter;
  int *buffer_recieve = (int *)malloc((elements_per_proc + 1) * sizeof(int));
  MPI_Recv(&min, 1, MPI_INT, rank ^ (1 << j), 0, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);
  CALI_MARK_END("comm_small");

  for (int i = elements_per_proc - 1; i >= 0; i--) {
    if (local[i] > min) {
      send_counter++;
      buffer_send[send_counter] = local[i];
    } else {
      break;
    }
  }

  CALI_MARK_BEGIN("comm_large");
  buffer_send[0] = send_counter;
  MPI_Send(buffer_send, send_counter + 1, MPI_INT, rank ^ (1 << j), 0,
           MPI_COMM_WORLD);

  MPI_Recv(buffer_recieve, elements_per_proc + 1, MPI_INT, rank ^ (1 << j), 0,
           MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  CALI_MARK_END("comm_large");
  CALI_MARK_END("comm");

  CALI_MARK_BEGIN("comp");
  CALI_MARK_BEGIN("comp_small");
  int *temp_array = (int *)malloc(elements_per_proc * sizeof(int));
  for (int i = 0; i < elements_per_proc; i++) {
    temp_array[i] = local[i];
  }

  int buffer_size = buffer_recieve[0];
  int k = 1;
  int m = 0;

  k = 1;
  for (int i = 0; i < elements_per_proc; i++) {
    if (temp_array[m] <= buffer_recieve[k]) {
      local[i] = temp_array[m];
      m++;
    } else if (k <= buffer_size) {
      local[i] = buffer_recieve[k];
      k++;
    }
  }
  CALI_MARK_END("comp_small");

  CALI_MARK_BEGIN("comp_large");
  std::sort(local, local + elements_per_proc);
  CALI_MARK_END("comp_large");
  CALI_MARK_END("comp");

  free(buffer_send);
  free(buffer_recieve);

  return;
}

/**
 * compares and merges at the high end (meaning in descending order)
 *
 * First the min in my array is sent to the partner, and the max is received
 * from the partner. After that, all numbers below the max are sent, and all
 * numbers above the min are received. A bitonic merge then happens on these
 * numbers so that all of the higher numbers are in this process in descending
 * order.
 *
 * credit to Sajid Khan
 * https://cse.buffalo.edu/faculty/miller/Courses/CSE702/Sajid.Khan-Fall-2018.pdf
 */
void compare_high(int j) {
  CALI_MARK_BEGIN("comm");
  int max;

  int recv_counter;
  int *buffer_recieve = (int *)malloc((elements_per_proc + 1) * sizeof(int));

  CALI_MARK_BEGIN("comm_small");
  MPI_Recv(&max, 1, MPI_INT, rank ^ (1 << j), 0, MPI_COMM_WORLD,
           MPI_STATUS_IGNORE);

  int send_counter = 0;
  int *buffer_send = (int *)malloc((elements_per_proc + 1) * sizeof(int));
  MPI_Send(&local[0], 1, MPI_INT, rank ^ (1 << j), 0, MPI_COMM_WORLD);
  CALI_MARK_END("comm_small");

  for (int i = 0; i < elements_per_proc; i++) {
    if (local[i] < max) {
      send_counter++;
      buffer_send[send_counter] = local[i];
    } else {
      break;
    }
  }


  CALI_MARK_BEGIN("comm_large");
  MPI_Recv(buffer_recieve, elements_per_proc + 1, MPI_INT, rank ^ (1 << j), 0,
           MPI_COMM_WORLD, MPI_STATUS_IGNORE);
  recv_counter = buffer_recieve[0];

  buffer_send[0] = send_counter;

  MPI_Send(buffer_send, send_counter + 1, MPI_INT, rank ^ (1 << j), 0,
           MPI_COMM_WORLD);
  CALI_MARK_END("comm_large");
  CALI_MARK_END("comm");

  int *temp_array = (int *)malloc(elements_per_proc * sizeof(int));

  CALI_MARK_BEGIN("comp");
  CALI_MARK_BEGIN("comp_small");
  for (int i = 0; i < elements_per_proc; i++) {
    temp_array[i] = local[i];
  }

  int k = 1;
  int m = elements_per_proc - 1;
  int buffer_size = buffer_recieve[0];

  for (int i = elements_per_proc - 1; i >= 0; i--) {
    if (temp_array[m] >= buffer_recieve[k]) {
      local[i] = temp_array[m];
      m--;
    } else if (k <= buffer_size) {
      local[i] = buffer_recieve[k];
      k++;
    }
  }
  CALI_MARK_END("comp_small");

  CALI_MARK_BEGIN("comp_large");
  std::sort(local, local + elements_per_proc);
  CALI_MARK_END("comp_large");
  CALI_MARK_END("comp");

  free(buffer_send);
  free(buffer_recieve);

  return;
}

int main(int argc, char *argv[]) {
  CALI_MARK_BEGIN("main");

  cali::ConfigManager mgr;
  mgr.start();
  /******************************************************************************
   * MPI_Init
   ******************************************************************************/
  MPI_Init(&argc, &argv);
  MPI_Comm_size(MPI_COMM_WORLD, &num_procs);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  /******************************************************************************
   * Command Line Option Parsing
   ******************************************************************************/
  char *input_type = nullptr;
  int opt;

  while ((opt = getopt(argc, argv, "t:n:p:")) != -1) {
    switch (opt) {
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
      printf("Unknown option received, exiting \n");
      return 1;
    }
  }

  if (input_type == nullptr || num_elements == -1 || num_procs == -1) {
    printf("Command line arguments are needed for this program to function. "
           "Please indicate the input type: {random, sorted, "
           "reverse, perturbed}, the number of elements: {int}, and the number "
           "of processes {int}.");
    return 1;
  }
  /******************************************************************************
   * Data Generation
   ******************************************************************************/
  CALI_MARK_BEGIN("data_init_runtime");
  elements_per_proc = num_elements / num_procs;
  local = (int *)malloc(elements_per_proc * sizeof(int));

  std::srand(time(NULL) + rank);

  for (int i = 0; i < elements_per_proc; i++) {
    local[i] = rand() % 20000;
  }

  CALI_MARK_END("data_init_runtime");
  MPI_Barrier(MPI_COMM_WORLD);
  /******************************************************************************
   * Local Sort
   ******************************************************************************/

  CALI_MARK_BEGIN("comp");
  CALI_MARK_BEGIN("comp_large");
  std::sort(local, local + elements_per_proc);
  CALI_MARK_END("comp_large");
  CALI_MARK_END("comp");
  /******************************************************************************
   * Bitonic Merge with Other Ranks
   ******************************************************************************/
  int dimension = static_cast<int>(std::log2(num_procs));

  // Note that caliper calls are inside compare_low() and compare_high()
  for (int i = 0; i < dimension; i++) {
    for (int j = i; j >= 0; j--) {
      if (((rank >> (i + 1)) % 2 == 0 && (rank >> j) % 2 == 0) ||
          ((rank >> (i + 1)) % 2 != 0 && (rank >> j) % 2 != 0)) {
        compare_low(j);
      } else {
        compare_high(j);
      }
    }
  }

  MPI_Barrier(MPI_COMM_WORLD);
  /******************************************************************************
   * Correctness Check
   ******************************************************************************/
  CALI_MARK_BEGIN("correctness_check");
  bool is_sorted = is_sorted_p();

  if (rank == MASTER && !is_sorted) {
    printf("ERROR: not sorted properly\n");
  } else if (rank == MASTER && is_sorted) {
    printf("SUCCESS: sorted\n");
  }
  CALI_MARK_END("correctness_check");
  /******************************************************************************
   * Cleanup
   ******************************************************************************/
  free(local);
  CALI_MARK_END("main");

  /******************************************************************************
   * Adiak Metadata
   ******************************************************************************/
  std::string adiak_input_type = std::string(input_type);

  adiak::init(NULL);
  adiak::launchdate();  // launch date of the job
  adiak::libraries();   // Libraries used
  adiak::cmdline();     // Command line used to launch the job
  adiak::clustername(); // Name of the cluster
  adiak::value("algorithm",
               "bitonic"); // The name of the algorithm you are
                                 // using (e.g., "merge", "bitonic")
  adiak::value("programming_model", "mpi"); // e.g. "mpi"
  adiak::value("data_type",
               "int"); // The datatype of input elements (e.g.,
                                 // double, int, float)
  adiak::value("size_of_data_type",
               "4"); // sizeof(datatype) of input
  adiak::value(
      "input_size",
      num_elements); // The number of elements in input dataset (1000)
  adiak::value(
      "input_type",
      adiak_input_type); // For sorting, this would be choices: ("Sorted",
                         // "ReverseSorted", "Random", "1_perc_perturbed")
  adiak::value("num_procs",
               num_procs); // The number of processors (MPI ranks)
  adiak::value(
      "scalability",
      "strong"); // The scalability of your algorithm. choices:
                          // ("strong", "weak")
  adiak::value(
      "group_num",
      "6"); // The number of your group (integer, e.g., 1, 10)
  adiak::value("implementation_source",
               "online"); // Where you got the source code

  mgr.stop();
  mgr.flush();

  MPI_Finalize();

  return 0;
}