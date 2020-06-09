CXXFLAGS= -fsanitize=address -std=c++11 -Weverything -Wall -Werror -g -Wall -Werror -O2 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CXX=clang++
AR=ar
ARFLAGS=rcu

.PHONY: clean

all: eson_test

main.o: main.cc eson.h
	$(CXX) $(CXXFLAGS) -c main.cc

eson_test: main.o eson.h
	$(CXX) $(CXXFLAGS) -o eson_test main.o

clean:
	rm -rf main.o eson_test
