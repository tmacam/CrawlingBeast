#include "config.h"
#include "zfilebuf.h"

#include <string.h> // for memcpy

#include <iostream>
#include <list>
#include <deque>
#include <memory>

filebuf decompres(const std::string filename)
{
	const int BUF_LEN = 10*1024;

	std::list< filebuf > buffers;
	std::list< filebuf >::iterator bi;
	int len = 0;
	int total_len = 0;

	GZFileWrapper input(filename.c_str());

	try {
		while (!input.eof()) {

			// Hell could freeze, sky could fall -- no matter
			// what happens, memory allocated here will be
			// released if something goes wrong.
			std::auto_ptr<char> buf(new char[BUF_LEN]);

			len = input.read(buf.get(),BUF_LEN);

			// OK, we successfuly read some of the compressed file
			// Get rid of the auto_ptr and add this buffer to our
			// list of known buffers -- they will be delete[]
			// later.
			buffers.push_back(filebuf(buf.release(),len));

			total_len += len;
		}
	} catch(...) {
                // There can be allocated memory hanging around in buffers...
                for(bi = buffers.begin(); bi != buffers.end(); ++bi){
                        delete[] bi->current;
                }

                throw;
        }

	char* final_buffer = new char[total_len];
	filebuf res(final_buffer, total_len);

	// Copy the uncompressed data to the resulting output.
	// Free all previously allocated buffers.
	for(bi = buffers.begin(); bi != buffers.end(); ++bi){
		const char* buf = bi->current;
		len = bi->len();

		memcpy(final_buffer, buf, len );
		final_buffer += len;
		delete[] buf;
	}

	return res;
}


// The overloaded version that deals only with gzip-content inside a filebuf.

filebuf decompress(filebuf data)
{
	/* This code is based on zLib's usage example, available 
	 * at http://www.zlib.net/zlib_how.html
	 */

	// Zlib stuff
	int ret;
	z_stream strm;
	unsigned int have;
	// Outputing stuff
	size_t OUTBUF_LEN = DECOMPRESS_RESERVE; 
	char outbuf[OUTBUF_LEN];
	std::vector<char> accbuf; // accumulator buffer
	accbuf.reserve(DECOMPRESS_RESERVE);

	
	if (data.len() == 0) {
		// Empty filebufs result in empty filebufs.
		return filebuf();
	}

	/* allocate inflate state */
	const int windowBits = 15 + 16; /* 15 is the default value for 
					 * windownBits in inflateInit().
					 *
					 * Bits+16 means "use gzip" in
					 * zlib >= 1.2. Use 32 instead
					 * of 16 to force automatic header
					 * detection for gzip/zlib.
					 */
	strm.zalloc = Z_NULL;
	strm.zfree = Z_NULL;
	strm.opaque = Z_NULL;
	strm.avail_in = data.len();
	strm.next_in = Z_NULL;

	ret = inflateInit2(&strm, windowBits);
	if (ret != Z_OK){
		throw ZLibException("in inflateInit2", ret);
	}

	strm.avail_in = data.len();
	strm.next_in = (unsigned char*) data.current;

	/* decompress until deflate stream ends or an error happens */
	do {
		/* Remember: all input data _is_ available! All we can and
		 * must do is loop until zlib reports that it has
		 * finished or an error occurs.
		 */

		/* run inflate() on input until output buffer not full */
		do {

			strm.avail_out = OUTBUF_LEN;
			strm.next_out = (unsigned char*) outbuf;

			ret = inflate(&strm, Z_NO_FLUSH);
			// make sure state not clobbered
			if (ret == Z_STREAM_ERROR) {
				throw ZLibException("in inflate", ret);
			}
			switch (ret) {
				case Z_NEED_DICT:
					ret = Z_DATA_ERROR; // and fall through
				case Z_DATA_ERROR:
				case Z_MEM_ERROR:
					(void)inflateEnd(&strm);
					throw ZLibException("in inflate", ret);
			}

			have = OUTBUF_LEN - strm.avail_out;

			accbuf.insert(	accbuf.end(),
					outbuf,
					&outbuf[have]);
		} while (strm.avail_out == 0);

		/* done when inflate() says it's done */

	} while(ret != Z_STREAM_END);

	/* clean up */
	(void)inflateEnd(&strm);

	/* Ugly and lazy copy to an array */
	char* final_buffer = new char[accbuf.size()];
	std::copy(accbuf.begin(), accbuf.end(), final_buffer);

	return filebuf(final_buffer, accbuf.size());

}


std::string ZLibException::getErrorMessage(int code)
{
	using std::string;

	switch (code) {
		case 0:
			return string("");
			break;
		case Z_ERRNO:
			return string("Unknown system error - consult errno.");
			break;
		case Z_STREAM_ERROR:
			return string("Invalid compression level");
			break;
		case Z_DATA_ERROR:
			return string("Invalid or incomplete deflate data");
			break;
		case Z_MEM_ERROR:
			return string("Out of memory");
			break;
		case Z_VERSION_ERROR:
			return string("zlib version mismatch!");
			break;
		default:
			return string("Unknown error w/ zlib.");
	}
}




// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
