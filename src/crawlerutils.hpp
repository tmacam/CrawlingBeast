// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __CRALWERUTILS_H__
#define __CRALWERUTILS_H__
/**@file crawlerutils.hpp
 * @brief Functions and typedefs to help dealing with crawled content.
 *
 */

#include <fstream>
#include <sstream>
#include <iomanip>
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

inline std::string make_crawler_filename(std::string store_dir, docid_t docid)
{
	std::ostringstream id_hex;

	id_hex << std::uppercase << std::hex << std::setw(8) <<
                std::setfill('0') << docid;
	std::string id_hex_str = id_hex.str();

	std::string id_path =  store_dir + "/" + 
				id_hex_str.substr(0,2) + "/" +
				id_hex_str.substr(2,2) + "/" +
				id_hex_str.substr(4,2) + "/" +
				id_hex_str.substr(6,2) + "/data.gz"; // FIXME data.gz constant
	return id_path;
}



#endif // __CRALWERUTILS_H__


// EOF
