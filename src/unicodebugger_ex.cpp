#include<iostream>
#include "unicodebugger.h"
#include "mmapedfile.h"

void do_your_thing(std::string filename)
{
	MMapedFile file(filename);
	UnicodeBugger unicode(file.getBuf());
	AutoFilebuf data(unicode.convert());

	//std::cout << filename << " : " << unicode.getEncoding() << std::endl;
}

int main(int argc, char* argv[])
{
	
	for(int loop = 0; loop < 1000; ++loop) {
		for(int i = 1; i < argc; ++i){
			do_your_thing(argv[i]);
		}
	}

	std::cout << "Done.";
	std::string press_enter;
	std::cin >> press_enter;

}

// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

