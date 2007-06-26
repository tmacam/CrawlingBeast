// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "isamutils.hpp"
#include "fnv1hash.hpp"

#include "mkprepr.hpp"

#include <algorithm>


/***********************************************************************
				    Typedefs
 ***********************************************************************/

//!@name Inter-page linking containers
//!@{
typedef hash_map < uint64_t, TURLFingerprintVec* > TFPAdjacencyMap;

typedef std::vector<uint32_t> TOutlinkList;
typedef std::vector< TOutlinkList*  > TSeqAdjacencyList;
//!@}


/***********************************************************************
			      PrePageRankVisitor
 ***********************************************************************/

/**Visitor that just loads the Pre-Pagerank data into memory.
 *
 * Pages with no outlinks are ignored.
 *
 */
struct PrePageRankVisitor {

	TFPAdjacencyMap& outlinks;


	PrePageRankVisitor(TFPAdjacencyMap& _map)
	: outlinks(_map)
	{}

	void operator()(uint32_t count, const prepr_hdr_entry_t* hdr,
			filebuf prepr_data)
	{
		prepr_data_entry_t* data_header = NULL;
		data_header = readFromFilebuf<prepr_data_entry_t>(prepr_data);

		assert(data_header->docid == hdr->docid);

		// Ignore entry if it has no outlinks
		if (data_header->n_outlinks == 0 ) {
			// Nothing to see here, move along
			return;
		}

		// Get self fingerprint
		uint64_t fp =  data_header->fp;

		// read list of outlinks
		filebuf fp_list_data = prepr_data.readf(data_header->len);
		uint64_t* begin = (uint64_t*) fp_list_data.current;
		uint64_t* end = (uint64_t*) fp_list_data.end;
		assert( uint32_t(end-begin) == data_header->n_outlinks );


		outlinks[fp] = new TURLFingerprintVec(begin, end);

	}

	bool pruneOutlinks()
	{
		bool any_changed = false;

		TFPAdjacencyMap pruned;
		TFPAdjacencyMap::iterator page;

		IsInMap<TFPAdjacencyMap> exists_in_outlinks(outlinks);

		for(page = outlinks.begin(); page != outlinks.end(); ++page) {
			const uint64_t& fp = page->first;
			TURLFingerprintVec* cur = page->second;

			TURLFingerprintVec valids;
			valids.reserve(cur->size());

			copy_if( cur->begin(), cur->end(),
				 std::inserter(valids, valids.begin()),
				 exists_in_outlinks);

			if ( ! any_changed) {
				any_changed = cur->size() != valids.size();
			}

			cur->swap(valids); // Current FPs are valid now

			if (! cur->empty() ) {
				pruned[fp] = cur;
			} else {
				delete cur;
//                                std::cout << "Pruned " << fp << std::endl;
			}
		}

		pruned.swap(outlinks); // Outlinks are pruned now

		return any_changed;
	}
};



/***********************************************************************
				 Aux. Functions
 ***********************************************************************/


void show_usage()
{
	std::cout <<
		"Usage:\t mkprepr prepr_dir output_dir\n"
		"\n"
		"\tprepr_dir\tWhere the Pre-PageRank data is\n"
		"\toutput_dir\tWhere the metadata ISAM will be written.\n"
		<< std::endl;
}


/***********************************************************************
				      MAIN
 ***********************************************************************/

void go(char* argv[])
{
	const char* prepr_dir = argv[1];
//        const char* output_dir = argv[2];

	TFPAdjacencyMap fp_outlinks;
	PrePageRankVisitor visitor(fp_outlinks);

	std::cout << "# Reading outlink data ..." << std::endl;
	VisitIndexedStore<prepr_hdr_entry_t>(prepr_dir, "prepr", visitor);

	std::cout << "Prunning\n";

	while( visitor.pruneOutlinks() ) {
		std::cout << "Another prunning iteration\n";
	}

	std::cout << "# documents read :" << fp_outlinks.size() << std::endl;
}


int main(int argc, char* argv[])
{
	/* Parse command line */
	if(argc < 3) {
		std::cerr << "Wrong number of argments" << std::endl;
		show_usage();
		exit(EXIT_FAILURE);
	}

	go(argv); // XXX Why, Oh!, Why do I have to code this workarounds for
		// these silly C++ issues/bugs/ghots...


	exit(EXIT_SUCCESS);
}


//EOF
