// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "slidingreader.hpp"

#include <iostream>


int main(int argc, char* argv[])
{
	run_triple t;
	FStreamSlidingReader w("_indexer_test_dir/run_0008",(10<<10)/9);
	for( ; !w.eof(); ++w) {
		t = *w;
		std::cout << t.termid << " " << t.docid << " " << t.freq <<
			std::endl;
	}
}
