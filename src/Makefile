

TARGETS	 = runner crawlingbeast
CC	 = g++
CXXFLAGS = -I. -ggdb -O0 -Wall -pthread `curl-config --cflags`
LDFLAGS	 = `curl-config --libs` -L. -lgzstream -lz -pthread
AR	 = ar cr
OBJFILES = filebuf.o parser.o htmlparser.o urltools.o explode.o mmapedfile.o unicodebugger.o urlretriever.o pagedownloader.o threadingutils.o domains.o deepthought.o paranoidandroid.o libgzstream.a sauron.o


all: $(TARGETS) tags test crawlingbeast

crawlingbeast: $(OBJFILES) crawlingbeast.o

gzstream.o:gzstream.C gzstream.h
	$(CXX) $(CXXFLAGS) -c -o gzstream.o gzstream.C

libgzstream.a : gzstream.o
	${AR} libgzstream.a gzstream.o


test.o: test.cpp parser.h

runner.cpp: *test.h *.h
	cxxtest/cxxtestgen.pl --error-printer -o runner.cpp *test.h

runner: runner.cpp $(OBJFILES)

unicodebugger_ex: $(OBJFILES) unicodebugger_ex.o

getter: getter.o $(OBJFILES)

threadingutils_teste: threadingutils_teste.cpp threadingutils.h $(OBJFILES)

tags: *.cpp *.h
	ctags *.cpp *.h


.PHONY: clean test

clean:
	rm -fv *.o libgzstream.a runner.cpp $(TARGETS)

test: runner
	./runner
