

TARGETS = test
CC=g++


all: $(TARGETS)

test: test.o filebuf.o parser.o

test.o: test.cpp parser.h

runner.cpp: *test.h
	cxxtest/cxxtestgen.pl --error-printer -o runner.cpp *test.h

runner: runner.cpp filebuf.o parser.o
	g++ -I . -o runner $^


.PHONY: clean

clean:
	rm -fv *.o $(TARGETS)
