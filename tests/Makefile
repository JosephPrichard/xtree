CXXFLAGS = -Iinclude -Wall -std=c++20 -Wno-deprecated

EXEC = main

all: main

main: ../tests/main.cpp
	mkdir -p ./out
	g++ $(CXXFLAGS) main.cpp ../include/xtree.cpp ../include/xtree.hpp -o ./out/test.exe -O1

run:
	cd out && ./test.exe

clean:
	rm -rf ./out

.PHONY: all clean