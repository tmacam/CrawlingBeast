// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#ifndef __VECTORIAL_H__
#define __VECTORIAL_H__
/**@file vectorial.h
 * @brief Vectorial Model defines, typedefs and misc. definitions.
 *
 */

#include <ext/hash_map>
#include <tr1/functional>

#include "common.h"


/***********************************************************************
			     Typedefs and constants
 ***********************************************************************/

/**Preprocessed \f$W_d\f$ and \f$\max_{i}f_{i,d}\f$ values.
 *
 * \f$W_d\f$ is the norm or module of the vector of a document, and is
 * given by the form \f$\sqrt{\sum_{i\subset d}w_{t,d}^{2}}\f$,
 * where \f$w_{t,d}=\frac{f_{t,d}}{\max_{i}f_{i,d}}\cdot\log\frac{N}{f_{t}}\f$
 *
 * @todo move this definition to vectorspace.hpp
 */
struct wdmaxfdt_t {
	double wd;
	uint32_t maxfdt;

	//!Constructor
	wdmaxfdt_t(double w=0, uint32_t max=1)
	: wd(w), maxfdt(max)
	{}
}__attribute__((packed));

typedef __gnu_cxx::hash_map < docid_t, wdmaxfdt_t> wdmfdt_map_t;

/**Structure for document's norm store index's entries.
 */
struct norm_hdr_entry_t {
	uint32_t docid;	//!< DocId of the document
	uint16_t fileno; /**< Number of the store data file holding the contents
			 *   of the document with id @p docid
			 */
	uint32_t pos;	/**< Position of the document in the document store 
			 *   data file with number @c fileno
			 */


	norm_hdr_entry_t(uint32_t id = 0, uint8_t n=0, uint32_t position=0)
	: docid(id), fileno(n),  pos(position)
	{}
} __attribute__((packed));







#endif // __VECTORIAL_H__

// EOF
