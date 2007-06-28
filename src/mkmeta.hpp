// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __MKMETA_H__
#define __MKMETA_H__
/**@file mkmeta.hpp
 * @brief Typedefs to read metadata dump-files.
 */

#include "common.h"
#include "isamutils.hpp"
#include "fnv1hash.hpp"
#include "mmapedfile.h"
#include "mkstore.hpp" // For store_hdr_entry_t and store_data_entry_t

/***********************************************************************
                             Typedefs and constants
 ***********************************************************************/


//!To read the header of an entry in the header file
typedef store_hdr_entry_t meta_hdr_entry_t;

/**To read header and contents of an data entry.
 *
 * The idea is that a data entry has an header and contents
 * and that the header @p len is the length of the data entry
 * contents.
 *
 */
struct meta_data_entry_t {
	uint32_t docid;	//!< DocId regarding this data entry.
	uint32_t len;   /**< Length of the data inside this ISAM data entry
			 *   after this header.
			 */
	uint32_t url;	//!< document's url length
	uint32_t title;	//!< document's title length

	meta_data_entry_t( uint32_t _id=0, uint32_t _len=0,
			   uint32_t _u=0, uint32_t _t=0)
	: docid(_id), len(_len),
	  url(_u), title(_t)
	{}

} __attribute__((packed));



/***********************************************************************
				   TMetaBase
 ***********************************************************************/



/**Simple and to the point mkmeta output reader.
 *
 * This class was made to retrieve information on URL and
 * title for pages, collected by mkmeta.
 *
 * @todo this class should be generalized and moved to isamutils
 */
class TMetaBase {
public:
	typedef hash_map<uint32_t, const meta_hdr_entry_t*> TMetaMap;
	typedef std::pair<std::string,std::string> TUrlTitle;
protected:
	std::string name;
	std::string path;
	MMapedFile mmheader;
	TMetaMap metamap;

	// Not default constructible and not assignable
	TMetaBase& operator=(const TMetaBase&);
	TMetaBase(const TMetaBase&);
	TMetaBase();
public:
	TMetaBase(std::string path)
	: name("meta"),
	  path(path),
	  mmheader(path + "/" + name + ".hdr")
	{
		filebuf data = mmheader.getBuf();
		// Load header data into memory
		const meta_hdr_entry_t* p = (meta_hdr_entry_t*) data.start;
		const meta_hdr_entry_t* end = (meta_hdr_entry_t*)data.end;
		while(p < end) {
			metamap[p->docid] = p;
			++p;
		}
	}

	TUrlTitle getMetaData(docid_t id)
	{
		// Unknown id?
		if (metamap.find(id) == metamap.end()) {
			return TUrlTitle();
		}

		// Open the data store for this entry and retrieve it
		const meta_hdr_entry_t* hdr = metamap[id];
		std::string store_name = mk_isam_data_filename( path, 
								name,
								hdr->fileno);
		MMapedFile mdata(store_name);
		filebuf raw_data = mdata.getBuf();
		raw_data.read(hdr->pos);

		// Read data header
		meta_data_entry_t* data_hdr = 0;
		data_hdr = readFromFilebuf<meta_data_entry_t>(raw_data);
		filebuf entry_data = raw_data.readf(data_hdr->len);

		// Grab the data we want
		filebuf url_data = entry_data.readf(data_hdr->url);
		filebuf title_data = entry_data.readf(data_hdr->title);

		return TUrlTitle(url_data.str(), title_data.str());

	}
};






#endif // __MKMETA_H__


// EOF
