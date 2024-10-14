#include "mpi.h"
#include <algorithm>
#include <cmath>
#include <limits.h>
#include <stdlib.h>
#include <vector>

#include <adiak.hpp>
#include <caliper/cali-manager.h>
#include <caliper/cali.h>

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
void bitonic_merge(std::vector<int> arr, int low, int count, bool direction);

/**
 * compare() is a parallel merging function for bitonic sort
 *
 * @param rank_to_send_to the processes rank to MPI_Sendrecv to
 * @param arr the array to be sorted
 * @param low the low index to start merging from
 * @param count the number of elements to merge
 * @param direction the direction to merge (increasing or decreasing). 1 for
 * increasing 0 for decreasing
 */
void compare(int rank_to_send_to, std::vector<int> arr, int low, int count,
             bool direction);

/**
 * bitonic_sort_p() is a parallel sorting algorithm that implements bitonic sort
 *
 * @param arr the array to be sorted
 * @param total_processes the total number of processes
 * @param rank this processes rank in the MPI_COMM_WORLD
 */
void bitonic_sort_p(std::vector<int> arr, int total_processes, int rank);

int main(int argc, char *argv[]) {
  int numtasks, taskid, source, dest, mtype;
  MPI_Init(&argc, &argv);
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

void bitonic_merge(std::vector<int> arr, int low, int count, bool direction) {
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

void compare(int rank_to_send_to, std::vector<int> arr, int low, int count,
             bool direction) {
  // MPI_Sendrev(rank_to_send_to, ...);
  bitonic_merge(arr, low, count, direction);
}

void bitonic_sort_p(std::vector<int> arr, int total_processes, int rank) {
  int dimension = static_cast<int>(log2(total_processes));
  std::sort(arr.begin(), arr.end());

  for (int i = 1; i < dimension; i++) {
    int num_bits = dimension - i;
    int window_id = get_most_significant_n_bits(rank, num_bits);

    for (int j = i - 1; j > 0; j--) {
      if (((window_id % 2 == 0) && (rank >> j == 0)) ||
          (window_id % 2 == 1) && (rank >> j == 1)) {
        compare(j, arr, 0, count, 0);
      } else {
        compare(j, arr, 0, count, 1);
      }
    }
  }
}
