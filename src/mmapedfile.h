#ifndef __MMAPEDFILE_H
#define __MMAPEDFILE_H
/**@file mmapedfile.h
 * @brief A simple wraper for MMAP that follows the RAII C++ design.
 *
 * This is a wrapper to simplify the use of mmap'ed files.
 *
 * It follows the Resource Acquisition Is Initialization technique
 * ( Stroustrup, "The C++ Language", Section 14.4 ).
 *
 */

#include <sys/types.h>
#include <stdexcept>
#include <string>

#include "filebuf.h"

#include <sys/mman.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>


//!A exception the uses the current errno value to
//!get it's error message.
class ErrnoSysException : public std::runtime_error {
public:
	static std::string getErrnoMsg()
	{
		const int max_strerror_msg = 256;
		char errno_msg[max_strerror_msg];

		strerror_r(errno, errno_msg, max_strerror_msg);

		return std::string(errno_msg, max_strerror_msg);
	}

	ErrnoSysException(std::string msg="")
		: std::runtime_error(msg + " - " + getErrnoMsg()) {}
};


class MMapedFileException : public ErrnoSysException {
public:
	MMapedFileException(std::string msg="")
		:ErrnoSysException(msg){}
};

/**RIIA-like file resource.
 *
 * This is almost a clone of Stroustrup example (Sec. 14.4 )
 *
 * These are read-only files.
 */
class ManagedFilePtr {
	FILE* fh;
public:
	ManagedFilePtr(const char* filename);
	ManagedFilePtr(FILE* ofh): fh(ofh){}
	~ManagedFilePtr() {fclose(fh);}
	operator FILE* (){ return fh;}

	int getFileno();
	off_t filesize();
};

/**@brief A simple wraper for MMAP that follows the RAII C++ design.
 *
 * We don't handle copy or assignment correctly - but neither does Stroustrup
 * in his example...
 *
 * MMap'ed files are opened just for read!
 *
 */
class MMapedFile {
private:
	ManagedFilePtr file;
	filebuf buf;
public:
	MMapedFile(std::string filename);
	~MMapedFile();
	filebuf getBuf() {return buf;}
};


#endif // __MMAPEDFILE_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
