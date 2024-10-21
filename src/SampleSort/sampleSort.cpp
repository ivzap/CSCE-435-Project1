#include <cstdlib>
#include <ctime>
#include <iostream>
#include <math.h>
#include <mpi.h>
#include <vector>
#include <caliper/cali.h>
#include <caliper/cali-manager.h>
#include <adiak.hpp>

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

void sampleSort(int arr[], unsigned long length, MPI_Comm comm, int npes,
                int rankId, int out[])
{
    CALI_MARK_BEGIN("sample_sort");

    int NoofElements = length;
    int elemPerSec = NoofElements / npes;

    if ((NoofElements % npes) != 0)
    {
        if (rankId == 0)
            printf("Number of Elements are not divisible by npes \n");
        MPI_Finalize();
        exit(0);
    }
    if (NoofElements < 3)
    {
        if (rankId == 0)
            printf("Number of Elements should be greater than three \n");
        MPI_Finalize();
        exit(0);
    }

    int *InputData = (int *)malloc(elemPerSec * sizeof(int));
    if (InputData == NULL)
    {
        printf("InputData Fail!\n");
        MPI_Finalize();
        exit(1);
    }

    MPI_Scatter(arr, elemPerSec, MPI_INT, InputData,
                elemPerSec, MPI_INT, 0, MPI_COMM_WORLD);

    int *splitters = (int *)malloc((npes - 1) * sizeof(int));
    if (splitters == NULL)
    {
        printf("Splitters Fail!\n");
        MPI_Finalize();
        exit(1);
    }

    for (int i = 1; i < npes; i++)
    {
        splitters[i - 1] = arr[i * length / npes];
    }

    int *global_splitter = (int *)malloc((npes - 1) * npes * sizeof(int));
    if (global_splitter == NULL)
    {
        printf("Global_splitter Fail\n");
        MPI_Finalize();
        exit(1);
    }

    MPI_Gather(splitters, npes - 1, MPI_INT, global_splitter, npes - 1, MPI_INT,
               0, MPI_COMM_WORLD);

    if (rankId == 0)
    {
        std::qsort(global_splitter, (npes - 1) * npes, sizeof(int), compare);

        for (int i = 1; i < npes - 1; ++i)
        {
            splitters[i] = global_splitter[(npes - 1) * (i + 1)];
        }
    }
    MPI_Bcast(global_splitter, npes - 1, MPI_INT, 0, MPI_COMM_WORLD);

    int *Buckets = (int *)malloc(sizeof(int) * (NoofElements + npes));

    int j = 0;
    int k = 1;

    for (unsigned long i = 0; i < elemPerSec; i++)
    {
        if (j < (npes - 1))
        {
            if (InputData[i] < splitters[j])
                Buckets[((elemPerSec + 1) * j) + k++] = InputData[i];
            else
            {
                Buckets[(elemPerSec + 1) * j] = k - 1;
                k = 1;
                j++;
                i--;
            }
        }
        else
            Buckets[((elemPerSec + 1) * j) + k++] = InputData[i];
    }
    Buckets[(elemPerSec + 1) * j] = k - 1;

    int *BucketBuffer = (int *)malloc(sizeof(int) * (NoofElements + npes));

    MPI_Alltoall(Buckets, elemPerSec + 1, MPI_INT, BucketBuffer,
                 elemPerSec + 1, MPI_INT, MPI_COMM_WORLD);

    int *LocalBucket = (int *)malloc(sizeof(int) * 2 * NoofElements / npes);

    int count = 1;

    for (j = 0; j < npes; j++)
    {
        k = 1;
        for (unsigned long i = 0; i < BucketBuffer[(NoofElements / npes + 1) * j]; i++)
            LocalBucket[count++] = BucketBuffer[(NoofElements / npes + 1) * j + k++];
    }
    LocalBucket[0] = count - 1;

    int NoElementsToSort = LocalBucket[0];
    qsort((char *)&LocalBucket[1], NoElementsToSort, sizeof(int), compare);

    int *OutputBuffer = NULL;
    int *Output = NULL;
    if (rankId == 0)
    {
        OutputBuffer = (int *)malloc(sizeof(int) * 2 * NoofElements);
        Output = (int *)malloc(sizeof(int) * NoofElements);
    }

    MPI_Gather(LocalBucket, 2 * elemPerSec, MPI_INT, OutputBuffer,
               2 * elemPerSec, MPI_INT, 0, MPI_COMM_WORLD);

    if (rankId == 0)
    {
        count = 0;
        for (j = 0; j < npes; j++)
        {
            k = 1;
            for (unsigned long i = 0; i < OutputBuffer[(2 * NoofElements / npes) * j]; i++)
                Output[count++] = OutputBuffer[(2 * NoofElements / npes) * j + k++];
        }

        FILE *fp = fopen("sort.out", "w");
        if (fp == NULL)
        {
            printf("Can't Open Output File \n");
            exit(0);
        }

        fprintf(fp, "Number of Elements to be sorted : %d \n", NoofElements);
        for (unsigned long i = 0; i < NoofElements; i++)
        {
            fprintf(fp, "%d\n", Output[i]);
        }

        fclose(fp);

        free(OutputBuffer);
        free(Output);
    }

    free(InputData);
    free(splitters);
    free(global_splitter);
    free(Buckets);
    free(BucketBuffer);
    free(LocalBucket);

    CALI_MARK_END("sample_sort");
}

int main(int argc, char **argv)
{
    CALI_MARK_BEGIN("main");
    cali::ConfigManager mgr;
    mgr.start();
    MPI_Init(&argc, &argv);

    int npes, rankId;
    MPI_Comm_size(MPI_COMM_WORLD, &npes);
    MPI_Comm_rank(MPI_COMM_WORLD, &rankId);

    int N = 50;
    int *arr = nullptr;

    if (rankId == 0)
    {
        arr = (int *)malloc(N * sizeof(int));

        adiak::init(NULL);
        adiak::value("algorithm", "SampleSort");
        adiak::value("programming_model", "MPI");
        adiak::value("data_type", "int");
        adiak::value("input_size", N);
        adiak::value("num_procs", npes);

        srand(time(0) + rankId);

        for (unsigned long i = 0; i < N; i++)
        {
            arr[i] = rand() % 100;
        }
    }

    int *out = (int *)malloc(N * sizeof(int));
    if (out == NULL)
    {
        if (rankId == 0)
        {
            free(arr);
        }
        MPI_Finalize();
        return 1;
    }

    sampleSort(arr, N, MPI_COMM_WORLD, npes, rankId, out);

    if (rankId == 0)
    {
        free(arr);
    }
    free(out);

    mgr.stop();
    mgr.flush();
    CALI_MARK_END("main");

    MPI_Finalize();
    return 0;
}
