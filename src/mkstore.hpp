// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __MKSTORE_H__
#define __MKSTORE_H__
/**@file mkstore.hpp
 * @brief Build a document store from set of crawled and downloaded docids.
 *
 * The document store consists of two parts:
 *
 * - An index file, store.idx
 *
 *   Which stores a fixed width index. Each entry in this index is a
 *   @c store_hdr_entry_t
 *
 * - A group of store data files (store.data_xxxx).
 *
 *   Each store data file consists of a sequence of
 *
 *   - docid
 *   - length of the document contents compressed
 *   - document contents
 *
 */

#include "common.h"

/***********************************************************************
				    Typedefs
 ***********************************************************************/

/**Structure for document store index's entries.
 */
struct store_hdr_entry_t {
	uint32_t docid;	//!< DocId of the document
	uint16_t fileno; /**< Number of the store data file holding the contents
			 *   of the document with id @p docid
			 */
	uint32_t pos;	/**< Position of the document in the document store 
			 *   data file with number @c fileno
			 */


	store_hdr_entry_t(uint32_t id = 0, uint16_t n=0, uint32_t position=0)
	: docid(id), fileno(n),  pos(position)
	{}
} __attribute__((packed));


struct store_data_entry_t {
	uint32_t docid;	//!< DocId of the document
	uint32_t len;	//!< Length of the document

	store_data_entry_t(uint32_t _id=0, uint32_t _len=0)
	: docid(_id), len(_len)
	{}

} __attribute__((packed));






#endif // __MKSTORE_H__

//EOF



