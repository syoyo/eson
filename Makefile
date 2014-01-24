CXXFLAGS=-g -O2 -D_LARGEFILE_SOURCE -D_FILE_OFFSET_BITS=64
CXX=g++
AR=ar
ARFLAGS=rcu

.PHONY: clean

all: eson_test

eson.o: eson.cc
	$(CXX) $(CXXFLAGS) -c eson.cc -o eson.o

main.o: main.cc
	$(CXX) $(CXXFLAGS) -c main.cc

libeson.a: eson.o
	$(AR) $(ARFLAGS) libeson.a eson.o

eson_test: libeson.a main.o
	$(CXX) $(CXXFLAGS) -o eson_test main.o -L./ -leson

clean:
	rm -rf main.o eson.o eson_test libeson.a
