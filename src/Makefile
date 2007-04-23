

TARGETS = test
CC=g++


all: $(TARGETS)

test: test.o filebuf.o parser.o

test.o: test.cpp parser.h


.PHONY: clean

clean:
	rm -fv *.o $(TARGETS)
