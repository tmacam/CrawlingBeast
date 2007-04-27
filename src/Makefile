

TARGETS = foo runner
CC=g++
CXXFLAGS = -ggdb -O0 -Wall -pthread


all: $(TARGETS) test tags

foo: test.o filebuf.o parser.o htmlparser.o

test.o: test.cpp parser.h

runner.cpp: *test.h *.h
	cxxtest/cxxtestgen.pl --error-printer -o runner.cpp *test.h

runner: runner.cpp filebuf.o parser.o htmlparser.o urltools.o explode.o
	g++ $(CXXFLAGS) -I . -o runner $^

tags: *.cpp *.h
	ctags *.cpp *.h

.PHONY: clean test

clean:
	rm -fv *.o $(TARGETS)

test: runner
	./runner
