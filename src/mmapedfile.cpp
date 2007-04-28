#include "mmapedfile.h"
#include <sys/types.h>
#include <sys/stat.h>

ManagedFilePtr::ManagedFilePtr(const char* filename)
	:fh(0)
{
	// Open the file
	if ( (fh = fopen(filename, "rb")) == NULL ) {
		// Open failed
		 throw MMapedFileException("ManagedFilePtr constructor:");
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

MMapedFile::~MMapedFile()
{
	if (buf.start) {
		munmap((void *)buf.start, buf.len());
	}
}

// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
