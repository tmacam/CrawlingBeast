// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __MKPREPR_HPP__
#define __MKPREPR_HPP__
/**@file mkprepr.hpp
 * @brief Pre-PageRank definitions
 */

#include "fnv1hash.hpp"
#include "mkstore.hpp" // For store_hdr_entry_t and store_data_entry_t


/***********************************************************************
				    Typedefs
 ***********************************************************************/

typedef hash_map <uint32_t, std::string> TIdUrlMap;

//!@name URL Fingerprint containers
//!@{
typedef hash_set < uint64_t > TURLFingerprintSet;

typedef std::vector<uint64_t> TURLFingerprintVec;
//!@}

//!@name Pre-PR Index Store structures
//!@{
//!To read the header of an entry in the header file
typedef store_hdr_entry_t prepr_hdr_entry_t;

/**To read header and contents of an entry in a data entry.
 *
 * The idea is that a data entry has an header and contents
 * and that the header @p len is the length of the data entry
 * contents.
 *
 * After this data header an array of uint64_t[n_outlinks]
 * follows.
 *
 */
struct prepr_data_entry_t {
	uint32_t docid;	//!< DocId regarding this data entry.
	uint32_t len;   /**< Length of the data inside this ISAM data entry
			 *   after this header.
			 */
	uint64_t fp;		//!< fingerprint
	uint32_t n_outlinks;	//!< title of the document

	prepr_data_entry_t( uint32_t _id=0, uint32_t _len=0,
				uint64_t fingerprint=0, uint32_t n=0)
	: docid(_id), len(_len),
	  fp(fingerprint), n_outlinks(n)
	{}

} __attribute__((packed));

//!@}


/***********************************************************************
			       Auxiliary Functors
 ***********************************************************************/

// Stroustrup, chap. 18.6.1
template<class In,class Out,class Pred>
Out copy_if(In first,In last,Out res,Pred p)
{
	while(first!=last){
		if(p(*first))*res++=*first;
		++first;
	}
	return res;
}

template<class T>
struct IsInMap {
	T& m;
	typedef typename T::key_type key_type;

	IsInMap(T& _m)
	: m(_m)
	{}

	inline bool operator()(const key_type& val)
	{
		return (m.find(val) != m.end());
	}
};

inline TIdUrlMap& read_ids_and_urls(const char * docid_list, TIdUrlMap& id2url)
{
	std::ifstream known_docids(docid_list);
	std::string url;
	docid_t docid;
	while(known_docids >> docid >> url){
		id2url[docid] = url;
	}

	return id2url;

}




#endif // __MKPREPR_HPP__

//EOF
