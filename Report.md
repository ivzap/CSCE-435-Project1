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
I will now explain how I implemented radix sort parallelized using the MPI API. 

I first looked up the sequential sorting algorithm for radix and studied it heavily, making sure I understood what it was doing. Next I looked up online implementations for parallel radix but couldn't find anything, well anything that was of any good quality, pretty much everyone did it incorrectly. I got tired of reading bad code so I then started brainstorming how I could come up with the parallel version of radix sort myself. There were a couple of obstacles, the main ones were:
1. How to "index" into other arrays, i.e if I was in processor i and wanted to communicate something to processor i+1 how would I do such a thing?
2.  How to find where we should place a element?

I solved these both by realizing that each digit will have some starting position to place elements from. For example if the digit was 0 then we would place this element at the very left of the array. We also know that each digit in some stage will have a "area" in the array. So if we had ten 0s found for some stage then we know for sure that these 10 elements fit in the addresses 0-9, where the 10th address represents the next digits start position. If the next digit didn't have any digits for that bucket then we would have the start represent the start of the next non zero frequency digit. I repeat this for every digit.

This can all be visualized below, where the '^' character represents the start position for each digit:

```
arr = [8,1,1,0,0,0,0,1,1,9,9,9]
single sort stage...
000011118999 
^   ^   ^^
```

The only problem is now the array is split so we need to be able to answer the question: If I have a element with digit x on processor p where do I place it?

We can solve this by creating a global prefix start. Where prefix_sum\[p]\[d] represents the starting position for process p and digit d. The reason this is called a prefix sum is because we must adjust the start of prefix_sum\[p+1]\[d] by how many integers were in the bucket prefix_sum\[p]\[d], effectively giving precedence to the digits in the smaller processors which is what we want inorder to maintain order.

Now we know in O(1) time **where** something must be placed and its relative position in that processor.

Now to actually change the position of some element we do start / arr_size where arr_size is the number of elements in each process. The relative position in that process is found by doing start % arr_size. Once we do this for every element that needs to be moved in the current process to another we use MPI_Send() to distribute it across other processes. This step used up alot of memory because I sent three integers (processor_dest, relative_position, value). Again I also should have used a batching system and MPI_Send() more frequently to reduce memory usage.

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

