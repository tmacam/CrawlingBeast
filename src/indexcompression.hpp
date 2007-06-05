// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __INDEXCOMPRESSION_H__
#define __INDEXCOMPRESSION_H__
/**@file indexcompression.hpp
 * @brief Index compression functors.
 */

#include "filebuf.h"	// We made this f*cker for byte processing, right?
#include "indexerutils.hpp"

/***********************************************************************
			     Typedefs and constants
 ***********************************************************************/

/***********************************************************************
			 ByteWise Compression Functions
 ***********************************************************************/

/**Byte-wise compression routines for inverted lists.
 *
 * For more information on the methods implemented here see the survey from
 * Zobel and Moffat, entitled "Inverted files for text search engines",
 * ACM, 2006. http://doi.acm.org/10.1145/1132956.1132959
 *
 * Yeah, we are using a struct just like if it was a namespace holder
 * - so what?
 *
 */
struct ByteWiseCompressor {

	typedef std::vector<uint8_t> charvec_t;

	static const uint32_t bit_128 = 128; // bit 128 on
	static const uint32_t low_128 = 0x7f; // <128 bits on
	static const uint32_t hi_128 = ~low_128; // >=128 bits on 

	inline static void encode(uint32_t value, charvec_t& output)
	{
		assert(value > 0);

		value = value -1;

		while(value & hi_128) {	// Value >= 128
			// write ( 128 + x mod 128)
			output.push_back( bit_128 | (low_128 & value));
			// value =  (value div 128) -1
			value = (value >> 7) - 1; 
		}
		output.push_back(value);

	}

	inline static const void decode(filebuf& bytes, uint32_t& value)
	{
		uint8_t b = *bytes.read(1);

		value = 0;
		uint32_t p = 0;

		while(b & hi_128) {		   //while value >= 128
			value += ((b - 127) << p); //	x +=  (b-127) x p
			p += 7;			   //	p *= 128
			b = *bytes.read(1);	   //   b = read_byte()
		}
		value = value + ((b+1) << p);	   //	x += (b+2)xp	
	}


	
	/**Compress an inverted list.
	 *
	 * Document id (stored as d-gaps) and term frequency in the documents
	 * are  both stored using this class' byte-wise encoding method.
	 *
	 * @note We expect to see ascending docids in the list.
	 *
	 * @param[in] in inverted list to compress
	 * @param[out] out compressed output.
	 */
	inline static void compress(const inverted_list_t& ilist, charvec_t& output)
	{
		inverted_list_t::const_iterator i = ilist.begin();
		uint32_t last_doc = 0;
		uint32_t dgap = 0;

		for(i = ilist.begin(); i != ilist.end(); ++i){
			const d_fdt_t&  d_ft= *i;

			dgap = d_ft.first - last_doc;
			encode(dgap, output);
			encode(d_ft.second, output);

			last_doc = d_ft.first;
		}
	}

	/**Decompres.
	 *
	 * @param count number of entries (tf)
	 * @param[in,out] in compressed list
	 * @param[out] out decompressed list
	 */
	inline static void decompress(uint32_t count, filebuf in, inverted_list_vec_t& out)
	{
		out.reserve(count);

		uint32_t last_docid = 0;
		uint32_t dgap;
		uint32_t fdt;

		for(uint32_t i = 0; i < count ; ++i){
			decode(in, dgap);
			decode(in,fdt);
			last_docid += dgap;
			out.push_back(d_fdt_t(last_docid, fdt));
		}
	}
};





#endif // __INDEXCOMPRESSION_H__

//EOF
