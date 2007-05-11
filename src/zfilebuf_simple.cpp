#include "gzstream.h"

#include <iostream>
#include <list>
#include <memory>
#include <stdexcept>
#include <utility>

typedef std::pair<char*,int> memory_buffer;

class GZFileWrapperException: public std::runtime_error {
public:
	GZFileWrapperException (std::string msg="")
                :runtime_error(msg){}
};

/**RAII-like wrapper for gzFile.
 *
 */
class GZFileWrapper {
public:
	gzFile fh;

	GZFileWrapper(const char* filename, const char* mode = "r")
	: fh(NULL)
	{
		if (NULL == (fh = gzopen(filename, mode)) ) {
			throw GZFileWrapperException("gzopen");
		}
	}

	~GZFileWrapper()
	{
		if (fh) {
			gzclose(fh);
		}
	}

	bool eof()
	{
		if (fh == NULL){
			throw GZFileWrapperException("File not opened");
		}

		return gzeof(fh) == 1;
	}

	int read(char* buf, unsigned len)
	{
		int ret = 0;

		ret = gzread(fh,buf,len);

		if (ret < 0) {
			throw GZFileWrapperException("error in gzread");
		}

		return ret;
	}
};

/**Decompresses a gzip file into a memory_buffer.
 *
 * Use zlib's functions to read and decompress a gzip file into memory.
 * The memory needed to hold the decompressed file is dinamically allocated
 * by this function.
 *
 * @param filename The name of the gzip file to decompress.
 *
 * @warning It is your responsability to release (whith delete[]) the memory
 * allocated by this function.
 *
 * @throw GZFileWrapperException if anything went wrong while
 * decompressing the file pointed by filename.
 */
memory_buffer decompres(const std::string filename)
{
	const int BUF_LEN = 10*1024;

	std::list< memory_buffer > buffers;
	std::list< memory_buffer >::iterator bi;
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
			// list of known buffers -- they will be deletede[]
			// later.
			buffers.push_back(memory_buffer(buf.release(),len));

			total_len += len;
		}
	} catch(...) {
		// There can be allocated memory hanging around in buffers...
		for(bi = buffers.begin(); bi != buffers.end(); ++bi){
			delete[] bi->first;
		}

		throw;
	}

	char* final_buffer = new char[total_len];
	memory_buffer res(final_buffer, total_len);

	// Copy the uncompressed data to the resulting output.
	// Free all previously allocated buffers.
	for(bi = buffers.begin(); bi != buffers.end(); ++bi){
		char* buf = bi->first;
		len = bi->second;

		memcpy(final_buffer, buf, len);
		final_buffer += len;
		delete[] buf;
	}

	return res;
}


int main(int argc, char* argv[])
{
	for(int i = 0; i < 100; ++i) {
		memory_buffer dec(0,0);
		try{
			dec = decompres(argv[1]);
		} catch(...) {
			// pass
		}

		delete[] dec.first;
	}

	std::cout << "Done." << std::endl;
	std::string press_enter;
	std::cin >> press_enter;

}


// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
