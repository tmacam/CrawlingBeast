// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
/**@file queryvec.cpp
 *
 * @brief Handles vectorial queries.
 *
 *
 * @note assuming that the inverted file is inverted.
 *
 */

#include <iostream>
#include <map>
#include <iterator>
#include <algorithm>

#include "mergerutils.hpp"
#include "indexerutils.hpp"
#include "indexcompression.hpp"
#include "strmisc.h"
#include "isamutils.hpp"
#include "vectorial.hpp"

// ASSUMING COMPRESSED INVERTED FILES


/***********************************************************************
			     Typedefs and constants
 ***********************************************************************/


//struct vec_res_t{
//        docid_t docid;
//        double weight;
//
//        vec_res_t(docid_t id, double w)
//        : docid(id), weight(w)
//        {}
//
//        inline bool operator<(const vec_res_t& other)
//        {
//                return (weight < other.weight); 
//        }
//};

//!A vectorial query result entry
typedef std::pair<docid_t,double> vec_res_t;

bool operator<(const vec_res_t& a, const vec_res_t& b)
{
	return a.second < b.second;
}

//!Vector of results of a vectorial query
typedef std::vector<vec_res_t> vec_res_vec_t;

struct NormLoadingVisitor {
	wdmfdt_map_t& WFMap;

	NormLoadingVisitor(wdmfdt_map_t& m)
	: WFMap(m)
	{}

	inline
	void operator()(uint32_t id, const norm_hdr_entry_t* entry, filebuf data)
	{
		wdmaxfdt_t* val = (wdmaxfdt_t* ) data.read(sizeof(wdmaxfdt_t));
		WFMap[entry->docid] = *val;
		std::cout << entry->docid << " " << val->wd << std::endl; // FIXME
	}
};


/***********************************************************************
			     Indexed
 ***********************************************************************/


/***********************************************************************
			     VectorialQueryResolver
 ***********************************************************************/


struct VectorialQueryResolver {
	typedef hdr_entry_t*	ifile_idx;
	typedef __gnu_cxx::hash_map < docid_t, double> accumulator_t;
	typedef std::vector<uint32_t> termvec_t;

	std::string store_dir;

	/**@name In memory pre-processed, cached and misc. information.
	 *
	 * This is the stuff we kepp in the memory for answering queries.
	 */
	//!@{
	StrIntMap voc;		//!< Vocabulary
	wdmfdt_map_t WFMap;	//!< Document norms / weights
	uint32_t N;		//!< Number of documents indexed.
	//!@}

	MMapedFile idx_file;
	ifile_idx idx;

	/**Constructor.
	 *
	 * @param dir Directory where inverted file and vocabulary data
	 * 	      files are.
	 */
	VectorialQueryResolver(const char* dir, bool conjunctive=true)
	: store_dir(dir),
	  idx_file( store_dir + "/index.hdr"),
	  idx((hdr_entry_t*)idx_file.getBuf().start)
	{
		load_vocabulary(voc,store_dir.c_str());

		// Load Document norms/weights
		NormLoadingVisitor norm_loader(WFMap);
		VisitIndexedStore<norm_hdr_entry_t>( store_dir.c_str(),
						     "norm",
						     norm_loader);
		N = WFMap.size();
	}

	//!@name Query and vocabulary conversion methods
	//@{

	/**Return the TermID associated with a given word.
	 *
	 * The word is normalized before the search is done.
	 */
	inline uint32_t word2termid(std::string term)
	{
		std::string word = normalize_term(term);

		StrIntMap::const_iterator wpos = voc.find(word);
		if (wpos == voc.end()){
			std::string err_msg("Term '");
			err_msg +=  term + "' (" +  word +
				") not in vocabulary";
			throw std::runtime_error(err_msg);
		}

		return wpos->second;
	}

	/**Functor to word2termid.
	 *
	 * All the work you have to do just to use C++
	 * as a functional language (map ptr_method list)
	 * is ridiculous!
	 */
	struct W2T_functor {
		VectorialQueryResolver& v;
		
		W2T_functor(VectorialQueryResolver& v)
		: v(v)
		{}

		uint32_t operator()(std::string w)
		{
			return v.word2termid(w);
		}
	};

