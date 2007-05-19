#ifndef __ZFILEBUF_H
#define __ZFILEBUF_H
/**@file zfilebuf.h
 * @brief Abstractions and utilities to deal with gzip/zip compressed data.
 */

#include <zlib.h>

#include "filebuf.h"
#include "mmapedfile.h"		// For ErrnoSysException
#include "unicodebugger.h"	// For AutoFilebuf

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



#endif // __ZFILEBUF_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
