

CURL_CFLAGS = `priv_curl/bin/curl-config --cflags` 
CURL_LDFLAGS = `priv_curl/bin/curl-config --libs` 
#CURL_CFLAGS = `curl-config --cflags`
#CURL_LDFLAGS = `curl-config --libs`

TARGETS	 = runner crawlingbeast indexer merger querybool queryvec mkstore\
	   mknorms mkprepr mkpagerank myserver mkmeta
CC	 = g++
#CXXFLAGS = -I. -ggdb -O3 -march=i686 -Wall -pthread  $(CURL_CFLAGS)
CXXFLAGS = -I. -ggdb -O0 -Wall -pthread  $(CURL_CFLAGS) -D_GLIBCXX_DEBUG
LDFLAGS	 = -L. -lgzstream -lz -pthread $(CURL_LDFLAGS)
AR	 = ar cr
OBJFILES = filebuf.o parser.o htmlparser.o urltools.o strmisc.o mmapedfile.o unicodebugger.o urlretriever.o pagedownloader.o threadingutils.o domains.o deepthought.o paranoidandroid.o libgzstream.a sauron.o libcurl.a robotshandler.o entityparser.o htmliterators.o indexerutils.o mergerutils.o zfilebuf.o httpserver.o



all: $(TARGETS) tags test

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

indexer: indexer.o $(OBJFILES)

merger.o: merger.cpp mergerutils.?pp indexerutils.?pp

merger: merger.o mergerutils.o 
	g++  -lz -pthread   merger.o mergerutils.o -o merger

mkstore: mkstore.o mmapedfile.o filebuf.o 
	g++  -lz mkstore.o mmapedfile.o filebuf.o -o mkstore

mknorms: mknorms.o mmapedfile.o filebuf.o
	g++  -lm mknorms.o mmapedfile.o filebuf.o -o mknorms

myserver: myserver.o $(OBJFILES)

test_indexer: test_indexer.o $(OBJFILES)

dump_index: dump_index.o $(OBJFILES)

slidingreader: slidingreader.o mmapedfile.o

querybool: querybool.cpp $(OBJFILES)

queryvec: queryvec.o $(OBJFILES)

dumb_indexer2: dumb_indexer2.o $(OBJFILES)

mkprepr: mkprepr.o $(OBJFILES)

mkmeta: mkmeta.o $(OBJFILES)

mkpagerank: mkpagerank.o libgzstream.a mmapedfile.o


__testa_parser: __testa_parser.o $(OBJFILES)

threadingutils_teste: threadingutils_teste.cpp threadingutils.h $(OBJFILES)

tags: *.cpp *.h
	ctags *.cpp *.h *.hpp


.PHONY: clean test

clean:
	rm -fv *.o libgzstream.a runner.cpp $(TARGETS)

test: runner
	./runner
