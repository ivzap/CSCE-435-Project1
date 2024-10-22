# CSCE 435 Group project

## 0. Group number: 6

## 1. Group members: (Communicate via Discord)
1. Ivan Zaplatar
2. Grant Martinez
3. Hayden O'Keefe
4. Ethan Rutt

## 2. Project topic: Parallel Sorting Algorithms

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)
Our project is to understand and implement different parallel sorting algorithms using the MPI standard. Additionally, we will gather metrics, such as scalablity, and speedup to determine how effective our parallel algorithm is to its sequential counterpart. We will be running all our experiments on the Grace cluster provided by Texas A&M University.

Below is a list of the sorting algorithms we will be implementing and a brief description of each...

- Bitonic Sort: Ethan Rutt
    - A comparison based sorting algorithm which sorts by converting a data-set
      that has `2 ^ k` elements where `k` is a positive integer into a bitonic
      sequence.
        - a bitonic sequence is a sequence of numbers that first increases,
          then decreases
        - formally, there exists an index `i` such that
          `arr[0] <= arr[1] <= arr[2] <= ... <= arr[i]` and
          `arr[i] >= arr[i+1] >= arr[i+2] >= ... >= arr[n-1]`
        - The list is then sorted using a merge function
- Sample Sort: Hayden O'Keefe
    - Sample sort follows a similar guideline to the divide and conquer algorithm quick sort. The sorting algorithm is not divide and conquer, however, unlike quick sort where it choses a pivot to be used to compare with elements of array sample sort has numerous pivots that are determined by the amount of processors that are being utilized. The amount of processors determine how the array will be sectioned off into buckets. Where once the processors has there corresponding buckets it can individually sort there array.
- Merge Sort: Grant Martinez
    - Divide and conquer algorithm that sorts recursively sorts smaller arrays until it reaches a size of 1. Then using comparisions it merges the arrays back together to produce a sorted array.
- Radix Sort: Ivan Zaplatar
    - A non comparative sorting algorithm that is stable and orders elements based on their digits.



### 2b. Pseudocode for each parallel algorithm

### Bitonic Sort - Sequential
* Bitonic Sort is an in-place sorting algorithm, so no additional allocations
  are needed
* This sorting algorithm works by splitting the array until you only have two
  elements (which by definition is a bitonic sequence, if we have 2 elements a
  and b, if a < b then the increasing part is the first two elements, and the
  decreasing part is empty, and if a > b then we have the decreasing part be the
  two elements and the increasing part is empty)
* Once the array is split properly, `bitonic_merge()` swaps if the elements
  aren't consistent with the direction, we want all the lower elements to be on
  the left side and the higher elements to be on the left to get a properly
  sorted list
* By moving them and then doing a `bitonic_merge()` we can end up with a sorted
  list because when the `bitonic_merge()` reaches the bottom of the stack, it
  will make a 2 element increasing bitonic sequence
