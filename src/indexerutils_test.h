// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __INDEXERUTILS_TEST_H
#define __INDEXERUTILS_TEST_H

#include "indexerutils.hpp"
#include "mergerutils.hpp"
#include "cxxtest/TestSuite.h"

#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>

/**
 * @todo better tests.
 * */
class IndexerUtilsTestSuit : public CxxTest::TestSuite {
	static const std::string INDEXER_SANDBOX_DIR;
public:

	/*
	 * Test Fixures setup
	 */
	void setUp()
	{
		mkdir(INDEXER_SANDBOX_DIR.c_str(),S_IRWXU);
	}

	void tearDown()
	{
		std::string cmd_line("rm -rf ");
		cmd_line += INDEXER_SANDBOX_DIR;

		system(cmd_line.c_str());
	}

	/*
	 * Tests
	 */

	void test_SmallInsertAndMerging()
	{
		const int KB = 1<<10;
		const int n_runs = 2;

		// Each run should store no more then 2 triples
		// And we should have no more then 2 runs in this test
		run_inserter run(INDEXER_SANDBOX_DIR, 2*sizeof(run_triple));

		*run++ = run_triple(1,0,0);
		*run++ = run_triple(2,0,0);
		*run++ = run_triple(3,0,0);
		*run++ = run_triple(4,0,0);

		run.flush();
		TS_ASSERT_EQUALS(run.getNRuns(), n_runs);

		RunMerger merger(n_runs, INDEXER_SANDBOX_DIR.c_str(),1*KB); 

		TS_ASSERT( ! merger.eof());
		TS_ASSERT_EQUALS( merger.getNext(), run_triple(1,0,0));
		TS_ASSERT_EQUALS( merger.getNext(), run_triple(2,0,0));
		TS_ASSERT_EQUALS( merger.getNext(), run_triple(3,0,0));
		TS_ASSERT_EQUALS( merger.getNext(), run_triple(4,0,0));
		TS_ASSERT( merger.eof());
	}

	void test_SmallInterleavedInsertAndMerging()
	{
		const int KB = 1<<10;
		const int n_runs = 2;

		// Each run should store no more then 2 triples
		// And we should have no more then 2 runs in this test
		run_inserter run(INDEXER_SANDBOX_DIR, 2*sizeof(run_triple));

		*run++ = run_triple(1,0,0);
		*run++ = run_triple(4,0,0);
		*run++ = run_triple(2,0,0);
		*run++ = run_triple(3,0,0);

		run.flush();
		TS_ASSERT_EQUALS(run.getNRuns(), n_runs);

		RunMerger merger(n_runs, INDEXER_SANDBOX_DIR.c_str(),1*KB); 

		TS_ASSERT( ! merger.eof());
		TS_ASSERT_EQUALS( merger.getNext(), run_triple(1,0,0));
		TS_ASSERT_EQUALS( merger.getNext(), run_triple(2,0,0));
		TS_ASSERT_EQUALS( merger.getNext(), run_triple(3,0,0));
		TS_ASSERT_EQUALS( merger.getNext(), run_triple(4,0,0));
		TS_ASSERT( merger.eof());
	}

	void testTripleInserterAndMerger()
	{
		const int KB = 1<<10;
		const int n_triples = 9 * KB;
		const size_t max_mem = 10 * KB;
		const int n_runs = int(0.5 + float(n_triples * sizeof(run_triple))/max_mem);

		run_inserter run(INDEXER_SANDBOX_DIR + "/", max_mem);

		// Insert the runs
		for(int i = n_triples; i > 0 ; --i){
			*run++ = run_triple(i,0,0);
		}
		run.flush();

		// Stupid test about run creation
		TS_ASSERT_EQUALS(run.getNRuns(), n_runs);

		// Test for merging correcteness
		RunMerger merger(n_runs, INDEXER_SANDBOX_DIR.c_str(),10*KB); 
		int processed_triples = 0;
		run_triple last_triple(1,0,0);
		run_triple cur_triple;
		// Merge!
		while( ! merger.eof()) {
			cur_triple = merger.getNext();

			TS_ASSERT_EQUALS(last_triple.termid, cur_triple.termid);
			//last_triple = cur_triple;
			++(last_triple.termid);

			++processed_triples;
		}
		TS_ASSERT(merger.eof());
		TS_ASSERT_EQUALS(processed_triples , n_triples);

	}
	

};



const std::string IndexerUtilsTestSuit::INDEXER_SANDBOX_DIR("_indexer_test_dir");


#endif // __INDEXERUTILS_TEST_H
