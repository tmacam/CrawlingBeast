// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "config.h"
#include "isamutils.hpp"

#include "mkprepr.hpp"
#include "mkpagerank.hpp"

#include <algorithm>
#include <limits>
#include <cmath>

/***********************************************************************
				    GLOBALS

			Sorry, father, for I use globals!
 ***********************************************************************/

static TFP2Id translator;



/***********************************************************************
			      PrePageRankVisitor
 ***********************************************************************/

/**Visitor that calcs one iteration of Pagerank from PrePR data.
 *
 * Pages with no outlinks are ignored.
 *
 * The code in this class is used to estimate the PageRank for pages
 * that were previously processed by mkprepr.
 *
 * The algorithm here is base on code from "Efficient Computation of
 * PageRank", from Taher H. Haveliwla. More precisely, in the code
 * from page 5.
 *
 * Initial PageRank value is based on tips from 
 * http://www.iprcom.com/papers/pagerank/
 *
 *
 * FIXME remember iterate over PR, accumulate in ACC
 * FIXME We remove dangling_links from the graph and we never
 * 	 see them afterwards.
 *
 *
 */
class PrePageRankVisitor {

	PrePageRankVisitor();
	PrePageRankVisitor& operator=(const PrePageRankVisitor&);

public:
	TFPPageRankAccumulator& pr;
	TFPPageRankAccumulator& acc;
	TURLFingerprintVec& dangling_links;

	//Constructor
	PrePageRankVisitor(const TURLFingerprintVec& valid_fps,
			TFPPageRankAccumulator& _pageranks,
			TFPPageRankAccumulator& _accumulators,
			TURLFingerprintVec danglings)
	: pr(_pageranks), acc(_accumulators), dangling_links(danglings)
	{
		float seed_value = PAGERANK_SEED_VALUE;

		// Assign initial PR values!
		TURLFingerprintVec::const_iterator f;
		for(f = valid_fps.begin(); f != valid_fps.end(); ++f) {
			pr[*f] = seed_value;
		}
	}

	//! Copy constructor
	PrePageRankVisitor(const PrePageRankVisitor& other)
	: pr(other.pr), acc(other.acc),
	  dangling_links(other.dangling_links)
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

		// Account this page's outlinks
		accountPagesOutlink(fp, data_header->n_outlinks, begin, end);

	}

	/**Account the "votes" of a given page.
	 *
	 * Outlinks in page with fingerprint @p fp
	 * are inside the range [begin,end)
	 *
	 * Pages with no outlinks wont'be accounted and will
	 * be removed from the accumulator mapping if found.
	 * This is our "remove dangling" links approach.
	 *
	 * @param fp Fingerprint of the given page.
	 * @param n number of outlinks
	 * @param begin Outlinks begin iterator.
	 * @param end Outlinks one-past-the-end "iterator".
	 */
	inline void accountPagesOutlink(uint64_t fp, uint32_t n,
				const uint64_t* begin, const uint64_t* end)
	{
		float vote_value = pr[fp]/n;

		//std::cout << translator[fp] << " == " << pr[fp] << " : " << 
		//	pr[fp] << "/" << n << " = " << vote_value << std::endl;

		bool is_dangling = true;

		if (begin != end) {
			for(const uint64_t* link = begin; link != end; ++link){
				// Ignore accounting PR to dangling links
				if (pr.find(*link) == pr.end()) {
					continue;
				} 

				//std::cout << "\t -> " << translator[*link] << " : " <<
				//	  acc[*link] << " + " << vote_value;
				
				acc[*link] = acc[*link] + vote_value;

				is_dangling = false;

				//std::cout << "= " << acc[*link] << std::endl;
			}
		}
		
		if (is_dangling) {
			// Page with no (valid) outlink => dangling link
			// Remove it.
			acc.erase(fp);
			pr.erase(fp);
			std::cout << "Erased " << translator[fp] << std::cout;

			dangling_links.push_back(fp);
		}
	}

	//!Do the last part of the PageRank estimation loop.
	void finishCalcs()
	{
		TFPPageRankAccumulator::iterator i;
		// Iterate over PR, accumulate in ACC
		for(i = pr.begin(); i != pr.end(); ++i){
			const uint64_t& fp = i->first;

			acc[fp] = 0.85*acc[fp] + 0.15;
		}
	}

	/**Update current PageRank estimates and clean accumulators for next
	 * iteration.
	 */
	void resetAccumulators()
	{
		pr.swap(acc);
		acc.clear();
	}



	float getL1Norm()
	{
		float accumulator = 0;
		TFPPageRankAccumulator::const_iterator i;
		TFPPageRankAccumulator::const_iterator j;

		for(i = pr.begin(); i != pr.end(); ++i) {
			j = acc.find(i->first);
			if ( j !=  acc.end() ) {
				accumulator += fabs( i->second - j->second);
			} else {
				accumulator += fabs( i->second );
			}
		}

		return accumulator;
	}

	void addVotesToDanglings()
	{
		TURLFingerprintVec::const_iterator i;
		for(i = dangling_links.begin();
		    i != dangling_links.end();
		    ++i)
		{
			pr[*i] = 0.15;
		}
	}
};



