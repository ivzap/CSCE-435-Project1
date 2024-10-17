#include <iostream>
#include <cstdlib>
#include <ctime>
#include <mpi.h>
#include <math.h>
#include <vector>
// #include <caliper/cali.h>
// #include <caliper/cali-manager.h>
// #include <adiak.hpp>

#define QUICKSORT_THRESHOLD 500
#define LENGTH_PER_SAMPLE 100

int compare(const void *arg1, const void *arg2)
{
  int a1 = *(int *)arg1;
  int a2 = *(int *)arg2;
  if (a1 < a2)
    return -1;
  else if (a1 == a2)
    return 0;
  else
    return 1;
}

// void swap(int *a, int *b)
// {
//   int tmp = *a;
//   *a = *b;
//   *b = tmp;
// }

// void quick_sort(int arr[], unsigned long length)
// {
//   if (length <= 1)
//     return;

//   int random_idx = rand() % length;
//   int pivot = arr[random_idx];
//   swap(&arr[random_idx], &arr[length - 1]);

//   int idx = 0, swap_marker = 0;
//   while (idx < length - 1)
//   {
//     if (arr[idx] > pivot)
//     {
//       idx++;
//       continue;
//     }
//     swap_marker++;
//     if (swap_marker == length - 1)
//       break;
//     if (arr[idx] < arr[swap_marker])
//       swap(&arr[idx], &arr[swap_marker]);
//     else if (arr[idx] == arr[swap_marker])
//       idx++;
//   }
//   quick_sort(arr, swap_marker);
//   if (length > swap_marker + 1)
//     quick_sort(&arr[swap_marker + 1], length - swap_marker - 1);
// }

