

TARGETS = foo runner
CC=g++
CXXFLAGS = -ggdb -O0 -Wall -pthread


all: $(TARGETS) test

foo: test.o filebuf.o parser.o htmlparser.o

test.o: test.cpp parser.h

runner.cpp: *test.h
	cxxtest/cxxtestgen.pl --error-printer -o runner.cpp *test.h

runner: runner.cpp filebuf.o parser.o htmlparser.o urltools.o
	g++ -I . -o runner $^

.PHONY: clean test

clean:
	rm -fv *.o $(TARGETS)

test: runner
	./runner
