

TARGETS = foo runner
CC=g++
CXXFLAGS = -ggdb


all: $(TARGETS)

foo: test.o filebuf.o parser.o htmlparser.o

test.o: test.cpp parser.h

#%.o: %.h

runner.cpp: *test.h
	cxxtest/cxxtestgen.pl --error-printer -o runner.cpp *test.h

runner: runner.cpp filebuf.o parser.o htmlparser.o
	g++ -I . -o runner $^

.PHONY: clean test

clean:
	rm -fv *.o $(TARGETS)

test: runner
	./runner
