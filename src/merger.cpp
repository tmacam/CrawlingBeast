// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "slidingreader.hpp"

#include <iostream>
#include <queue>


struct GetSmallestQueue {
	inline bool operator()( const BaseSlidingReader* a,
				const BaseSlidingReader* b)
	{
		return !(*a < *b);
	}
};

typedef std::priority_queue< BaseSlidingReader*,
			     std::vector<BaseSlidingReader*>,
			     GetSmallestQueue >		RunPriorityQueue;

void show_usage()
{
	std::cout << 	"Usage:\n"
			"merger output_dir RUNs" << std::endl;
}

int main(int argc, char* argv[])
{
	int max_mem = 1<<30; // 1GB
	char* output_dir = NULL;
	int n_runs = 0;
	size_t mem_per_run = 0;

	/*
	 * parse comand line
	 */
	if (argc < 3) {
		show_usage();
		exit(EXIT_FAILURE);
	}
	output_dir = argv[1];
	n_runs = argc - 2;
	// Extra run account for merger buffered output
	mem_per_run = max_mem / ( n_runs + 1 );
	// Setup Run Merger
	std::vector<BaseSlidingReader*> _runs;
	_runs.reserve(n_runs);
	GetSmallestQueue _comp_functor;
	RunPriorityQueue run_merger(_comp_functor, _runs );

	// Register runs with merger
	for (int i = 2; i < argc; ++i) {
		run_merger.push(new FStreamSlidingReader(argv[i],mem_per_run));
	};

	// Merge Runs
	BaseSlidingReader* cur_run = NULL;
	run_triple last_triple(0,0,0);
	while(!run_merger.empty()) {
		cur_run = run_merger.top();
		run_merger.pop();

		const run_triple& cur_triple = *(*cur_run);
//
//                std::cout << (*cur_run)->termid << " " << 
//                        (*cur_run)->docid << " " << 
//                        (*cur_run)->freq << " " << 
//                        std::endl;


		// FIXME
		assert( last_triple < cur_triple );
		last_triple = cur_triple;

		// Advance current run's reading position and 
		// add it back to the queue if it not empy
		++(*cur_run);
		if ( ! cur_run->eof() ){
			run_merger.push(cur_run);
		} else {
			std::cout << "Removing a run" << std::endl;
			delete cur_run;
		}

	}
}
