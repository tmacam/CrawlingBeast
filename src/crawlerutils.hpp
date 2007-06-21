// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __CRALWERUTILS_H__
#define __CRALWERUTILS_H__
/**@file crawlerutils.hpp
 * @brief Functions and typedefs to help dealing with crawled content.
 *
 */

#include <fstream>
#include "config.h"

/***********************************************************************
				    Typedefs
 ***********************************************************************/


typedef std::vector<docid_t> docid_vec_t;


/***********************************************************************
				Misc. Functions
 ***********************************************************************/


/**Read the docids listed in a docid-url list.
 *
 * @param[out] ids Vector where the docids will be added.
 *
 * @return Return @p ids, filled with all the docids read
 */
inline docid_vec_t& read_docid_list(const char* docid_list, docid_vec_t& ids)
{
        ids.reserve(DOCIDLIST_RESERVE);
        std::ifstream known_docids(docid_list);
        std::string url;
        docid_t docid;
        while(known_docids >> docid >> url){
                ids.push_back(docid);
        }

	return ids;
}


#endif // __CRALWERUTILS_H__


// EOF
