

TARGETS = foo runner
CC=g++
CXXFLAGS = -ggdb -O0 -Wall -pthread `curl-config --cflags`
LDFLAGS = `curl-config --libs`
OBJFILES = filebuf.o parser.o htmlparser.o urltools.o explode.o mmapedfile.o unicodebugger.o


all: $(TARGETS) tags test

foo: test.o filebuf.o parser.o htmlparser.o

test.o: test.cpp parser.h

runner.cpp: *test.h *.h
	cxxtest/cxxtestgen.pl --error-printer -o runner.cpp *test.h

runner: runner.cpp $(OBJFILES)
	g++ $(CXXFLAGS) -I . -o runner $^

unicodebugger_ex: $(OBJFILES) unicodebugger_ex.o
getter: getter.o $(OBJFILES)

tags: *.cpp *.h
	ctags *.cpp *.h

.PHONY: clean test

clean:
	rm -fv *.o runner.cpp $(TARGETS)

test: runner
	./runner
