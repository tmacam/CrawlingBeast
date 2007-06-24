// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __QUERYVEC_LOGIC_H__
#define __QUERYVEC_LOGIC_H__
/**@file queryvec_logic.hpp
 *
 * @brief Handles vectorial queries.
 *
 *
 * @note assuming that the inverted file is inverted.
 *
 */

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


//!A vectorial query result entry
typedef std::pair<docid_t,double> vec_res_t;

//! In descending order.
inline bool VecResComparator(const vec_res_t& a, const vec_res_t& b)
{
	return (a.second > b.second);
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
	}
};


/***********************************************************************
			     VectorialQueryResolver
 ***********************************************************************/


struct VectorialQueryResolver {
	typedef hdr_entry_t*	ifile_idx;
	typedef __gnu_cxx::hash_map < docid_t, double> accumulator_t;
	typedef std::vector<uint32_t> termvec_t;
	typedef std::vector<uint32_t> docidvec_t;

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

	//!@name Query and vocabulary conversion methods and utils
	//@{
	
	//!Just to signal that a word is not in vocabulary
	struct NotInVocabulary : public std::runtime_error {
		static std::string mkErrMsg(std::string term)
		{
			std::string err_msg("Term '");
			err_msg +=  term + "' not in vocabulary";

			return err_msg;
		}

		NotInVocabulary(std::string term)
		: std::runtime_error(mkErrMsg(term))
		{}
	};

	/**Return the TermID associated with a given word.
	 *
	 * The word is normalized before the search is done.
	 */
	inline uint32_t word2termid(std::string term)
	{
		std::string word = normalize_term(term);

		StrIntMap::const_iterator wpos = voc.find(word);
		if (wpos == voc.end()){
			throw NotInVocabulary(term);
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

		result.clear();

		// Convert the query to term-ids
		termvec_t terms;
		termvec_t::const_iterator t;
		try {
			terms = query2termids(query);
		} catch (NotInVocabulary& e) {
			std::cerr << e.what();
			terms.clear();
		};

		// We may end with an empty term list either due to
		// a empty query or to words that were not found.
		if(terms.empty()) return;

		accumulator_t acc;
		accumulator_t::iterator acc_d;
		inverted_list_vec_t ilist;
		inverted_list_vec_t::const_iterator d;

		// For "AND" semmantics
		bool first_list = true;

		// Variables used for calculating per-document
		// and per-term weight
		double idf;		// term weight
		uint32_t ft;		// n documents w/ term

		for(t = terms.begin(); t != terms.end(); ++t) {

			getTermIdInvertedList(*t, ilist);
			ft = ilist.size();
			idf = log(double(N)/double(ft));

			// Apply "AND" semmantics
			if (first_list) {
				first_list = false;
			} else {
				pruneToIntersection(acc,ilist);
			}

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
		std::stable_sort(result.begin(), result.end(), VecResComparator); // FIXME
	}

	/**Prune ilist and accummulators to the intersection of current and
	 * new docids.
	 *
	 * It will obtain the intersectino between previously known docids
	 * and the new ones in @p ilist. After that it will prune the list
	 * in @p ilist to match this intersection.
	 *
	 * We assume both lists are sorted and have no duplicates.
	 *
	 * @param[in,out] acc The current accumulators.
	 * @param[in,out] ilist The list of new docids. It will be pruned to
	 * 			match the intersection between its contents and
	 * 			@p acc contents.
	 *
	 * @todo Do it properly! This code is an ugly hack!
	 */
	void pruneToIntersection(accumulator_t& acc,
			inverted_list_vec_t& ilist)
	{
		// Current set of document ids
		docidvec_t cur;
		cur.reserve(acc.size());
		accumulator_t::iterator a;
		for (a = acc.begin(); a != acc.end(); ++a){
			cur.push_back(a->first);
		}
		std::sort(cur.begin(), cur.end());

		// list of new document ids
		docidvec_t new_ids;
		ilist2docvet(ilist, new_ids);

		// The resulting intersection
		std::set<uint32_t> inter;

		std::set_intersection(cur.begin(), cur.end(),
                                        new_ids.begin(), new_ids.end(),
                                        std::inserter(inter,inter.begin()));

		// Prune the inverted list - the lazy way FIXME
		inverted_list_vec_t result;
		result.reserve(new_ids.size());
		inverted_list_vec_t::iterator i;
		for(i = ilist.begin(); i != ilist.end(); ++i) {
			// If it is in inter, add to the results..
			if ( inter.find(i->first) != inter.end() ) {
				result.push_back(*i);
			}
		}

		// Prune the accumulators
		for(a = acc.begin(); a!= acc.end(); ++a) {
			// Remove any acc if it's document is not in inter...
			if( inter.find(a->first) == inter.end()) {
				acc.erase(a);
			}
		}

		ilist = result;
	}

	inline void ilist2docvet(const inverted_list_vec_t& ilist, docidvec_t& ids)
	{
		inverted_list_vec_t::const_iterator d;

		ids.reserve(ilist.size());
		for(d = ilist.begin(); d != ilist.end(); ++d ){
			ids.push_back(d->first);
		}

	}

};


#endif // __QUERYVEC_LOGIC_H__


//EOF
