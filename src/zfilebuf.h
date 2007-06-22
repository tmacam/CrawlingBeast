#ifndef __ZFILEBUF_H
#define __ZFILEBUF_H
/**@file zfilebuf.h
 * @brief Abstractions and utilities to deal with gzip/zip compressed data.
 */

#include <zlib.h>

#include "filebuf.h"
#include "mmapedfile.h"		// For ErrnoSysException
#include "unicodebugger.h"	// For AutoFilebuf

/**Exception for errors dealing with low-level zlib functions.
 *
 * By low-level we man inflateInit2, inflate and such. Those functions
 * are only called by the overloaded version of @c decompress that
 * deals with gzip-content inside a filebuf.
 *
 * @see decompress
 */
struct ZLibException : public std::runtime_error {

	/**Constructor.
	 *
	 * @param msg Optional message.
	 * @param code Optional zlib error code.
	 */
	ZLibException(std::string msg="", int code = 0)
	: std::runtime_error(msg + " : " + getErrorMessage(code))
	{}

	//!Convert zlib error codes into erro messages
	static std::string getErrorMessage(int code = 0);
	
};

/**GZFileWrapper exception.
 *
 * Just used to signal that some operation on a GZFileWrapper failed.
 *
 * @see GZFileWrapperException
 */
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
filebuf decompres(const std::string filename);

/**Decompresses a filebuf holding the contents of a gzip file into a filebuf.
 *
 * We expect the input filebuf (@p data) to contain the contents of a
 * full gzip file with headers included. You can modify it to handle both
 * gzip and zlib data formats. See the implementation.
 *
 * The memory needed to hold the decompressed file is dinamically allocated
 * by this function.
 *
 * Empty filebuf, (i.e,  <code>len() == 0</code>) will not result
 * in errors or invald data, but on an also empyt result filebuf.
 *
 * @warning It is your responsability to release (whith delete[]) the memory
 * allocated by this function.
 *
 * @throw ZLibException if anything went wrong while
 * decompressing the filebuf.
 *
 */
filebuf decompress(filebuf data);


#endif // __ZFILEBUF_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
