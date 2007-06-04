// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "slidingreader.hpp"

#include <iostream>


int main(int argc, char* argv[])
{
	run_triple t;
	run_triple last_triple(0,0,0);

	MMapedSlidingReader w("/tmp/indexer_test_dir/run_0008",(1<<20));
	for( ; !w.eof(); ++w) {
		t = *w;
//                std::cout << t.termid << " " << t.docid << " " << t.freq <<
//                        std::endl;
		assert(last_triple < t);
		last_triple = t;
	}
}
