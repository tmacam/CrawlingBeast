// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __MERGERUTILS_H__
#define __MERGERUTILS_H__
/**@file mergerutils.hpp
 * @brief Utilities helper classes for run mergeing and index generation .
 */


#include "slidingreader.hpp"

#include <queue>
#include <list>
#include <vector>


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

/*We could have used a uint16_t for freq. but this would not matter anyway as
 * this struct is not __attribute__((packed))*/
typedef std::pair<uint32_t, uint32_t> d_fdt_t;

typedef std::list<d_fdt_t> inverted_list_t;

typedef std::vector<d_fdt_t> inverted_list_vec_t;

struct hdr_entry_t {
	uint32_t ft;	//!< Count of documents containint term t
	uint32_t pos;	/**< Position of the inverted list in data file
			 *   with number @c fileno
			 */
	uint8_t fileno; /**< Number of the data file holding the inverted list
			 *   for this term
			 */

	hdr_entry_t(uint32_t count = 0, uint32_t position=0, uint8_t n=0)
	: ft(count), pos(position), fileno(n)
	{}
} __attribute__((packed));



/***********************************************************************
				   RunMerger
 ***********************************************************************/


/**Java-like iterator that merges the content of several runs.
 *
 * We really intended to make this a C++-like iterator (like most of the other
 * iterators in this project) but we just run out of patience. See 
 * http://doc.trolltech.com/qq/qq12-qt4-iterators.html for more info on
 * what we mean by java-like iterators.
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
	typedef FStreamSlidingReader SlidingReader;

	/**Constructor.
	 *
	 * @param _n_runs Number of runs to merge.
	 * @param run_store_dir Location of the rusns.
	 * @param max_memory Maximum memory to be distributed between all the
	 * 		     run readers.
	 */
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
				new SlidingReader( run_filename.c_str(),
								mem_per_run);

			if (! new_run_reader->eof()) {
				run_merger.push(new_run_reader);
			}
		};

	}

	/**Destructor.
	 *
	 * Make sure there's no run reader handing around allocated.
	 */
	~RunMerger()
	{
		while(!eof()){
			BaseSlidingReader* cur_run = run_merger.top();
			run_merger.pop();
			delete cur_run;
		}
	}

	inline bool eof() const {return run_merger.empty();}

	/**Get the next smallest triple availiable in all runs.
	 *
	 * This functio will  advance the reading position of the merger.
	 *
	 */
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


	/**Get the next availiable triple.
	 *
	 * This method is here just 'cuz i said this object implemented
	 * a java-like iterator interface. All it does is calling @c getNext.
	 *
	 * @see getNext
	 */
	inline run_triple next() { return getNext(); }

	/**Verifies if there is data left to be read in this iterator.
	 *
	 * This method is here just 'cuz i said this object implemented
	 * a java-like iterator interface. All it does is calling @c eof.
	 *
	 * @see eof
	 */inline bool hasNext() {return eof();}


};

/***********************************************************************
				BaseIndexDumper
 ***********************************************************************/

/**Simple and naïve inverted file writer.
 *
 * All other (?!) index dumpers descend from this one.
 * This one uses no compression at all.
 *
 * This class defines a really simple inverted file writer. As "input" it
 * uses a run merger, from which it will read triples that, in turn, will 
 * be grouped in <em><term, tf, <inverted list>></em> sequences and written
 * to disk.
 *
 * While this class does not implement any kind of index compression, you
 * can subclass it to construct more advanced inverted file writers that
 * do compress the generated inverted lists. See @c dumpInvertedList
 * for more information on how to specialize this class.
 *
 * The basic structure of inverted files created with this class is as follow.
 * There are two or more files for each inverted file:
 *
 * - index.hdr
 *
 *  	The inverted list header, holding the search structure of this inverted
 *  	file. It consists of an array of @c hdr_entry tuples, containing, for
 *  	each term, its <document count (tf), file position, data file number>.
 *  	
 *
 * - index.data_xxxx
 *
 *   	The inverted file data files. Each of such files contain a
 *   	concatanation of various inverted lists.
 *
 * 	On 32bits architeture, files and processes can address at most ~2GB of
 * 	data. There are tricks to get around this limitation but they are way
 * 	too much troublesome for this assignment. For this reason, an index
 * 	file may be splitted in more then one data file.
 *
 */
class BaseInvertedFileDumper {
protected:
	std::string output_dir;	/**< Output directory where inverted-file file's
				 *   will be written.
				 */
	RunMerger& merger;	//!< The run merger
	int n_index;		//!< Number of data files issued so far.
	size_t max_data_size;	//!< A suggestion of how big a data file should be
	std::vector<hdr_entry_t> header_data;	/**< Storage for header data.
						 *   We use this to delay
						 *   header dump and to it
						 *   at once.
						 */
	std::ofstream hdr_file;	//!< The inverted  list index writer
	std::ofstream data_file;//!< Data file writer
public:
	/**Constructor.
	 *
	 * @param output_path Output directory where inverted-file file's
	 * 		      will be written.
	 *
	 * @param m	      A reference to a RunMerger instance ready to be
	 * 		      used and already setup with all the runs need to
	 * 		      create this inverted file.
	 *
	 * @param max_data_size A @e suggestion of how big a data file should
	 * 			be.
	 *
	 * @param header_reserve The number of header entries to reserve for
	 * 			 storing the header dada contents before
	 * 			 flushing it to disk. Observe this will only
	 * 			 be used to "reserve" and optimize header
	 * 			 storage before dumping it do disk -- header
	 * 			 content is not written in a buffered fashion.
	 *
	 */
	BaseInvertedFileDumper(std::string output_path, RunMerger& m,
			size_t _max_data_size, size_t header_reserve=1<<22);


	virtual ~BaseInvertedFileDumper(){}

	//! Generate the inverted index.
	void dump();

	/**Rotate index data files if needed.
	 * 
	 * This only happens if the current data file's length is greater than
	 * max_data_size.
	 *
	 */
	void rotateDataFile();

	/**Store information about a term.
	 *
	 * This function stores, for a given term:
	 *  - its frequency in documents (tf), 
	 *  - the position of its correspoding inverted list ( data file #,
	 *    and position in the data file),
	 *  - its inverted list.
	 *
	 *
	 * This function also causes the roration of a data files.
	 *
	 * @param termid The id of the term
	 * @param doc_count The number of documents with this term (term
	 * 		    frequency in the colection, tf).
	 * @param ilist The inverted list for this term, i.e. a list of
	 * 		<em><d, f_td></em> intries for this term.
	 *
	 * @see rotateDataFile
	 */
	void storeTerm(uint32_t termid, uint32_t doc_count,
			const inverted_list_t& ilist);
	
	static std::string mk_data_filename(std::string path_prefix, int n);

	/**Dump a inverted list to disk.
	 *
	 * This method can be redefined in sub-classes to add compression
	 * or whatsoever. The BaseInvertedFileDumper implementation
	 * just writes the inverted lists as <code>d_fdt_t[]</code> dumps.
	 *
	 * @param tf Number of entries in the inverted list.
	 * @param list The inverted list.
	 */
	virtual void dumpInvertedList(uint32_t tf, const inverted_list_t& ilist);

};



#endif // __MERGERUTILS_H__

//EOF