* [source](https://www.youtube.com/watch?v=uEfieI0MumY)
```python
def bitonic_sort(arr: list[int], low, count, direction):
    def bitonic_merge(arr: list[int], lowIndex, count, direction):
        if (count > 1):
            k = count // 2
            for i in range(lowIndex, lowIndex + k):
                if direction == (arr[i] > arr[i+k]):
                    swap(arr[i], arr[i+k])
            bitonic_merge(arr, lowIndex, k, direction)
            bitonic_merge(arr, lowIndex + k, k, direction)

    if (count > 1):
        k = count // 2
        bitonic_sort(arr, lowIndex, k, 1) # recurse on left half increasing
        bitonic_sort(arr, lowIndex + k, k, 0) # recurse on right half decreasing
        bitonic_merge(arr, lowIndex, count, direction) # merge to actually sort

```

### Bitonic Sort - Parallel
* We can split up the array into sections and then sort each section. The
  bitonic merge can be performed parallel
* The powers of two are very important for this sorting algorithm, because when
  merging, we want to swap with the partner processor that is relevant. After
  sorting, we will swap with the next process over (`2^0`). After the merging
  is done there, we will swap with the process that's two processes over (`2^1`)
  then the process next over (`2^0`). After this, we will repeat with 4
  processes over, then 2 over, then 1 over, etc.
* [source](https://cse.buffalo.edu/faculty/miller/Courses/CSE702/Sajid.Khan-Fall-2018.pdf)
```python
def bitonic_sort_p(local_arr: list[int], p: int):
    def compare_high(x: int):
        MPI_Sendrecv(x) # send my data and receive their data
        bitonic_merge(data, low, count, 1) # merge up

    def compare_low(x: int):
        MPI_Sendrecv(x) # send my data and receive their data
        bitonic_merge(data, low, count, 0) # merge down

    size = p
    rank = MPI_Comm_Rank(MPI_COMM_WORLD, rank)
    # generate elements
    # split into p buckets where p is the number of processors
    # MPI_Scatter to where each process p has n/p elements
    d = log(p) # dimension
    # locally sort local data, either with bitonic sort above or quicksort or
    # whatever
    sort(local_arr)

    # parallel bitonic sort
    for i in range(1, d):
        window-id = most significant (d-i) bits of rank
        for j in range(i, 0):
            if ((window-id % 2 == 0 and jth bit of pk == 0) or
                (window-id % 2 == 1 and jth bit of pk == 1)
            ):
                compare_low(j)
            else:
                compare_high(j)
```

### Radix Sort - Sequential(Naive Allocation)
This version of radix, while easier to understand initially, has a problem. Appending elements to a vector like structure will cause dynamic allocation, which is slow.
```python
def sort_arr_on_digit(arr, d):
        // buckets elements based on digits. Buckets[i] are
        // the elements in the ith digit bucket.

        buckets = {{}*10}

        for i, elm in arr:
            d_digit = (elm / 10**d ) % 10
            buckets[d_digit].append(elm)

        output = []

        for digit in range(0, 10):
            for elm in bucket[digit]:
                output.append(elm)

        return output

def radix_sort(arr):
    max_digits = get_max_digits(arr)

    // lsd to msd
    for d in range(0, max_digits):
        arr = sort_arr_on_digit(arr, d)

    return arr
```

### Radix Sort - Sequential(Better)
```python
def sort_arr_on_digit(arr, d):
        // buckets elements based on digits. Buckets[i] are
        // the elements in the ith digit bucket.

        buckets = {0}*10

        for i, elm in arr:
            d_digit = (elm / 10**d ) % 10
            buckets[d_digit]++

        // allows us to determine the offset our buckets
        // will need to be from the start of arr to place elements
        // tells us how much room we have to place the elements with digit 0
        // as an example.
        for i in range(1, arr.size()):
          buckets[i] += buckets[i-1]

        output = [0]*arr.size()

        // must iterate reverse to maintain the ordering we have found so far
        for i, elm in reverse(arr):
            d_digit = (elm / 10**d ) % 10
            // we use up a spot for this digit in output, do before setting output since 0 indexed
            buckets[d_digit]--
            output[buckets[d_digit]] = elm

        for i, elm in output:
            arr[i] = elm

        return arr

def radix_sort(arr):
    max_digits = get_max_digits(arr)

    // lsd to msd
    for d in range(0, max_digits):
        arr = sort_arr_on_digit(arr, d)

    return arr
```

### Radix Sort - Parallel(Better)
```python
def sort_arr_on_digit(arr, d):
        // MPI workers, master each get a portion of array
        output = [0]*arr.size()
        // buckets elements based on digits. Buckets[i] are
        // the elements in the ith digit bucket.

        buckets = {0}*10

        for i, elm in arr:
            d_digit = (elm / 10**d ) % 10
            buckets[d_digit]++

        // MPI_reduce on buckets to sum up all counts

        // allows us to determine the offset our buckets
        // will need to be from the start of arr to place elements
        // tells us how much room we have to place the elements with digit 0
        // as an example.

        // MPI master will sequentially develop prefix sum
        for i in range(1, arr.size()):
          buckets[i] += buckets[i-1]


        // MPI master, workers will place their elements in their output position
        // must iterate reverse to maintain the ordering we have found so far
        for i, elm in reverse(arr):
            d_digit = (elm / 10**d ) % 10
            // we use up a spot for this digit in output, do before setting output since 0 indexed
            buckets[d_digit]--
            output[buckets[d_digit]] = elm

        // MPI_reduce to collect output results
        for i, elm in output:
            arr[i] = elm

        return arr

def radix_sort(arr):
    max_digits = get_max_digits(arr)

    // lsd to msd
    // MPI master to call sort_arr_on_digit
    for d in range(0, max_digits):
        arr = sort_arr_on_digit(arr, d)

    return arr
```

### Merge Sort - Sequential
```python
def mergesort(arr):
    if length of arr <= 1:
        return
    middle = length of arr / 2
    left = arr[:middle]
    right = arr[middle:]

    sortedLeft = mergesort(left)
    sortedRight = mergesort(right)

    return merge(sortedLeft, sortedRight)

def merge(left, right):
    result = []
    i = 0
    j = 0

    while i < length of left and j < length of right):
        if (left[i] < right[i]):
            append left[i] to result
            i++;
        else:
            append right[j] to result
            j++

    while (i < length of left)
        append left[i] to result
    while (j < length of right):
        append right[j] to result

    return result
```

### Merge Sort - Parallel
* This uses the sequential functions from above
* Read first portion of research paper Parallel Merge Sort with Load Balancing (did not include load balancing)
* [source](https://www.researchgate.net/publication/220091378_Parallel_Merge_Sort_with_Load_Balancing)
```python
def parallelMergeSort(arr):
    # Send partions of arr to each procces possibly using MPI_Scatter
    # local partitions are stored in variable called local

    active_procs = total number of processors

    # sort local partitions in each process
    mergesort(local)

    # P is total number of processors
    # logP - 1 is number of levels in merge tree
    for j = 0 to logP - 1:
        # half of active processors are sending their sorted arrays to be merged other half are recieving and merging
        if (curr_rank < active_procs/2):
            MPI_Recieve partition to merge from processes curr_rank + active_procs/2
            local = merge(local, recieved_partition)
        else :
            MPI_Send local to curr_rank - active_procs/2

        active_procs /= 2

    if curr_rank == 0:
        return local
```

### Sample Sort - Sequential
* [source](https://www.youtube.com/watch?v=WprjBK0p6rw)
* [source](https://learning.oreilly.com/library/view/introduction-to-parallel/0201648652/ch09.html#ch09lev2sec11)
```python
start

def SampleSort(numOfElements, numBuckets)
    define splitter
    define numBuckets

    i = 1
    for 1 < numBuckets
        splitter = numOfElements / numBuckets

    QuickSort(sortedArray, first element, last element)

    new array = appended sorted buckets

    return new array

def QuickSort(sortedaArray, first element, last element)

    //quick sort local buckets
    define pivot = array[last element]

    while i < last element
        if current_index > pivot then
            pass

        if current_index <= pivot then
            swap_marker++
            if current_index > swap_marker then
                swap(current_index, swap_marker)
            else if current_index equals swap_marker then
                pass
        i++

endf
```

### Sample Sort - Parallel
* [source](https://www.youtube.com/watch?v=WprjBK0p6rw)
* [source](https://learning.oreilly.com/library/view/introduction-to-parallel/0201648652/ch09.html#ch09lev2sec11)
```python
start

def SampleSort(numOfElements, communicator)
    define numProcesses
    define rank_id

    // number of processors
    MPI_Comm_size(communicator, &numProcesses)

    // rank of processor
    MPI_Comm_rank(communicator, &rank_id)

    // bucket_size refers to how many elements a processor is required to manage
    define bucket_size = numOfElements / numProcesses

    define local splitters = size of numProcesses
    define collect splitters = numProcesses * (local splitters - 1)

    // sort the buckets
    QuickSort(array, first element, last element)

    idx = (numProcesses - 1)
    for idx < bucket_size
        numsplitters = numOfElements/numProcesses
        divide = idx * numsplitters
        idx++

    // sort the buckets
    QuickSort(array, first element, last element)

    MPI_gather(collect splitters, communicator)

    // sort the buckets
    QuickSort(array, first element, last element)

    MPI_Barrier(collect values, communicator)

    //prefix sum

    for idx < offset
        if P0 < P1_offset
            copy(P0)
        else if P1 < P2_offset
            copy(P1)
        else if P2 < P3_offset
            copy(P2)
        else
            copy(P3)

    MPI_gather(collect splitters, communicator)

     // sort the buckets
    QuickSort(array, first element, last element)

    return array


def QuickSort(array, first element, last element)

    //quick sort local buckets
    define pivot = array[last element]

    while i < last element
        if current_index > pivot then
            pass

        if current_index <= pivot then
            swap_marker++
            if current_index > swap_marker then
                swap(current_index, swap_marker)
            else if current_index equals swap_marker then
                pass
        i++

endf
```



### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types
    - Test sorting algorithms with input size of
        - 2^16, 2^18, 2^20, 2^22, 2^24, 2^26, 2^28
    - Test both `long` and `int`
    - Types of input
        - sorted
        - reverse sorted
        - random
        - 1% swapped
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
- There will be a test on 2, 4, 8, 16, 32, 64, 128, 256, 512, 1024 processors for each input size
- Test of performance and evaluation with utilize Caliper and Thicket to visualize the performance of each algorithm and how they compare to one another. This will also determine which algorithms have their strengths/weaknesses in parallel or sequential methods.

## 3. Implementation Descriptions

#### Radix Sort
I struggled to find any correct implementations of parallel radix sort, so I developed my own. The core algorithm remains the same: find the maximum element, iterate over the digits, count the frequency of each digit, determine where each element should go, and repeat. The challenging part in the parallel version was determining where each element belongs. Specifically, we need to identify which processor to place the element in and its relative position within that processor. 

I introduced a concept I call the "start" variable to handle this. The "start" represents where an element would be placed in a non-parallel version. We then determine the processor by calculating `start / arr_size` and the relative position by `start % arr_size`. 

To compute the start values, we gather the digit counts for each processor. We begin with a base value of 0 and iterate through each digit for every processor, assigning a starting position for each digit based on the base. We then update the base by adding the count of elements for that digit. For example, if we have 10 elements, the new base becomes 10, and any further elements with that digit will be placed starting from this new base. 

At the end of this process, we have a `gathered_cnt[p][digit]` array, which tells each processor where to place elements with a specific digit. After placing an element within a processor, we shift the start value for that processor by one. This part of the algorithm was the most complex. The rest simply involves communicating between processors to place elements in the correct locations. We repeat this for each phase until all digits of the largest element are processed.

#### Sample Sort

Sample Sort is an effective sorting algorithm for working in parallel. I began with implementing the algorithm with having the algorithm check if the provided array was below a threshold and given that it was it would began with using the quick sort algorithm. This quick sort algorithm I fully implemented without using the standard C library to gain a deeper understanding of how the elements in the array were being sorted. It begans by selecting a pivot point and comparing with elements until the random pivot point that was selected reaches the center of the list. Once the center has been reached it splits the array and the same process occurs recursively.
When the array did not meet the threshold the array is split by how many elements there are by how many processors there are to determine how many elements are local to the processor. In other words how many local elements that the processors have to evaluate. These use splitters to divide the array into buckets to be sent to processes. The splitters were gathered to gather all the splitters that were created. The root process then broadcasts the pivots of all the local sections to all the processes. A global bucket list and local bucket list were created to store all the buckets that are evaluated. The local buckets range from specific points divided by the amount of processors even though the over sampling. Once the local buckets are gathered together into the list this list is then gathered using a vector to the global bucket list to be then quick sorted again to have the final array.
This algorithm was immensely challenging for me given the complexity of the algorithm and MPI functionality being challenging. I was unaware that we were allowed to use the standard C library quick sort and implemented the algorithm from scratch at first. I had difficulty particularly with the gather functionality with the buckets. I managed to spend a lot of time on the implementation and attend office hours and get assistance from the TAs. I nearly completed the entire algorithm, however, unfortunately it was the final global bucket sort that I was unable to manage to figure out. My global bucket list was empty and I could not populate it correctly.  

#### Merge Sort
The parallel merge sort algorithm was implmented by having each process sort individual sections of an array then combining them into one sorted array in the final process. The combination of each process is done by combining the local arrays of process i with process i + active_procs / 2. This creates a logarithmic pattern of combination. The number of of active processes starts as n which is the total number of processes avaible, but it is divided in half after each level of combination. As the local arrays combine and become larger, the number of processes that are involved in communication shrinks. By the end, all local arrays have combined into one process. Because the entire array must fit into the memory of one processor in the end, both data initialization and correctness checking are done in one processor. Below is a visual representation of how the processors are combining the subarrays into one main array.
![merge_sort combination of subarrays visualized](https://github.com/user-attachments/assets/744a8fd0-9658-41a7-94ce-93520f12a893)


#### Bitonic Sort
* The parallel bitonic sort algorithm was implemented by having each process
  sort their own local array, turning it into a bitonic sequence (since an
  ascending array is a bitonic sequence with the decreasing part empty), then
  consists of bitonic merging with other processes until the list is sorted.
  This works by the `compare_low()` and `compare_high()` functions. Each
  process has a partner, namely, it's power of 2 partner, so if we have process
  00, their partners will be 10, 100, 1000, depending on how many processes
  there are. These processes will bitonic merge in ascending order. The other
  processes have their partners similarly (process i has partner i ^ j) and
  will order their arrays in descending order. At each step, we are moving the
  lower elements to the ascending side and the bigger elements to the
  descending side so that all elements on the left are smaller than elements on
  the right regardless of order (ascending/descending). On the final iteration,
  all of the processes merge together to form a bitonic sequence with the
  decreasing part empty, i.e. a sorted array.

![bitonic sort communication visualization](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic_sort_image.png)

* [source](https://cse.buffalo.edu/faculty/miller/Courses/CSE702/Sajid.Khan-Fall-2018.pdf)

### 3a. Caliper instrumentation
#### Radix Sort
```c
56.101 main
├─ 0.000 MPI_Init
├─ 0.033 data_init_runtime
│  ├─ 0.006 MPI_Reduce
│  └─ 0.000 MPI_Bcast
├─ 0.167 comp
│  ├─ 0.167 comp_large
│  └─ 0.000 comp_small
├─ 55.225 comm
│  ├─ 0.139 comm_small
│  │  ├─ 0.004 MPI_Gather
│  │  └─ 0.135 MPI_Bcast
│  └─ 55.086 comm_large
│     ├─ 13.466 MPI_Isend
│     ├─ 13.710 MPI_Irecv
│     └─ 7.738 MPI_Waitall
```

#### Merge Sort
```c
3.822 main
├─ 0.000 MPI_Init
├─ 1.547 data_init_runtime
│  └─ 0.071 MPI_Scatter
├─ 0.520 comp
│  └─ 0.520 comp_large
├─ 0.161 comm
│  └─ 0.161 comm_large
│     ├─ 0.068 MPI_Recv
│     └─ 0.023 MPI_Send
└─ 0.176 correctness_check
```

#### Bitonic Sort
```c
2.929 main
├─ 0.000 MPI_Init
├─ 0.012 data_init_runtime
├─ 0.529 MPI_Barrier
├─ 0.903 comp
│  ├─ 0.841 comp_large
│  └─ 0.062 comp_small
├─ 0.055 comm
│  ├─ 0.022 comm_small
│  │  ├─ 0.000 MPI_Send
│  │  └─ 0.021 MPI_Recv
│  └─ 0.014 comm_large
│     ├─ 0.005 MPI_Send
│     └─ 0.009 MPI_Recv
└─ 0.003 correctness_check
   └─ 0.002 MPI_Gather
0.000 MPI_Finalize
0.000 MPI_Initialized
0.000 MPI_Finalized
0.005 MPI_Comm_dup
```

### 3b. Collect Metadata
#### Radix Sort
```
profile	2380433062
cali.caliper.version	2.11.0
mpi.world.size	10
spot.metrics	min#inclusive#sum#time.duration,max#inclusive#...
spot.timeseries.metrics
spot.format.version	2
spot.options	time.variance,profile.mpi,node.order,region.co...
spot.channels	regionprofile
cali.channel	spot
spot:node.order	true
spot:output	p10-a1000000.cali
spot:profile.mpi	true
spot:region.count	true
spot:time.exclusive	true
spot:time.variance	true
launchdate	1728700519
libraries	[/scratch/group/csce435-f24/Caliper/caliper/li...
cmdline	[./radix_p, 1000000]
cluster	c
algorithm	radix
programming_model	mpi
data_type	int
size_of_data_type	4
input_size	10000000
input_type	Random
num_procs	10
scalability	strong
group_num	6
implementation_source	handwritten

```
#### Merge Sort
```
profile	1933867154
cali.caliper.version	2.11.0
mpi.world.size	32
spot.metrics	min#inclusive#sum#time.duration,max#inclusive#...
spot.timeseries.metrics
spot.format.version	2
spot.options	time.variance,profile.mpi,node.order,region.co...
spot.channels	regionprofile
cali.channel	spot
spot:node.order	true
spot:output	p32-a67108864-tr.cali
spot:profile.mpi	true
spot:region.count	true
spot:time.exclusive	true
spot:time.variance	true
launchdate	1728924056
libraries	[/scratch/group/csce435-f24/Caliper/caliper/li...
cmdline	[./mergesort, 67108864, r]
cluster	c
algorithm	merge
programming_model	mpi
data_type	double
size_of_data_type	8
input_size	67108864
input_type	Random
num_procs	32
scalability	strong
group_num	6
implementation_source	online
```

#### Bitonic Sort
```
profile	3944471742
cali.caliper.version	2.11.0
mpi.world.size	32
spot.metrics	min#inclusive#sum#time.duration,max#inclusive#...
spot.timeseries.metrics
spot.format.version	2
spot.options	time.variance,profile.mpi,node.order,region.co...
spot.channels	regionprofile
cali.channel	spot
spot:node.order	true
spot:output	p32-a16777216.cali
spot:profile.mpi	true
spot:region.count	true
spot:time.exclusive	true
spot:time.variance	true
launchdate	1729093173
libraries	[/scratch/group/csce435-f24/Caliper/caliper/li...
cmdline	[./bitonic_sort, -t, random, -n, 16777216, -p,...
cluster	c
algorithm	bitonic
programming_model	mpi
data_type	int
size_of_data_type	4
input_size	16777216
input_type	random
num_procs	32
scalability	strong
group_num	6
implementation_source	online
```
## 4. Performance evaluation

### Merge Sort
This implementation of merge sort has inherent limitations when it comes to parallelization. These limitations come from the fact that as the combination of subarrays occurs, the number of active processors decreases. When fewer processors are doing work, the burden of work on those processors increases. In the very last step of the algorithm two halves of the initial input are combined into one array. Thus, the algorithm is limited by memory. This implementation holds two arrays of size `input_size`. Because the algorithm was implemented using doubles (64 bits) the largest input that could be given to the merge sort algorithm was 2^26. We made the assumption that each process has 4GB (2^35) of memory available (originally 8GB but have to account for libraries like MPI). The calculations are show below:
* (size of type) $\times$ (input size) $\times$ (number of arrays held in memory) = memory used
* 64 $\times$ 2^26 $\times$ 2 = 2^33
* 64 $\times$ 2^28 $\times$ 2 = 2^35

These calculations show that the memory gets filled up when running with an input size of 2^28. 

For the parallel merge sort algorithm, it is expected that we will not see major differences in the various input types. This is becuase the data is sent and received as well as traversed and "sorted" regardless of how it comes in.

When doing performance analysis for merge sort the Max time/rank was used instead of the average. Due to the algorithms nature of communication the computation and communication performance is dictated by the Max time. The data from each process is conjoined into one process in the end. This is where we expect to see the maximum occur which gives us the correct run time of every process.

The figures below show the runtime of the computation portion of the algorithm as a funciton of the number of processors used. The four input types are present on each graph.
![image](https://github.com/user-attachments/assets/4a48a2d8-f1ab-464a-b555-bcfd8c597d92)
![image](https://github.com/user-attachments/assets/0de334d8-f077-4f57-9da6-ce43bf400291)
![image](https://github.com/user-attachments/assets/e55e02ec-cf26-41b1-bcc4-e796e1f4e405)
![image](https://github.com/user-attachments/assets/21578d28-429a-43e9-9c79-737562ad3e10)
![image](https://github.com/user-attachments/assets/7f511a59-9251-4a22-85cc-5bf713077d45)
![image](https://github.com/user-attachments/assets/3e446911-369e-4dab-b0bf-80d3187999e1)

It can clearly be seen that the computation time decreases very rapidly as the number of processors increase initially, but as the number of processors is continually increased, the number computation time approaches a limit. From this it can be stated that once the number of processors increases past 8 or 16 the benefit in performance is seemingly negligible. We can also see that there is almost no difference among the different types of input as expected.

The figures below show the runtime of the communication portion of the algorithm as a function of the number of processors used. The four input types are present on each graph
![image](https://github.com/user-attachments/assets/0a297547-160a-40a7-952f-0fc6b6dfde74)
![image](https://github.com/user-attachments/assets/382cb4a6-3c4d-48d7-af45-90371741aebd)
![image](https://github.com/user-attachments/assets/4d462e8a-09a4-47e5-8b92-fdd02963a982)
![image](https://github.com/user-attachments/assets/b8ba31c8-bf39-4df1-89d9-cfe7fc5dbf33)
![image](https://github.com/user-attachments/assets/2ce8a3a3-bf00-4871-b498-5e2a94a28664)
![image](https://github.com/user-attachments/assets/71fc7f82-072a-4337-bba9-576f45a39019)

These graphs are slightly more tricky to read compared to the computation graphs. This is due to the outliers present from the data collected. Multiple rounds of data tests were run as an attempt to remove these outlier points but they still remained. They occur mostly with the 128 and 256 processor tests, and it can not be said which input types they occur for most often. Aside from these outliers we do see a trend in the commuication time. For input sizes, 2^16 and 2^18 the communication times are similar (despite the outliers). This is expected for the smaller input sizes. However, as the input size increases to 2^20 and 2^22 that first processor increase plays a more significant role than the later ones. The overhead for communication is initially significant but eventually tapers off. So as the processors begin to increases we see a larger difference until about 16 or 32 processors where it levels off. Finally for the largest input sizes of 2^24 and 2^26 (2^28 was not available for merge sort as referenced above) the outliers make less of an inpact and the trend can be seen better. The dramatic increase in communication time in the beginning is again due to the initial overhead of communication. However we dont see a large difference between the communication times for 8, 16, 32, and 64 processors. This later jump in 128 and 256 can possibly be explained by the increase in the number of nodes needed for communication. It makes sense that communication within a node is faster and cheaper than communication to extneral nodes. When we reach 64 processors and above we require more and more nodes. 