	inline termvec_t query2termids(std::string query)
	{
		std::vector<std::string> terms( split(query," ") );
		termvec_t res(terms.size());

		std::transform( terms.begin(), terms.end(),
				res.begin(),
				W2T_functor(*this));

		return res;
	}
	//!@}

	void getTermIdInvertedList(uint32_t termid, inverted_list_vec_t& ilist)
	{
		hdr_entry_t& entry = idx[termid];

		ilist.clear();

		std::string data_filename = 
			BaseInvertedFileDumper::mk_data_filename(store_dir,
								entry.fileno);
		MMapedFile data_file(data_filename);
		filebuf data = data_file.getBuf();
		data.read(entry.pos);

		ByteWiseCompressor::decompress(entry.ft, data,ilist);
	}

	/**Processes a query.
	 *
	 * Observe that term normalization is handled internally.
	 * 
	 * @param query A string with the query terms separated
	 * 		by space.
	 *
	 * @param[out] result A set with all docids that match
	 * 		      the query. Will be overwritten
	 * 		      by this method.
	 *
	 */
	void processQuery(std::string query, vec_res_vec_t& result)
	{
		// Convert the query to term-ids
		termvec_t terms = query2termids(query);
		termvec_t::const_iterator t;
		if(terms.empty()) return;

		accumulator_t acc;
		accumulator_t::iterator acc_d;
		inverted_list_vec_t ilist;
		inverted_list_vec_t::const_iterator d;

		// Variables used for calculating per-document
		// and per-term weight
		double idf;		// term weight
		uint32_t ft;		// n documents w/ term

		result.clear();

		for(t = terms.begin(); t != terms.end(); ++t) {

			getTermIdInvertedList(*t, ilist);
			ft = ilist.size();
			idf = log(double(N)/double(ft));

			for(d=ilist.begin(); d != ilist.end(); ++d) {
				// Syntatic sugar - cause you known
				// you will need it later, don't ya?
				const d_fdt_t& d_fdt = *d;
				const uint32_t& doc = d_fdt.first;
				const uint32_t& fdt = d_fdt.second;

				acc[doc] = acc[doc] + (float(fdt)*idf);
			}// end for each document
		} //end for each term

		// Normalize all the matches
		for(acc_d = acc.begin(); acc_d != acc.end(); ++acc_d) {
			const docid_t& doc = acc_d->first;
			double& doc_weight = acc_d->second;

			const wdmaxfdt_t& wf = WFMap[doc];

			doc_weight /= (double(wf.maxfdt) * wf.wd);
		}


		// Copy result to output
		result.reserve(acc.size());
		result.assign(acc.begin(), acc.end());
		std::sort(result.begin(), result.end());
	}

};


/***********************************************************************
				      MAIN
 ***********************************************************************/

void do_query(VectorialQueryResolver& resolver, std::string query)
{
	vec_res_vec_t d_ids;
	resolver.processQuery(query,d_ids);


	if (d_ids.empty()) {
		std::cout << "No documents found matching query '" << query <<
			"' in documents." << std::endl;
	} else {
		std::cout << "Document(s) found matching query '" <<
				query << "': " << std::endl;
		vec_res_vec_t::const_iterator i;
		for(i = d_ids.begin(); i != d_ids.end(); ++i){
			std::cout << " doc: " << i->first << "  weight: " <<
						 i->second << std::endl;
		}
	}

}

void show_usage()
{
        std::cout << 	"Usage:\n"
			"queryvec store_dir\n"
			"\tstore_dir\twhere the index and norms data are."<< std::endl;
}

int main(int argc, char* argv[])
{
	// Parse command line
	if(argc < 2) {
		std::cerr << "Wrong number of arguments." << std::endl;
		show_usage();
		exit(EXIT_FAILURE);
	}

	std::string store_dir(argv[1]);
	std::string query;

	// Ok, let's start the show.

	std::cout << "Loading vocabulary and inverted file ... ";
	std::cout.flush();
	VectorialQueryResolver resolver(store_dir.c_str());
	std::cout << "done" << std::endl;

	std::cout <<"Type your query using spaces to split terms."<<std::endl;

	// Prompt
	std::cout << "> ";
	std::cout.flush();
	while(getline(std::cin,query)) {

		do_query(resolver, query);

		// Prompt
		std::cout << "> ";
		std::cout.flush();
	}


	exit(EXIT_SUCCESS);

}




//EOF
