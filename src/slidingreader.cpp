// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "slidingreader.hpp"

#include <iostream>


int main(int argc, char* argv[])
{
	run_triple t;
	FStreamSlidingReader w("/tmp/runtest/run_0007",256);
	for( ; !w.eof(); ++w) {
		t = *w;
		std::cout << t.termid << " " << t.docid << " " << t.freq <<
			std::endl;
	}
}