/***********************************************************************
				 Aux. Functions
 ***********************************************************************/

void outputPageRank(const PrePageRankVisitor& visitor,
		    std::string pr_filename)
{
	// Setup dumper
	std::ofstream file(pr_filename.c_str(),
				std::ios::binary | std::ios::out);
	file.exceptions( std::ios_base::badbit|std::ios_base::failbit);

	// Copy pagerank data to vector - helps during write
	std::vector<pagerank_hdr_entry_t> pagerank_data;
	pagerank_data.reserve(visitor.pr.size());

	TFPPageRankAccumulator::const_iterator i;
	for(i = visitor.pr.begin(); i != visitor.pr.end(); ++i){
		pagerank_data.push_back(
			pagerank_hdr_entry_t(translator[i->first], i->second));
	}

	// write file - if non-empty
	size_t len = pagerank_data.size() * sizeof(pagerank_hdr_entry_t);
	if (len > 0) {
		file.write((char*)&pagerank_data[0], len);
	}
}


void show_usage()
{
	std::cout <<
		"Usage:\t mkprepr docid_list prepr_dir output_dir\n"
		"\n"
		"\tdocid_list\tlist of valid docids\n"
		"\tprepr_dir\tWhere the Pre-PageRank data is\n"
		"\toutput_dir\tWhere the metadata ISAM will be written.\n"
		<< std::endl;
}


/***********************************************************************
				      MAIN
 ***********************************************************************/

void go(char* argv[])
{
	int i_count = 0;
	float residual = std::numeric_limits<float>::max();

	const char* docid_list = argv[1];
	const char* prepr_dir = argv[2];
	const char* output_dir = argv[3];
	
	// Get the list of valid URLs
	TIdUrlMap id2url;
	TIdUrlMap::const_iterator iu;
	TURLFingerprintVec valid_fps;
	read_ids_and_urls(docid_list,id2url);
	valid_fps.reserve(id2url.size());
	for(iu = id2url.begin(); iu != id2url.end(); ++iu) {
		valid_fps.push_back(FNV::hash64(iu->second));
		translator[FNV::hash64(iu->second)] = iu->first; //FIXME
	}
	id2url.clear();

	TFPPageRankAccumulator pr;
	TFPPageRankAccumulator acc;
	TURLFingerprintVec danglings;
	PrePageRankVisitor visitor(valid_fps, pr, acc, danglings);

	/*
	 *  Iterate calculating PageRank
	 */
	std::cout << "# Starting iteration..." << std::endl;
	while ( residual > PAGERANK_RESIDUAL_LIMIT ) {
		VisitIndexedStore<prepr_hdr_entry_t>(prepr_dir, "prepr", visitor);
		visitor.finishCalcs();
		residual = visitor.getL1Norm();
		visitor.resetAccumulators();
		std::cout << "# iteration number " << i_count << " residual " << residual << std::endl;

		++i_count;
	}
	visitor.addVotesToDanglings();

	// Output PR data
	std::string pr_filename = output_dir + PAGERANK_HDR_SUFIX;
	outputPageRank(visitor, pr_filename);
}


int main(int argc, char* argv[])
{
	/* Parse command line */
	if(argc < 4) {
		std::cerr << "Wrong number of argments" << std::endl;
		show_usage();
		exit(EXIT_FAILURE);
	}

	go(argv); // XXX Why, Oh!, Why do I have to code this workarounds for
		// these silly C++ issues/bugs/ghots...


	exit(EXIT_SUCCESS);
}


//EOF
