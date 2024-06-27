# Variables
CXXFLAGS = -Iinclude -Wall -std=c++20

# Executable name
EXEC = main

# Default target
all: main

# Compile source files into the executable directly
main: ../tests/main.cpp
	g++ $(CXXFLAGS) main.cpp ../include/xml.cpp ../include/xml.hpp -o ./out/test.exe -O3

# Clean up build files
clean:
	rm -f ./out/test.exe

# Phony targets
.PHONY: all clean