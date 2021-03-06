// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "slidingreader.hpp"
#include "mergerutils.hpp"
#include "strmisc.h"

#include <iostream>
#include <queue>

void show_usage()
{
	std::cout << 	"Usage:\n"
			"merger n_runs run_dir" << std::endl;
}

int main(int argc, char* argv[])
{
	const int MB =  1<<20;

	int max_mem = 1<<29; // 0.5GB

	/*
	 * parse comand line
	 */
	if (argc != 3) {
		show_usage();
		exit(EXIT_FAILURE);
	}
	int n_runs = fromString<int>(argv[1]);
	const char* output_dir = argv[2];

	// Create index
	RunMerger merger(n_runs, output_dir, max_mem);
	ByteWiseCompressedInvertedFileDumper ifile(output_dir, merger, 256*MB);
	std::cout << "Setup complete. Staring the merging process." <<std::endl;
	ifile.dump();

}