#### Sample Sort
```c
0.550 main
├─ 0.000 MPI_Init
├─ 0.000 data_init_runtime
├─ 0.006 comm
│  ├─ 0.003 comm_large
│  │  ├─ 0.002 MPI_Scatter
│  │  ├─ 0.000 MPI_Gather
│  │  └─ 0.001 MPI_Gatherv
│  └─ 0.003 comm_small
│     ├─ 0.003 MPI_Bcast
│     └─ 0.000 MPI_Allgather
├─ 0.000 MPI_Barrier
├─ 0.001 comp
│  ├─ 0.001 comp_large
│  └─ 0.000 comp_small
└─ 0.002 correctness_check
0.002 MPI_Comm_dup
0.000 MPI_Send
0.000 MPI_Comm_free
0.000 MPI_Recv
0.000 MPI_Finalize
0.000 MPI_Probe
0.000 MPI_Initialized
0.000 MPI_Get_count
0.000 MPI_Finalized
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

### Radix Sort
My radix sort implementation overall shows that it was able to succesfully sort in parallel various input size but also showed some of its limitations. The first limitation is that a high amount of memory will be required when the processor arrays arent sorted, as each one element that must be placed in a different processor must be communicated to such processor. During experimentation I frequently saw for low processor count that MPI would run out of memory as each processor requires more and more memory. To avoid this limitation we could reimpliment the algorithm to communicate data in batches as this would reduce the stress on the internal MPI message buffers and our local memory. Another limitation comes from the fact that we need to create a duplicate temp array which can double our memory usage, this cannot be avoided as we need somewhere to store the elements without modifing the current array. Other issues I saw were with large processes, I would get HYDRA errors, but it turns out this is due to a faulty network for the cluster.

### Radix
#### Speedup
The speedup for radix sort was by far the best out of all the algorithms we implemented. Radix sort uses a non comparative algorithm, which means that there is less dependency between elements which in turn means less computation and communication. Less computation means less clock cycles which means the cpu can spend time on other useful things, and the reduction of communication reduces network i/o overheard.

However, speedup is not everything, as can be seen in the Total Time figures. That is while radix had the best speedup it has the largest sort time compared to the others. It required significant resources to be able to eventually compete with the other sorting algorithms speeds. Its also worthy to mention that my implementation was not the most optimal. I came up with it during a couple sessions of thinking. If interested, I explain how I came up with my solution later in the report.

![image](https://github.com/user-attachments/assets/3baf5cdf-68f8-473f-9658-525ac8ad2d97)
![image](https://github.com/user-attachments/assets/d0d5ebba-a7b6-4f7b-96ed-f8167eddb01d)
![image](https://github.com/user-attachments/assets/dc44c36b-e80d-4499-8cd1-d27ecd6c52b6)


#### Total Time
As mentioned within the speedup, total time was quite high for radix sort. The large amount of sort time is highly attributed to two things:
1. Memory management
2. Network communication

My implementation was not perfect by any means as I did not optimize my memory usage. I would prepare a worst case fixed buffer of jobs to send to different workers which told the worker to put a value somewhere within its current array which would be the new sorted position by the current digit. Creating a fixed buffer is not optimal with respect to memory usage as it has an average memory usage of the worst possible case, its constant. Instead what I should have done was to implement a batching system where I could batch the jobs allowing each process to only require a small amount of memory at a time instead up piling up a massive job buffer. The consequence of this part of my implementation was missing data points in my experiments as I would frequently run out of memory on a small number of processors. The smaller the process the more data it must hold in memory before communicating which also resulted in MPI running out of memory frequently.

An example of MPI running out of memory can be seen in the Figure 2. of this section. Notice how **only** the sorted input was able to run on the smaller processors. This is because if something is sorted I **do not** communicate with the other processors using MPI thus we avoid MPI running out of memory because it doesn't need to maintain any internal buffers. 

![image](https://github.com/user-attachments/assets/812dd26f-e5b3-44df-b472-e6825f836cb3)
![image](https://github.com/user-attachments/assets/58b5b5c3-ed10-4af1-8164-0daec8c2c0bb)


#### Weak Scaling
My weak scaling graphs show some of the communication and memory overheads I was talking about in prior sections. As we distribute the work across more processors we continue to decline in average time per rank. As we increased the input size and most notably with 2^26 we saw that MPI or Grace would run out of memory. It is more likely that we are simply running out of memory on grace as even sorted has problems getting data points for the smaller processor counts with large input sizes. Sorted inputs have almost no communication as they are already in sorted order as such no changes need to take place.

The nature of our weak scaling plots can be best described as a exponential decay. As we increased the number of processors, the average time per rank decreased. This matches are previous claims about having a high speedup.

![image](https://github.com/user-attachments/assets/a707a008-c72a-4346-a55d-37aeea3f4626)
![image](https://github.com/user-attachments/assets/388606f7-056b-49b9-b752-bd11c9613d1b)
![image](https://github.com/user-attachments/assets/1640423c-f49f-4201-9a38-098d0925b8bd)




### Merge Sort
This implementation of merge sort has inherent limitations when it comes to parallelization. These limitations come from the fact that as the combination of subarrays occurs, the number of active processors decreases. When fewer processors are doing work, the burden of work on those processors increases. In the very last step of the algorithm two halves of the initial input are combined into one array. Thus, the algorithm is limited by memory. This implementation holds two arrays of size `input_size`. Because the algorithm was implemented using doubles (64 bits) the largest input that could be given to the merge sort algorithm was 2^26. We made the assumption that each process has 4GB (2^35) of memory available (originally 8GB but have to account for libraries like MPI). The calculations are show below:
* (size of type) $\times$ (input size) $\times$ (number of arrays held in memory) = memory used
* 64 $\times$ 2^26 $\times$ 2 = 2^33
* 64 $\times$ 2^28 $\times$ 2 = 2^35

These calculations show that the memory gets filled up when running with an input size of 2^28.

For the parallel merge sort algorithm, it is expected that we will not see major differences in the various input types. This is becuase the data is sent and received as well as traversed and "sorted" regardless of how it comes in. However, we do see a slight increase in computation time for the random input type. This is due to the high volume of `std::swap` operations required.

When doing performance analysis for merge sort the Max time/rank was used instead of the average. Due to the algorithms nature of communication the computation and communication performance is dictated by the Max time. The data from each process is conjoined into one process in the end. This is where we expect to see the maximum occur which gives us the correct run time of every process.

#### Computation
The figures below show the runtime of the computation portion of the algorithm as a funciton of the number of processors used. The four input types are present on each graph.
![merge_strong_comp_2^16](https://github.com/user-attachments/assets/210c699a-ccac-43e3-8f7f-6b4535c11fe3)
![merge_strong_comp_2^18](https://github.com/user-attachments/assets/b51fcf0d-d393-4be3-8b6d-3c39d3ad6a1c)
![merge_strong_comp_2^20](https://github.com/user-attachments/assets/726eada5-09b4-4062-9cec-c2a8d7622256)
![merge_strong_comp_2^22](https://github.com/user-attachments/assets/5c774375-1f58-4c38-9f85-820e34763849)
![merge_strong_comp_2^24](https://github.com/user-attachments/assets/e0171ce3-afdd-4c2c-83c4-da25e1d5108b)
![merge_strong_comp_2^26](https://github.com/user-attachments/assets/8a904deb-833a-458d-892d-172c9553cab8)


It can clearly be seen that the computation time decreases very rapidly as the number of processors increase initially, but as the number of processors is continually increased, the computation time approaches a limit. From this it can be stated that once the number of processors increases past 16 the benefit in performance is seemingly negligible. We can also see that the random input type has a slightly higher computation time than the rest of the input types. As the number of processors increases the random input type converges with the rest. Finally, it is important to note that as the input size increases, so does the computation time as expected.

#### Communication
The figures below show the runtime of the communication portion of the algorithm as a function of the number of processors used. The four input types are present on each graph

![merge_strong_comm_2^16](https://github.com/user-attachments/assets/ef482ea0-cff3-46e2-999a-12b2d5bd85d7)
![merge_strong_comm_2^18](https://github.com/user-attachments/assets/0b888d1b-f7fd-4fed-93c7-2d5cb5743c72)
![merge_strong_comm_2^20](https://github.com/user-attachments/assets/4e781697-8451-418f-93de-a9a7bd56ca92)
![merge_strong_comm_2^22](https://github.com/user-attachments/assets/51b91892-19b1-4c45-8231-d46933d50dda)
![merge_strong_comm_2^24](https://github.com/user-attachments/assets/bd97c8e7-3dab-4f5c-b505-7ca05de4b4a6)
![merge_strong_comm_2^26](https://github.com/user-attachments/assets/624379bb-2eaa-466f-b014-63325c5e9316)


These graphs are slightly more tricky to read compared to the computation graphs. This is due to the outliers present from the data collected. Multiple rounds of data tests were run as an attempt to remove these outlier points but they still remained. They occur mostly with the 128 and 256 processor tests, and it can not be said which input types they occur for most often. Aside from these outliers we do see a trend in the commuication time. For input sizes, 2^16, 2^18, 2^20, and 2^22 the communication times are similar (despite the outliers). This is expected for the smaller input sizes. However, as the input size increases to 2^24 and 2^26 we see an increase in communication time. This increase is still very uniform up until about 64 processors. This is when we jump to using many more nodes than before. Communication between nodes takes much longer than communication within one node. This is why we see so many wild data points at 64 processors and above. The graphs show a larger communication overhead for the larger input sizes. This makes sense as we are sending larger amounts of data. It can also be seen that communication time does not play a significant role in the algorithm when compared to computation (especially as input size increases) as merge sort does at most 10 communications in a single process (this occurs when we use 1024 processors). The input type has no affect on communication overhead, as the number of communications is logarithmically determined by the number of processors.


#### Total Time
By comparing the computation and communication times it is clear that the merge sort algorithm is dependent on the computation time for larger input sizes and communication for smaller input sizes. Below are graph showing the total time of the algorithm vs the number of processors. This is the computation time plus the communication time.
![merge_strong_total_2^16](https://github.com/user-attachments/assets/49d7aeb9-1541-47c6-be04-9786daf2daae)
![merge_strong_total_2^18](https://github.com/user-attachments/assets/d3da5507-ffec-4167-8805-fc794c79d51a)
![merge_strong_total_2^20](https://github.com/user-attachments/assets/6e4347a4-2b21-418a-acb9-8cd97651e1f6)
![merge_strong_total_2^22](https://github.com/user-attachments/assets/e0ad551f-b4ea-4ddb-bc2b-78481f2c6315)
![merge_strong_total_2^24](https://github.com/user-attachments/assets/4f01f346-751e-4ca8-afc9-0848dfcb5b5e)
![merge_strong_total_2^26](https://github.com/user-attachments/assets/c1bfa9f7-c0b2-4859-91c3-62eabcf8688c)


For input sizes of 2^26, 2^18, and 2^20 the graphs look very similar to those above in the Communication section. This confirms the idea that in lower input sizes the communication overhead makes a larger inpact on the overall run time. However, as we get to the larger inputs of 2^22, 2^24, and 2^26, we see the graphs become more and more similar to the computation graphs. This is because at the larger input sizes, the computation portion of the algorithm takes far longer than the communication overhead does.

#### Input Size Comparison
Below are the figures that compare the computation, communication, and total times for a random input type on different input sizes.
![merge_comp_weak](https://github.com/user-attachments/assets/8f280772-423f-4cf8-82ef-ab9b0ae9bc2c)
![merge_comm_weak](https://github.com/user-attachments/assets/85eebb05-811f-4633-be47-645957de8008)
![merge_total_weak](https://github.com/user-attachments/assets/aa8558f2-b1b5-4314-a72c-1713df1d49e5)

These plots confirm what was stated above, the larger input sizes have a significantly longer runtime than the smaller input sizes. It can also be inferred from these plots that the computaiton time is greater than communication time.

#### Speedup


The speedup of the merge sort algorithm was calculated by taking the total time (which is the time it would take to run on one processor) and divding it by the max time/rank. Below are the graphs for the speedup vs the number of processors.
![merge_speedup_2^16](https://github.com/user-attachments/assets/fd77689e-fc6b-4e72-b91a-7a77c203cfd6)
![merge_speedup_2^18](https://github.com/user-attachments/assets/5e9e990a-c856-4e80-99da-ff0cc799af0f)
![merge_speedup_2^20](https://github.com/user-attachments/assets/663bd2a3-91a3-40ed-83e9-8ca4a22a0b59)
![merge_speedup_2^22](https://github.com/user-attachments/assets/b495a279-fd58-4fd8-b545-069106b5c431)
![merge_speedup_2^24](https://github.com/user-attachments/assets/5af65f56-5c59-4b23-8ccd-8f24acf2453e)
![merge_speedup_2^26](https://github.com/user-attachments/assets/d753cf10-8781-471b-b64d-954d91696e42)


From these plots, it can be seen that the speedup increases  with the number of processes. It does not seem like we have hit a limitation for the parallelization of merge sort, however we are still limited by processor memory size (same as sequential merge sort). It is notable that on the 2^16 and 2^18 inputs sizes there seems to be more descrepancies in the speedup between the different input types, but I do not think this is caused by the change in input types. The speedups for each input size are roughly the same (maxing out around 600).


Based on the graphs merge sort is a very parallelizable algorithm because it is computation heavy without a large requirement for communication. It's biggest limitation is the processor's memory. This does not allow parallel merge sort to sort anything above its sequential equivalent.

## Bitonic Sort
* My bitonic sort implementation had some peculiar behavior for high
  processors, showing that even with large input sizes, not using 1024
  processors might still be faster. I suspect this is because I use quicksort
  to sort locally, and then bitonic merging is done between processes. Just
  using quicksort might be faster than worrying about the heavy overhead for
  communicating with lots of different processes. `comp` was still lower with
  more processors since each processor was sorting less elements. My algorithm
  was able to run 1024 processors with 2 ** 28 elements with only 8 GB of
  memory, but due to various issues with grace including `hydra` and `oom`
  errors that popped up in various areas (even as low as 32 processors, so I
  don't think it was a memory issue since I can run 2 ** 28 and 1024 processors
  successfully).
* Due to hydra issues, I wasn't able to collect all the data points but for the
most part, everything is there

### Strong
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong/strong_65536.png)
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong/strong_262144.png)
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong/strong_1048576.png)
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong/strong_4194304.png)
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong/strong_16777216.png)
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong/strong_67108864.png)
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong/strong_268435456.png)

### Strong Speedup
#### 2 ** 16
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_65536_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_65536_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_65536_main.png)

#### 2 ** 18
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_262144_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_262144_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_262144_main.png)


#### 2 ** 20
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_1048576_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_1048576_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_1048576_main.png)

#### 2 ** 22
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_4194304_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_4194304_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_4194304_main.png)

#### 2 ** 24
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_16777216_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_16777216_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_16777216_main.png)

#### 2 ** 26
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_67108864_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_67108864_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_67108864_main.png)

#### 2 ** 28
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_268435456_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_268435456_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/strong_speedup/strong_speedup_268435456_main.png)

### Weak
#### sorted
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_sorted_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_sorted_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_sorted_main.png)

#### random
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_random_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_random_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_random_main.png)

#### reversed
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_reversed_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_reversed_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_reversed_main.png)

#### perturbed
##### comm
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_perturbed_comm.png)
##### comp
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_perturbed_comp.png)
##### main
![image](https://github.com/ivzap/CSCE-435-Project1/blob/main/images/bitonic/weak/weak_perturbed_main.png)

## Algorithm Comparisons

The following two graphs compare the speedup and overall time of all the algorithms. Its important to not only look at the speedup of an algorithm but the total time because, as we will see, radix while it has an excellent linear speedup it has the worst sort time compared to all the other algorithms. 

### Bitonic
Since bitonic sorting is designed for parallelization, the speedup was significant, as we only communicate parts of the local array, except in the worst-case scenario. The communication overhead was approximately balanced with the computation by further splitting the array, resulting in a linear speedup as the number of processors increased.

### Merge
The algorithm exhibited linear speedup up to a certain threshold, after which it began to show diminishing returns. This is because merge sort is logarithmic in nature, and as we increase the number of processors, more processors become underutilized as the algorithm progresses. Consequently, we observe a drop-off in performance as the number of processors increases.

### Sample
Initially, we observed a gradual decrease in average time per rank, but once the algorithm reached around 512 processes, there was a significant drop. This could be due to load imbalances, where too many processors result in each holding only a very small amount of elements. Each of these processors must then communicate with a global list, further increasing communication overhead. Additionally, this issue could be caused by poor implementation of communicators, such as not using an extra barrier to ensure all processors are synchronized.

### Radix
Radix sorting exhibits a highly linear speedup because it is a non-comparative algorithm, which reduces computation overhead and, to some extent, the need for communication between processors. Radix sorting also distributed work evenly across multiple processors, further supporting the substantial speedup as we increased the number of processors. While the speedup was impressive, the total time to sort remained high. This highlights the importance of not evaluating an algorithm based solely on a single statistic.

![image](https://github.com/user-attachments/assets/d0ed7aea-3a57-4ba2-81b9-aea5c2001afe)

![image](https://github.com/user-attachments/assets/5e74b3e8-bc0c-4867-af1b-6f518de87c68)


