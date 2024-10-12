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
* [source](https://people.cs.rutgers.edu/~venugopa/parallel_summer2012/mpi_bitonic.html)
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
        for j in range(i-1, 0):
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
