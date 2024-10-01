# CSCE 435 Group project

## 0. Group number: 

## 1. Group members:
1. Ivan Zaplatar
2. Second
3. Third
4. Fourth

## 2. Project topic (e.g., parallel sorting algorithms)

### 2a. Brief project description (what algorithms will you be comparing and on what architectures)

- Bitonic Sort:
- Sample Sort:
- Merge Sort:
- Radix Sort: Ivan Zaplatar



### 2b. Pseudocode for each parallel algorithm
- For MPI programs, include MPI calls you will use to coordinate between processes


#### Radix Sort - Sequential(Naive Allocation)
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

#### Radix Sort - Sequential(Better)
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

#### Radix Sort - Parallel(Better)
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


### 2c. Evaluation plan - what and how will you measure and compare
- Input sizes, Input types
- Strong scaling (same problem size, increase number of processors/nodes)
- Weak scaling (increase problem size, increase number of processors)
