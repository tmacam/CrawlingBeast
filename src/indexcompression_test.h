#ifndef __INDEXCOMPRESSION_TEST_H
#define __INDEXCOMPRESSION_TEST_H

#include "indexcompression.hpp"
#include "cxxtest/TestSuite.h"

#include <algorithm>

class ByteWiseCompressorParserTestSuit : public CxxTest::TestSuite {
public:
	void test_Encode()
	{
		
		ByteWiseCompressor::charvec_t out;
		out.reserve(10);

		uint8_t val_1044[] = {147, 7, 0};
		uint32_t val = 1044;

		ByteWiseCompressor::encode(val, out);

		TS_ASSERT_EQUALS(out.size(), 2);
		TS_ASSERT_EQUALS(out[0], val_1044[0]);
		TS_ASSERT_EQUALS(out[1], val_1044[1]);
	}

	void test_Decode()
	{
		ByteWiseCompressor::charvec_t out;

		uint8_t val_1044[] = {147, 7, 10};
		filebuf in((char*)val_1044, sizeof(val_1044));
		uint32_t res = 0;

		ByteWiseCompressor::decode(in, res);
		TS_ASSERT_EQUALS(res, 1044);
		
	}

	void test_enchained_reversibility()
	{
		uint32_t res = 0;
		ByteWiseCompressor::charvec_t out;

		ByteWiseCompressor::encode(10123, out);
		ByteWiseCompressor::encode(123, out);
		ByteWiseCompressor::encode(1, out);
		ByteWiseCompressor::encode(65123, out);
		filebuf in((char*)&out[0], out.size());

		ByteWiseCompressor::decode(in, res);
		TS_ASSERT_EQUALS(res,10123);

		ByteWiseCompressor::decode(in, res);
		TS_ASSERT_EQUALS(res,123);

		ByteWiseCompressor::decode(in, res);
		TS_ASSERT_EQUALS(res,1);

		ByteWiseCompressor::decode(in, res);
		TS_ASSERT_EQUALS(res,65123);


	}

	void test_CompressDecompress()
	{
		inverted_list_t da_list;
		da_list.push_back(d_fdt_t(10,20));
		da_list.push_back(d_fdt_t(100,2));
		da_list.push_back(d_fdt_t(102,2000));
		da_list.push_back(d_fdt_t(205,200));
		da_list.push_back(d_fdt_t(1000,5));
		
		uint32_t count = da_list.size();

		ByteWiseCompressor::charvec_t out;

		ByteWiseCompressor::compress(da_list, out);

		filebuf buf((char*)&out[0], out.size());
		inverted_list_vec_t res;

		ByteWiseCompressor::decompress(count,buf,res);

		TS_ASSERT_EQUALS(count, res.size());
		TS_ASSERT( std::equal(da_list.begin(), da_list.end(),
						res.begin()));

	}

};


#endif // __INDEXCOMPRESSION_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
