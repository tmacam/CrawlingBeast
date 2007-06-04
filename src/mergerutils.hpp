// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __MERGERUTILS_H__
#define __MERGERUTILS_H__
/**@file mergerutils.hpp
 * @brief Utilities helper classes for run mergeing and index generation .
 */


#include "slidingreader.hpp"

#include <sstream>
#include <queue>


/***********************************************************************
			     Typedefs and constants
 ***********************************************************************/

struct GetSmallestQueue {
	inline bool operator()( const BaseSlidingReader* a,
				const BaseSlidingReader* b)
	{
		return !(*a < *b);
	}
};

typedef std::vector<BaseSlidingReader*> ReadersPtrVec;

typedef std::priority_queue< BaseSlidingReader*,
			     ReadersPtrVec,
			     GetSmallestQueue >		RunPriorityQueue;

/***********************************************************************
				     Merger
 ***********************************************************************/


/** Java-like iterator that merges the content of several runs.
 *
 * We really intended to make this a C++-like iterator (like most of the other
 * iterators in this project) but we just run out of pacience.
 *
 * <b>How this class works</b>
 *
 * All run readers are modeled after the iterator concept. So, calling theirs
 * operator*() return the current smalles value inside each of them. Calling
 * theirs operator++ make them advance their reading position.
 *
 * So, instead of putting the values of each run in a priority_queue and
 * recording from which run each value came, we add the run readers
 * (BaseSlidingReader) themselves to the queue. The "head" of the queue will
 * then be the run with the smallest value up that moment. This does need a
 * more elaborate comparison operator for the priority_queue but nothing really
 * difficult - it is quite obvious, really.
 *
 * The whole merging process then can be seen as a sequence of the following
 * steps:
 *
 *  - Poping the head of the queue (run with the smalles triple in this
 *  given moment),
 *
 *  - Saving this triple value and advacing the run's reading position,
 *
 *  - Adding the run reader back to the priority_queue if it non-empty.
 *
 */
class RunMerger {

	int n_runs;		//!< Number of runs that will be merged
	size_t mem_per_run;	//!< Max memory allocated per run reader
	RunPriorityQueue run_merger; /**< Priority Queue that holds all
				      *	  our run readers
				      */
	run_triple cur_triple;	//!< The last triple read;
	
	//! Not Default Constructible.
	RunMerger();

	//! Prevent Copying and assignment.
        RunMerger(const RunMerger& );

        //! Prevent Copying and assignment.
        RunMerger& operator=(const RunMerger&);
public:
	RunMerger(int _n_runs, const char* run_store_dir, size_t max_memory)
	: n_runs(_n_runs),
	  mem_per_run(max_memory / n_runs),
	  cur_triple(0,0,0)
	{
		// Register runs with RunMerger
		for (int i = 0; i < n_runs; ++i) {
			std::string run_filename = 
			      run_inserter::make_run_filename(run_store_dir, i);
			BaseSlidingReader* new_run_reader =
				new FStreamSlidingReader( run_filename.c_str(),
								mem_per_run);

			if (! new_run_reader->eof()) {
				run_merger.push(new_run_reader);
			}
		};

	}

	~RunMerger()
	{
		while(!eof()){
			BaseSlidingReader* cur_run = run_merger.top();
			run_merger.pop();
			delete cur_run;
		}
	}

	inline bool eof() const {return run_merger.empty();}

	inline run_triple getNext()
	{
		// Merge Runs
		if (!eof()) {
			/* We don't use references here (despite the
			 * readability improvements they may cause) because
			 * using references would cause problems accessing
			 * the virtual table of BaseSlidingReader descendents.
			 *
			 * After all, sliding readers depend heavly on
			 * virtual methods for their correct operation.
			 */
			BaseSlidingReader* cur_run = run_merger.top();
			run_merger.pop();

			// Get the contents of the current run
			cur_triple = *(*cur_run);

			// Advance current run's reading position and 
			// add it back to the queue if it not empy
			++(*cur_run);
			if ( ! cur_run->eof() ){
				run_merger.push(cur_run);
			} else {
				delete cur_run;
			}

		}
		return cur_triple;
	}


};


#endif // __MERGERUTILS_H__

//EOF
