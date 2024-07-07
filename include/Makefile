CXXFLAGS = -Iinclude -Wall -std=c++20 -Wno-deprecated

EXEC = main

all: lib

lib: ../include/xtree.cpp ../include/xtree.hpp
	mkdir -p ./out
	g++ $(CXXFLAGS) -Ofast -shared -o ./out/xtree.so -fPIC ../include/xtree.cpp ../include/xtree.hpp

clean:
	rm -rf ./out

.PHONY: all clean