# Define the compiler
CXX = mpic++

# Define compiler flags
CXXFLAGS = -O2 -Wall

# Define the target executable name
TARGET = radix_p

# Define source files
SRCS = radix_p.cpp

# Define the build directory
BUILD_DIR = build

# Create the object file list from the source files
OBJS = $(SRCS:.cpp=.o)

# Default rule to build the target
all: $(TARGET)

# Rule to link the target executable
$(TARGET): $(OBJS)
	@mkdir -p $(BUILD_DIR)
	$(CXX) $(CXXFLAGS) -o $(BUILD_DIR)/$(TARGET) $(OBJS)

# Rule to compile .cpp files into .o object files
%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# Clean rule to remove object files and the executable
clean:
	rm -rf $(OBJS) $(BUILD_DIR)

# Phony targets
.PHONY: all clean
