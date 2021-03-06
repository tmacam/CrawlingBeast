#include "mmapedfile.h"
#include <sys/types.h>
#include <sys/stat.h>

ManagedFilePtr::ManagedFilePtr(const char* filename)
	:fh(0)
{
	// Open the file
	if ( (fh = fopen(filename, "rb")) == NULL ) {
		// Open failed
		throw MMapedFileException(std::string("ManagedFilePtr constructor for '") + filename + "'");
	}
}

int ManagedFilePtr::getFileno()
{
	int no = 0;

	// Get it's fileno
	if ( (no = fileno(fh)) == -1 ) {
		// fileno failed
		 throw MMapedFileException("in fileno: ");
	}

	return no;
}

off_t  ManagedFilePtr::filesize()
{
	struct stat stat_buf;

	// Get it's filesize
	if ( fstat(this->getFileno(), &stat_buf ) != 0 ) {
		// stat failed
		 throw MMapedFileException("in stat: ");
	}
	return stat_buf.st_size;
}



MMapedFile::MMapedFile(std::string filename)
	: file(filename.c_str()), buf()
{

	size_t filesize = 0;
	void *mmap_start_pos = 0;

	filesize = file.filesize();
	
	mmap_start_pos = mmap(	0,			// start
				filesize,		// length
				PROT_READ,		// prot
				MAP_PRIVATE,		// flags
				file.getFileno(),	// fd
				0);			// offset
	if (mmap_start_pos == MAP_FAILED) {
		// mmap failed failed
		 throw MMapedFileException("in mmap");
	}

	buf = filebuf((const char*)mmap_start_pos, filesize);
}


MMapedFile::MMapedFile(std::string filename, size_t length, off_t offset)
	: file(filename.c_str()), buf()
{

	void *mmap_start_pos = 0;

	// Deals with "out of range" requests
	off_t filesize = file.filesize();
	if (offset > filesize) {
		// This should probably be an std::out_of_range but...
		throw MMapedFileException("MMapedFileException with offset: "
					  "offset bigger than filesize.");
	} else if (off_t(length + offset) > filesize) {
		length = filesize - offset;
	}

	mmap_start_pos = mmap(	0,			// start
				length,			// length
				PROT_READ,		// prot
				MAP_PRIVATE,		// flags
				file.getFileno(),	// fd
				offset);		// offset
	if (mmap_start_pos == MAP_FAILED) {
		// mmap failed failed
		 throw MMapedFileException("in mmap with offset");
	}

	buf = filebuf((const char*)mmap_start_pos, length);
}


MMapedFile::~MMapedFile()
{
	if (buf.start) {
		munmap((void *)buf.start, buf.len());
	}
}

void MMapedFile::advise(advice_t adv)
{
	if ( madvise((void*)buf.start, buf.len(), adv) ) {
		throw MMapedFileException("mavise(2) failed.");
	}
}

// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
