#include "gzstream.h"
#include "filebuf.h"
#include "mmapedfile.h"
#include "unicodebugger.h"

#include <iostream>
#include <list>
#include <memory>

class GZFileWrapperException: public ErrnoSysException {
public:
	GZFileWrapperException (std::string msg="")
                :ErrnoSysException(msg){}
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

/**Decompresses a gzip file into a filebuf.
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


int main(int argc, char* argv[])
{
	for(int i = 0; i < 100; ++i) {
		try{
			AutoFilebuf dec(decompres(argv[1]));
		} catch(...) {
			// pass
		}
	}

	std::cout << "Done." << std::endl;
	std::string press_enter;
	std::cin >> press_enter;

}


// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
