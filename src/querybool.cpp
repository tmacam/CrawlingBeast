// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
/**@file querybool.cpp
 *
 * @brief Handles boolean queries.
 *
 *
 * @note assuming that the inverted file is inverted.
 *
 * @todo move index reading routines into static methos of index dumping
 * classes.
 */

#include <iostream>
#include <map>
#include <iterator>
#include <algorithm>

#include "mergerutils.hpp"
#include "indexerutils.hpp"
#include "indexcompression.hpp"
#include "strmisc.h"

// ASSUMING COMPRESSED INVERTED FILES


/***********************************************************************
			     Typedefs and constants
 ***********************************************************************/

typedef hdr_entry_t*	ifile_idx;

/**Set of docids.
 *
 * @deprecated See QueryResolver::getTermDocset()
 */
typedef std::set<uint32_t> docset;

//!Vector of docids
typedef std::vector<uint32_t> docvet;

/***********************************************************************
				      BLAH
 ***********************************************************************/


inline std::string& normalize_term(std::string& word)
{
	WideCharConverter wconv;

	std::wstring w_word = wconv.mbs_to_wcs(word);
	normalize_term(w_word);
	word = wconv.wcs_to_mbs(w_word);

	return word;
}

struct QueryResolver {
	std::string store_dir;
	StrIntMap voc;
	MMapedFile idx_file;
	ifile_idx idx;
	bool conjunctive;

	/**Constructor.
	 *
	 * @param dir Directory where inverted file and vocabulary data
	 * 	      files are.
	 * @param conjunctive Should we default to @e AND between terms?
	 * 		      If @c false we default to disjunctive query
	 * 		      processing, i.e, using @e OR.
	 */
	QueryResolver(const char* dir, bool conjunctive=true)
	: store_dir(dir),
	  idx_file( store_dir + "/index.hdr"),
	  idx((hdr_entry_t*)idx_file.getBuf().start),
	  conjunctive(conjunctive)
	{
		load_vocabulary(voc,store_dir.c_str());
	}

	/**Get the set of documents where a given term appears.
	 *
	 * @param[out] docs Docids of matching documents will be @b appended
	 * 		    to this list.
	 *
	 * @note We do not clear @p docs.
	 *
	 * @deprecated This function is clearly not needed - our
	 * 		inverted_lists already provide docids that are uniq
	 * 		and sorted in ascending order. Replaced by
	 * 		@c getTermDocvet().
	 *
	 * @see getTermDocvet()
	 *
	 */
	void getTermDocset(const std::string& term, docset& docs)
	{
		inverted_list_vec_t ilist;

		getWordInvertedList(term, ilist);
		ilist2docset(ilist,docs);
	}

	/**Get a list of documents where a given term appears.
	 *
	 * @param[out] docs Docids of matching documents will be @b appended
	 * 		    to this list.
	 *
	 * @note We @b clear @p docs.
	 *
	 */
	void getTermDocvet(const std::string& term, docvet& docs)
	{
		docs.clear();

		inverted_list_vec_t ilist;
		inverted_list_vec_t::const_iterator d;

		getWordInvertedList(term, ilist);
		docs.reserve(ilist.size());

		for(d = ilist.begin(); d != ilist.end(); ++d ){
			docs.push_back(d->first);
		}
	}

	void getWordInvertedList(std::string term, inverted_list_vec_t& ilist)
	{
		std::string word = normalize_term(term);

		StrIntMap::const_iterator wpos = voc.find(word);
		if (wpos == voc.end()){
			std::string err_msg("Term '");
			err_msg +=  term + "' (" +  word +
				") not in vocabulary";
			throw std::runtime_error(err_msg);
		}

		uint32_t termid = wpos->second;

		getTermIdInvertedList(termid,ilist);

	}

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

	void ilist2docset(const inverted_list_vec_t& ilist, docset& ids )
	{
		inverted_list_vec_t::const_iterator i;

		for(i = ilist.begin(); i != ilist.end(); ++i){
			ids.insert(i->first);
		}
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
	void processQuery(std::string query, docvet& result)
	{
		std::vector<std::string> words;
		std::vector<std::string>::const_iterator w;

		result.clear();
		words = split(query," ");

		if(words.empty()) return;

		// We populate the result with the hits for the first query
		// term and we add/remove docids to it as we iterate over
		// the other query terms
		w=words.begin();
		getTermDocvet(*w,result);
		++w; //Continue at the second term (if it exists)...
		for(; w != words.end(); ++w) {

			// Is this an query operator? if so, handle it.
			if (*w == "AND" ) {
				conjunctive = true;
				continue;
			} else if ( *w == "OR") {
				conjunctive = false;
				continue;
			}

			// Not an operator? Process this query term
			docvet cur;		// Current term's matches
			docvet prev(result);	// previous result
			result.clear();

			getTermDocvet(*w, cur);

			if (conjunctive) {
				std::set_intersection(prev.begin(), prev.end(),
					cur.begin(), cur.end(),
				 	std::inserter(result,result.begin()));
			} else {
				std::set_union(prev.begin(), prev.end(),
					cur.begin(), cur.end(),
				 	std::inserter(result,result.begin()));
			}
		} //end for
	}


};


/***********************************************************************
				      MAIN
 ***********************************************************************/

void do_query(QueryResolver& resolver, std::string query)
{
	docvet d_ids;
	resolver.processQuery(query,d_ids);


	if (d_ids.empty()) {
		std::cout << "No documents found matching query '" << query <<
			"' in documents." << std::endl;
	} else {
		std::cout << "Document(s) found matching query '" <<
				query << "': ";
		docvet::const_iterator i;
		for(i = d_ids.begin(); i != d_ids.end(); ++i){
			std::cout << " " << *i;
		}
		std::cout << std::endl;
	}

}

void show_usage()
{
        std::cout << 	"Usage:\n"
			"dump_index store_dir\n"
			"\tstore_dir\twhere the index is."<< std::endl;
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
	QueryResolver resolver(store_dir.c_str());
	std::cout << "done" << std::endl;

	std::cout <<"Type your query using AND, OR and spaces to split terms."<<std::endl;
	std::cout <<"Default operation is AND (conjunctive)."<<std::endl;
	std::cout <<"Notice: a AND b c OR d == (((a AND b) AND c) OR d )"<<std::endl;

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
