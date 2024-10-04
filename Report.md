# CSCE 435 Group project

## 0. Group number: 6

## 1. Group members: (Communicate via Discord)
1. Ivan Zaplatar
2. Grant Martinez
3. Hayden O'Keefe
4. Fourth

## 2. Project topic: Parallel Sorting Algorithms

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)
Our project is to understand and implement different parallel sorting algorithms using the MPI standard. Additionally, we will gather metrics, such as scalablity, and speedup to determine how effective our parallel algorithm is to its sequential counterpart. We will be running all our experiments on the Grace cluster provided by Texas A&M University. 

Below is a list of the sorting algorithms we will be implementing and a brief description of each...

- Bitonic Sort: Ethan Rutt
    - A comparison based sorting algorithm which sorts by converting a data-set
      that has `2 ^ k` elements where k is a positive integer into a bitonic
      sequence.
        - a bitonic sequence is a sequence of numbers that first increases,
          then decreases
        - formally, there exists an index i such that 
          arr[0] <= arr[1] <= arr[2] <= ... <= arr[i] and 
          arr[i] >= arr[i+1] >= arr[i+2] >= ... >= arr[n-1]
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
  two elements and the decreasing part is empty)
* Once the array is split properly, `bitonic_merge()` swaps if the elements
  aren't consistent with the direction, we want all the lower elements to be on
  the left side and the higher elements to be on the left to get a properly
  sorted list
* By moving them and then doing a `bitonic_merge()` we can end up with a sorted
  list because when the `bitonic_merge()` reaches the bottom of the stack, it
  will make a 2 element increasing bitonic sequence
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
* We can split up the array 
```python
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
```python
def main:
    arr = input array to be sorted
    MPI_Scatter to distribute arr across all processes store in local

    if Master process:
        mergesort(local)
        for each process:
            MPI_Recv sorted portion of arr
            local = merge(local, received portion)
    else:
        local = mergesort(local)
        MPI_Send local
    
```

### Sample Sort - Sequential
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

    // splitters (pivots) for each individual bucket to reference in quicksort
    define local splitters = size of numProcesses
    // collect splitters
    define collect splitters = numProcesses * (local splitters - 1)

    // sort the buckets utilizing quick sort
    QuickSort(array, first element, last element)

    idx = (numProcesses - 1)
    for idx < bucket_size
        numsplitters = numOfElements/numProcesses 
        divide = idx * numsplitters
        idx++

    // sort the buckets
    QuickSort(array, first element, last element)

    // collect all splitters to account for global array
    MPI_gather(collect splitters, communicator)

    // sort the buckets
    QuickSort(sorted_array, first element, last element)

    return sorted_array


def QuickSort(sorted_array, first element, last element)

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
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
