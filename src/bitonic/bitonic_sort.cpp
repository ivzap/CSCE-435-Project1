#include "mpi.h"
#include <algorithm>
#include <adiak.hpp>
#include <caliper/cali-manager.h>
#include <caliper/cali.h>
#include <getopt.h>
#include <ctime>

int elements_per_proc, rank, num_tasks, curr_partner;

void compare(int curr, int other, int up) {
	int curr_rank = curr / elements_per_proc;
	int partner_rank = other / elements_per_proc;

	int curr_offset = curr % elements_per_proc;
	int partner_offset = other % elements_per_proc;

	if (rank == 0) {
		std::cout << "comparing between " << curr_rank << " and " << partner_rank << " ranks" << std::endl;
	}

	if ((curr_rank != rank) && (partner_rank != rank)) {
		return;
	}
	
	if ((curr_rank == rank) && (partner_rank == rank)) {
		if (((up == 0) && (local[curr_offset] > local[partner_offset])) || ((up != 0) && (local[curr_offset] < local[partner_offset]))) {
			std::swap(local + curr_offset, local + partner_offset);
		}
		return;
	}

	if (curr_rank == rank && rank_partner != rank) {
		if (cu
		
	}
}


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
	std::cout << "command line parsed successfully" << std::endl;
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
	char adiak_input_size[20];
	sprintf(adiak_input_size, "%d", num_elements);
	char adiak_num_procs[20];
	sprintf(adiak_num_procs, "%d", num_procs);
	char* adiak_scalability = "strong";
	char* adiak_group_number = "6";
	char* adiak_implementation_source = "online";

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
	std::cout << "Metadata added successfully" << std::endl;
	/******************************************************************************
	 * End Adiak Metadata
	 ******************************************************************************/
	/******************************************************************************
	 * Start MPI Initialization
	 ******************************************************************************/
	MPI_Init(&argc, &argv);
	MPI_Comm_rank(MPI_COMM_WORLD, &rank);
	MPI_Comm_size(MPI_COMM_WORLD, &num_tasks);

	if (num_procs != num_tasks) {
		std::cerr << "num procs passed in as a cmdline arg doesn't equal MPI tasks" << std::endl;
		return 1;
	}
	std::cout << "mpi init success" << std::endl;
	/******************************************************************************
	 * End MPI Initialization
	 ******************************************************************************/

	/******************************************************************************
	 * Start Data Generation
	 ******************************************************************************/
	elements_per_proc = num_elements / num_tasks;
	int local[elements_per_proc];
	int partner[elements_per_proc];

	std::srand(rank);
	for (int i = 0; i < elements_per_proc; i++) {
		local[i] = std::rand() % 2000;
	}
	MPI_Barrier(MPI_COMM_WORLD);
	std::cout << "data gen success" << std::endl;

	/******************************************************************************
	 * End Data Generation
	 ******************************************************************************/
	if (rank == 0) {
		for (int i = 0; i < elements_per_proc; i++) {
			std::cout << "local[" << i << "]: " << local[i] << std::endl;
		}
	}


	/******************************************************************************
	 * Start Computation Small
	 ******************************************************************************/
	std::sort(local, local + elements_per_proc);

	if (rank == 0) {
		std::cout << "after sort" << std::endl;
		for (int i = 0; i < elements_per_proc; i++) {
			std::cout << "local[" << i << "]: " << local[i] << std::endl;
		}
	}
	/******************************************************************************
	 * End Computation Small
	 ******************************************************************************/
	/******************************************************************************
	 * Start Computation Large
	 ******************************************************************************/
	for (int i = 2; i <= num_procs; i *= 2) {
		for (int j = i >> 1; i > 0; i >>= 1) {
			for (int k = 0; k < num_procs; k++) {
				int partner_rank = i ^ j;
				if (partner_rank > i) {
					compare(k, partner_rank, k&i);
				}
			}
		}
	}

	delete[] local;
	delete[] partner;

	return 0;
}


