CXXFLAGS= -fsanitize=address -Weverything -Wall -Werror -g -Wall -Werror -O2 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CXX=clang++
AR=ar
ARFLAGS=rcu

.PHONY: clean

all: eson_test

main.o: main.cc
	$(CXX) $(CXXFLAGS) -c main.cc

eson_test: main.o
	$(CXX) $(CXXFLAGS) -o eson_test main.o

clean:
	rm -rf main.o eson_test
