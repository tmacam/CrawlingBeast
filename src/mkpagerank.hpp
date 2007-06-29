// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __MKPAGERANK_H__
#define __MKPAGERANK_H__
/**@file mkpagerank.hpp
 * @brief Typedefs for reading and writing pagerank data.
 */

#include "fnv1hash.hpp"

/***********************************************************************
				    Typedefs
 ***********************************************************************/

//!@name PageRank-related containers
//!@{

//!Simple class to accumulate estimate PageRank values for pages.
typedef hash_map< uint64_t, float> TFPPageRankAccumulator;

typedef hash_map<uint64_t, uint32_t> TFP2Id; // FIXME

struct pagerank_hdr_entry_t{
	uint32_t docid;
	float   pagerank;

	explicit pagerank_hdr_entry_t(uint32_t d, float pr)
	: docid(d), pagerank(pr)
	{}
} __attribute__((packed));
//!@}






#endif // __MKPAGERANK_H__


// EOF
