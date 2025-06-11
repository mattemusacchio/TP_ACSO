# Compiler settings - Can change to clang++ if preferred
CXX = g++
CXXFLAGS = -std=c++11 -Wall -pthread -g

# Build targets
TARGET = threadpool
TARGET_TEST = threadpool_test
TARGET_CUSTOM_TEST = threadpool_custom_test

# Source files
SRC = thread-pool.cc Semaphore.cc main.cc
SRC_TEST = thread-pool.cc Semaphore.cc tptest.cc
SRC_CUSTOM_TEST = thread-pool.cc Semaphore.cc tpcustomtest.cc

# Default target
all: $(TARGET)

# Link the target with object files
$(TARGET): $(SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Build and link test version
test: $(SRC_TEST)
	$(CXX) $(CXXFLAGS) -o $(TARGET_TEST) $^

# Build and link custom test version
custom_test: $(SRC_CUSTOM_TEST)
	$(CXX) $(CXXFLAGS) -o $(TARGET_CUSTOM_TEST) $^

# Clean up build artifacts
clean:
	rm -f $(TARGET) $(TARGET_TEST) $(TARGET_CUSTOM_TEST)

.PHONY: all clean test custom_test
