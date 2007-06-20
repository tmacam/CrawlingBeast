// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __INDEXERUTILS_H__
#define __INDEXERUTILS_H__
/**@file indexerutils.hpp
 * @brief Utilities helper classes for text indexing.
 */



#include "common.h"
#include "strmisc.h"

#include "htmliterators.hpp"

#include <iterator>
#include <iostream>

#include <ext/hash_map>
#include <tr1/functional>
#include <map>
#include <string>
#include <list>
#include <iomanip> // for make_run_filename

/***********************************************************************
			     Typedefs and constants
 ***********************************************************************/

typedef __gnu_cxx::hash_map < std::string, int,
                        std::tr1::hash<std::string> > StrIntMap;

typedef std::map<int,std::string> IntStrMap;

/*We could have used a uint16_t for freq. but this would not matter anyway as
 * this struct is not __attribute__((packed))*/
typedef std::pair<uint32_t, uint32_t> d_fdt_t;

typedef std::list<d_fdt_t> inverted_list_t;

typedef std::vector<d_fdt_t> inverted_list_vec_t;




/***********************************************************************
				 RUN ITERATORS
 ***********************************************************************/

/** Stores <term_id, doc_id, freq> triples.
 *
 * Theses triples are mostly used while generating "runs", hence the the
 * name "run_triple"
 */
struct run_triple{

	uint32_t termid;
	uint32_t docid; // docid_t is a unit32_t
	uint16_t freq;

	run_triple(uint32_t tid = 0, uint32_t did=0, uint16_t f=0)
	: termid(tid), docid(did), freq(f)
	{}

	/**Comparison operator (less than).
	 *
	 * The smaller run_triple is the one with the smaller termid.
	 * Docid is used to resolve ties,  winning the one with the smallest
	 * one.
	 */
	inline bool operator< (const run_triple& other) const
	{
		return (termid < other.termid) ||
			(termid == other.termid && docid < other.docid);
	}

	inline bool operator==(const run_triple& other) const
	{
		return this->termid == other.termid && 
			this->docid == other.docid &&
			this->freq == other.freq;
	}

	friend std::ostream& operator<<(std::ostream& out, const run_triple& tri)
	{
		out << "<" << tri.termid << "," <<
				tri.docid << "," <<
			     	tri.freq << ">";
		return out;
	}
	
} __attribute__((packed));



/***********************************************************************
				 RUN ITERATORS
 ***********************************************************************/

/**Inserter or output interator for runs.
 *
 * This is just syntatic sugar for writing runs to disk.
 *
 * @warning This class ONLY flushes all it's data on descruction.
 */
class run_inserter :public std::iterator<std::output_iterator_tag, run_triple, void,
					void, void>
{
private:
	std::string path_prefix;
	size_t max_length; //!< Max ammount of bytes we reserve to store triples
	size_t max_triples; //!< Max number of tripes we hold in memory
	
	run_triple* run_buf; //!< Triples buffer
	run_triple* cur_triple; //!< The position of the next triple writing pos
	run_triple* end; //!< Past-the-last-triple-in-buffer position

	int n_runs; //!< Number of runs flushed to disk so far


	//! Not default constructible
	run_inserter();

	//! Prevent Copying and assignment.
	run_inserter(const run_inserter& );

	//! Prevent Copying and assignment.
	run_inserter& operator=(const run_inserter&);
public:
	/**Constructor.
	 *
	 * @param prefix prefix where the run_?? file will be stored.
	 *        It can include a full path.
	 * @param length max ammount in bytes of memory will be used to hold
	 * 		 a run in memory before flushing it to a run file.
	 *
	 */
	run_inserter(const std::string prefix, size_t length)
	: path_prefix(prefix),
	  max_length(length),
	  max_triples( max_length / sizeof(run_triple)),
	  run_buf( new run_triple[ max_triples ]),
	  cur_triple(run_buf),
	  end(&cur_triple[max_triples]),
	  n_runs(0)
	{}

	~run_inserter()
	{
		try {
			flush();
		} catch(...) {
			std::cerr << "An error ocurred in run_inserter" << 
				std::endl;
		}

		delete[] run_buf;
	}

	inline run_inserter& operator=(const run_triple& val)
	{
		// Add current
		*cur_triple++ = val;

		if(cur_triple == end) {
			flush();
		}

		return *this;
	}

	inline run_inserter& operator*() {return *this;}	// deref
	inline run_inserter& operator++() {return *this;}	// prefix
	inline run_inserter& operator++(int) {return *this;}	// postfix

	/** Write the current pending/remaining triples to disk.
	 *
	 * You should call this function before instance desctruction.
	 * Although this class destructor calls flush any possible
	 * raised exception will be consumed and ignored if called inside
	 * the destructor.
	 */
	void flush();

	/**Make a run filename from a prefix and a run number.*/
	static std::string make_run_filename(std::string path_prefix, int n)
	{

		std::ostringstream filename_stream;
		filename_stream << path_prefix << "/run_" <<
			std::hex <<  std::setw(4) << std::setfill('0') <<
			n; //"%s/run_%04x"
		return filename_stream.str();

	}


	
	int getNRuns() const {return n_runs;}

};

/***********************************************************************
		     VOCABULARY RETRIEVAL AND NORMALIZATION
 ***********************************************************************/


