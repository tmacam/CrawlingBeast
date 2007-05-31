// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __INDEXER_H
#define __INDEXER_H
/**@file indexer.h
 * @brief Indexing functions and classes.
 *
 */

#include <ext/hash_map>
#include <tr1/functional>
#include <map>
#include <string>

/***********************************************************************
			     Typedefs and constants
 ***********************************************************************/

typedef __gnu_cxx::hash_map < std::string, int,
                        std::tr1::hash<std::string> > StrIntMap;

typedef std::map<int,std::string> IntStrMap;


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
	
} __attribute__((packed));




#endif // __INDEXER_H


