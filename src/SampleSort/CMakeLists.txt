cmake_minimum_required(VERSION 3.12)

# Find the necessary packages
find_package(MPI REQUIRED)
find_package(caliper REQUIRED)
find_package(adiak REQUIRED)

# Create the executable
add_executable(sampleComplete sampleComplete.cpp)

# Output messages to verify the paths of the included libraries
message(STATUS "MPI includes : ${MPI_INCLUDE_PATH}")
message(STATUS "Caliper includes : ${caliper_INCLUDE_DIR}")
message(STATUS "Adiak includes : ${adiak_INCLUDE_DIRS}")

# Add include directories for MPI, Caliper, and Adiak
include_directories(SYSTEM ${MPI_INCLUDE_PATH})
include_directories(${caliper_INCLUDE_DIR})
include_directories(${adiak_INCLUDE_DIRS})

# Link the necessary libraries to the target executable
target_link_libraries(sampleComplete
    PRIVATE MPI::MPI_CXX
    PRIVATE caliper
    PRIVATE adiak
)
