CXXFLAGS = -Iinclude -Wall -std=c++20

EXEC = main

all: main

main: ../benchmarks/main.cpp
	mkdir -p ./out
	g++ $(CXXFLAGS) main.cpp ../include/xtree.cpp ../include/xtree.hpp -o ./out/benchmark.exe -O3

run:
	cd out && ./benchmark.exe

clean:
	rm -rf ./out

.PHONY: all clean