void sampleSort(int arr[], unsigned long length, MPI_Comm comm, int npes, int rankId)
{
  // CALI_MARK_FUNCTION_BEGIN;

  std::cout << "Unsorted array: ";
  for (unsigned long i = 0; i < length; i++)
  {
    std::cout << arr[i] << " ";
  }
  std::cout << std::endl;

  if (length < QUICKSORT_THRESHOLD)
  {
    std::qsort(arr, (size_t)length, sizeof(int), compare);

    std::cout << "quick sorted first: ";
    for (unsigned long i = 0; i < length; i++)
    {
      std::cout << arr[i] << " ";
    }
    std::cout << std::endl;

    // CALI_MARK_FUNCTION_END;
  }

  // unsigned long num_samples = length / LENGTH_PER_SAMPLE;
  // int *samples = (int *)malloc(sizeof(int) * num_samples);
  int *splitters = (int *)malloc((npes - 1) * sizeof(int));
  int *allpicks = (int *)malloc(npes * (npes - 1) * sizeof(int));

  for (int i = 1; i < npes; i++)
  {
    splitters[i - 1] = arr[i * length / npes];
  }

  std::cout << "splitter test: ";
  for (unsigned long i = 0; i < npes - 1; i++)
  {
    std::cout << splitters[i] << " ";
  }
  std::cout << std::endl;

  // CALI_MARK_BEGIN("sampling");
  // for (unsigned long i = 0; i < num_samples; i++)
  // {
  //   int sample;
  //   bool another_sample;
  //   do
  //   {
  //     another_sample = false;
  //     sample = arr[rand() % length];
  //     for (unsigned long j = 0; j < i; j++)
  //     {
  //       if (samples[j] == sample)
  //       {
  //         another_sample = true;
  //         break;
  //       }
  //     }
  //   } while (another_sample);
  //   samples[i] = sample;
  // }
  // CALI_MARK_END("sampling");

  // std::cout << "Process " << rankId << " samples: ";
  // for (unsigned long i = 0; i < num_samples; i++)
  // {
  //   std::cout << samples[i] << " ";
  // }
  // std::cout << std::endl;

  // CALI_MARK_BEGIN("local_sort_samples");
  // std::qsort(samples, num_samples, sizeof(int), compare);

  // std::cout << "local " << rankId << " samples: ";
  // for (unsigned long i = 0; i < num_samples; i++)
  // {
  //   std::cout << samples[i] << " ";
  // }
  // std::cout << std::endl;

  // CALI_MARK_END("local_sort_samples");

  // int *all_samples = (int *)malloc(sizeof(int) * num_samples * npes);
  // CALI_MARK_BEGIN("all_gather_samples");
  MPI_Allgather(splitters, npes - 1, MPI_INT, allpicks, npes - 1, MPI_INT, comm);
  int *global_splitter = (int *)malloc((npes - 1) * sizeof(int));
  // CALI_MARK_END("all_gather_samples");

  if (rankId == 0)
  {
    std::cout << "Gathered Splitters: ";
    for (unsigned long i = 0; i < (npes - 1) * npes; i++)
    {
      std::cout << allpicks[i] << " ";
    }
    std::cout << std::endl;
  }

  // CALI_MARK_BEGIN("global_sort_samples");

  if (rankId == 0)
  {
    std::qsort(allpicks, (npes - 1) * npes, sizeof(int), compare);
    std::cout << "quick sorted Gathered Splitters: ";
    for (unsigned long i = 0; i < (npes - 1) * npes; i++)
    {
      std::cout << allpicks[i] << " ";
    }
    std::cout << std::endl;

    int idx = ceil(float((npes - 1) * npes) / float(npes));

    for (int i = 1; i < npes; ++i)
    {
      global_splitter[i - 1] = allpicks[idx * (i - 1)];
    }
  }

  MPI_Bcast(global_splitter, npes - 1, MPI_INT, 0, MPI_COMM_WORLD);

  std::cout << "Global Splitters: ";
  for (unsigned long i = 0; i < (npes - 1); i++)
  {
    std::cout << global_splitter[i] << " ";
  }
  std::cout << std::endl;

  int chunk = npes;
  std::vector<int> globalChunkList[chunk];
  std::vector<int> localChunkList[chunk];

  for (int i = 0; i < length; i++)
  {
    for (int j = 0; j < chunk; j++)
    {
      if (arr[i] < global_splitter[j])
      {
        localChunkList[j].push_back(arr[i]);
        break;
      }
    }
  }

  std::cout << "Buckets: ";
  for (unsigned long i = 0; i < chunk; i++)
  {
    for (unsigned long j = 0; j < localChunkList[i].size(); j++)
    {
      std::cout << localChunkList[i].at(j) << " ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;

  std::vector<int> recvcounts(chunk);
  std::vector<int> displs(chunk);

  for (int i = 0; i < chunk; ++i)
  {
    recvcounts[i] = globalChunkList[i].size();
  }

  for (int i = 0; i < chunk; i++)
  {
    // MPI_Allgather(globalChunkList, chunk, MPI_INT,chunk, MPI_INT, comm); ??????????????

    //
    //
    //

    // CALI_MARK_END("global_sort_samples");

    // int *splitters = (int *)malloc(sizeof(int) * (npes - 1));

    // int *scounts = (int *)malloc(sizeof(int) * npes);
    int *sdispls = (int *)malloc(sizeof(int) * npes);
    // int *rcounts = (int *)malloc(sizeof(int) * npes);
    // int *rdispls = (int *)malloc(sizeof(int) * npes);

    // CALI_MARK_BEGIN("bucket_counting");
    // for (int i = 0; i < npes; i++)
    //   scounts[i] = 0;

    // int j = 0;
    // for (unsigned long i = 0; i < length; i++)
    // {
    //   if (arr[i] < splitters[j])
    //   {
    //     scounts[j]++;
    //   }
    //   else
    //   {
    //     while (j < npes - 1 && arr[i] >= splitters[j])
    //       j++;
    //     scounts[j]++;
    //   }
    // }
    // CALI_MARK_END("bucket_counting");

    sdispls[0] = 0;

    // for (int i = 1; i < npes; i++)
    // {
    //   sdispls[i] = sdispls[i - 1] + scounts[i - 1];
    // }

    // CALI_MARK_BEGIN("all_to_all_counts");
    MPI_Gatherv(localChunkList[i].data(), localChunkList[i].size(), MPI_INT, globalChunkList[i].data(),
                recvcounts.data(), sdispls, MPI_INT, 0, MPI_COMM_WORLD);
  }

  if(rankId != 0){
    return;
  }

  std::cout << "global chunck: ";
  for (unsigned long i = 0; i < chunk; i++)
  {
    for (unsigned long j = 0; j < globalChunkList[i].size(); j++)
    {
      std::cout << globalChunkList[i].at(j) << ", ";
    }
    std::cout << std::endl;
  }
  std::cout << std::endl;
  // CALI_MARK_END("all_to_all_counts");

  // rdispls[0] = 0;
  // for (int i = 1; i < npes; i++)
  // {
  //   rdispls[i] = rdispls[i - 1] + rcounts[i - 1];
  // }

  // int total_recv = rdispls[npes - 1] + rcounts[npes - 1];
  // int *recvbuf = (int *)malloc(sizeof(int) * total_recv);

  // // CALI_MARK_BEGIN("all_to_allv_exchange");
  // MPI_Alltoallv(arr, scounts, sdispls, MPI_INT, recvbuf, rcounts, rdispls, MPI_INT, comm);
  // // CALI_MARK_END("all_to_allv_exchange");

  // std::cout << "Process " << rankId << " received data: ";
  // for (unsigned long i = 0; i < total_recv; i++)
  // {
  //   std::cout << recvbuf[i] << " ";
  // }
  // std::cout << std::endl;

  // // CALI_MARK_BEGIN("local_sort_received");
  // std::qsort(recvbuf, total_recv, sizeof(int), compare);

  // // CALI_MARK_END("local_sort_received");

  // int *sorted_arr = nullptr;
  // if (rankId == 0)
  // {
  //   sorted_arr = (int *)malloc(sizeof(int) * length);
  // }

  // // CALI_MARK_BEGIN("gather_results");
  // MPI_Gather(recvbuf, total_recv, MPI_INT, sorted_arr, total_recv, MPI_INT, 0, comm);
  // // CALI_MARK_END("gather_results");

  // if (rankId == 0)
  // {
  //   std::cout << "Final sorted array: ";
  //   for (unsigned long i = 0; i < length; i++)
  //   {
  //     std::cout << sorted_arr[i] << " ";
  //   }
  //   std::cout << std::endl;
  // }

  free(splitters);
  free(allpicks);
  free(splitters);
  // if (rankId == 0)
  // {
  //   free(sorted_arr);
  // }

  // CALI_MARK_FUNCTION_END;
}

int main(int argc, char **argv)
{
  // CALI_MARK_BEGIN("main");
  // cali::ConfigManager mgr;
  // mgr.start();
  int N = 50;

  MPI_Init(&argc, &argv);

  int npes, rankId;
  MPI_Comm_size(MPI_COMM_WORLD, &npes);
  MPI_Comm_rank(MPI_COMM_WORLD, &rankId);

  srand(time(0) + rankId);

  int local_number = N / npes;

  // adiak::init(NULL);
  // adiak::value("algorithm", "SampleSort");
  // adiak::value("programming_model", "MPI");
  // adiak::value("data_type", "int");
  // adiak::value("input_size", length);
  // adiak::value("num_procs", npes);

  // CALI_MARK_BEGIN("data_initialization");
  // int *arr = (int *)malloc(N * sizeof(int));
  int *arr = new int[local_number];
  for (unsigned long i = 0; i < local_number; i++)
  {
    arr[i] = rand() % 100;
  }
  // CALI_MARK_END("data_initialization");

  std::cout << "Process " << rankId << " initial array: ";
  for (unsigned long i = 0; i < local_number; i++)
  {
    std::cout << arr[i] << " ";
  }
  std::cout << std::endl;

  sampleSort(arr, local_number, MPI_COMM_WORLD, npes, rankId);

  free(arr);

  MPI_Finalize();

  // mgr.stop();
  // mgr.flush();
  // CALI_MARK_END("main");

  return 0;
}