struct Isalpha : std::unary_function<char, bool> {
	bool operator()(char c) {
		return not (std::ispunct(c) || std::isspace(c));
	}
};

struct WIsalpha : std::unary_function<wchar_t, bool> {
	static const wchar_t control_code_start = 0x007f; // [control] DELETE
	static const wchar_t control_code_end = 0x00A0; // NBSP
	bool operator()(wchar_t c) {
		return not ( (c >= control_code_start && c <= control_code_end)
				|| std::iswpunct(c) || std::iswspace(c));
	}
};

/**Normalize a term or word.
 *
 * For now, normalization means:
 * - lowercasing
 * - converting accented letters to their corresponding representations without
 *   accents.
 *
 * FIXME wouldn't a hashtable be better?
 * FIXME Are we really ignoring non-latin1 vowels? see
 * 	 http://www.windspun.com/unicode-test/unicode.xml
 * 	 http://www.decodeunicode.org/w3.php?viewMode=block&ucHex=0080
 * 	 and look for letters such as U+0100 ..
 */
inline std::wstring& normalize_term(std::wstring& word)
{
	to_lower(word);
	//filter_accents(word);
	for(size_t i = 0; i < word.size(); ++i){
		wchar_t& c = word[i];
		if (c >= 224 && c <= 230 ) { c = L'a';}
		if (c == 231 ) { c = L'c';}
		if (c >= 232 && c <= 235 ) { c = L'e';}
		if (c >= 236 && c <= 239 ) { c = L'i';}
		if (c == 241 ) { c = L'n';}
		if (c >= 242 && c <= 248 ) { c = L'o';}
		if (c >= 249 && c <= 252 ) { c = L'u';}
		if (c >= 253 && c <= 255 ) { c = L'y';}
	}
	return word;
}


/**Retrieve the term or word frequency for a given document.
 *
 * @param f The HTML file from which the term frequency will be extracted.
 * @param[out] wfreq The term frequency dictionary. Will be cleared upon
 * 			function start.
 * @param wconv	wide-string converter. Used only for caching and
 * 		performance purposes.
 *
 * @param docid is used just for error reporting and can be ignored in commom
 * 		usage.
 *
 *
 * wfreq is cleared at every function call.
 */
void getWordFrequency(filebuf f, StrIntMap& wfreq,
			const WideCharConverter& wconv, docid_t docid=0);


/***********************************************************************
			       INDEXING FUNCTIONS
 ***********************************************************************/

/**Convert a docid into a filepath of where the contents of this docid is.
 *
 * Our crawler saves the contents of a the page corresponding to a given
 * docif as "<store_dir>/<docid in hex, splited every 2 characters>/data.gz".
 *
 * For instance, suppose that our store path is "/tmp/down" and that we want
 * the contents of the docume whose docid is 1. Then the path to this document
 * contents is "/tmp/down/00/00/00/01/data.gz".
 *
 */
std::string make_filename(std::string store_dir, docid_t docid);

/**Get the reverse mapping from a vocabulary.
 *
 * The vocabulary is a map from terms to their corresponding term Id.
 * This function constructs the reverse mapping. The parameter @p id2term
 * is used to return the resulting mapping.
 *
 * @param[in] term2id The vocabulary.
 * @param[out] id2term The reverse mapping of the vocabulary.
 */
void mapIdToTerms(const StrIntMap& term2id, IntStrMap& id2term);

/**Write the contents of a vocabulary dict/map to disc.
 *
 * The vocabulary constents is stored as in two files:
 * - @c vocabulary.data
 *
 *   	This files holds the concatenation of all terms in the vocabulary,
 *   	separated by '\0'. All terms are sequential and ascending order
 *   	of their correspondinng term id.
 *
 * - @c vocabulary.hdr
 *
 *       This file holds a vector sequentialy maps term ids (uint32_t) into
 *       the the position of this term as a string inside the file
 *       @c vocabulary.data
 *
 *
 * @param[in] vocabulary The map holding the "term -> id " mapping.
 * @param[in] output_dir Path to the directory where the dump of the
 * 			 vocabulary will be stored.
 *
 * @note We assume that we have a sequential ordering of the term ids.
 */
void dump_vocabulary(const StrIntMap& vocabulary, const char* output_dir);


/**Load a dumped vocabulary.
 *
 * @see dump_vocabulary
 *
 * @param[out] vocabulary Map where the vocabulary will be loaded.
 * @param[in] store_dir	Directory where the dumped vocabulary is.
 *
 */
void load_vocabulary(StrIntMap& vocabulary, const char* store_dir);



/**Create an index for a given vocabulary.
 *
 * Actually, we only create the runs...
 *
 * @param store_dir Path where the crawler saved the pages to be indexed.
 *
 * @param docids_list 	Full path to a file holding an ordered list of valid
 * 			docids and theirs corresponing urls, as saved by the
 * 			crawler.
 *
 * @param output_dir Path where the indexing "runs" will be created.
 *
 */
void index_files(const char* store_dir, std::vector<docid_t> docids_list,
		const char* output_dir, unsigned int run_size= 100*1024);


void prefetchDocs(const char* store_dir, std::vector<docid_t>& ids);

#endif // __INDEXERUTILS_H__

//EOF
