#include "zfilebuf.h"

#include <iostream>
#include <list>
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

// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
