// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "slidingreader.hpp"
#include "mergerutils.hpp"

#include <iostream>
#include <queue>

template <class T>
T fromString(std::string s)
{
	T val;
	std::istringstream reader(s);
	reader >> val;

	return val;

}

void show_usage()
{
	std::cout << 	"Usage:\n"
			"merger n_runs run_dir" << std::endl;
}

int main(int argc, char* argv[])
{
	int max_mem = 1<<30; // 1GB

	/*
	 * parse comand line
	 */
	if (argc != 3) {
		show_usage();
		exit(EXIT_FAILURE);
	}
	int n_runs = fromString<int>(argv[1]);
	const char* output_dir = argv[2];

	//Merge!
	RunMerger merger(n_runs, output_dir, max_mem);
	while(!merger.eof()){
		run_triple val = merger.getNext();

		std::cout << val.termid << " " << 
			val.docid << " " << 
			val.freq << " " << 
			std::endl;

	}
}
