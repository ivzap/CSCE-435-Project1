cmake_minimum_required(VERSION 3.12)

# Find the required packages
find_package(MPI REQUIRED)
find_package(caliper REQUIRED)
find_package(adiak REQUIRED)

# Define the executable
add_executable(sampleComplete sampleComplete.cpp)

# Message output for verification
message(STATUS "MPI includes : ${MPI_INCLUDE_PATH}")
message(STATUS "Caliper includes : ${caliper_INCLUDE_DIR}")
message(STATUS "Adiak includes : ${adiak_INCLUDE_DIR}") 

# Include directories
include_directories(SYSTEM ${MPI_INCLUDE_PATH})
include_directories(${caliper_INCLUDE_DIR})
include_directories(${adiak_INCLUDE_DIR})

# Link libraries
target_link_libraries(sampleComplete PRIVATE MPI::MPI_CXX)
target_link_libraries(sampleComplete PRIVATE caliper)
target_link_libraries(sampleComplete PRIVATE adiak::adiak) 
