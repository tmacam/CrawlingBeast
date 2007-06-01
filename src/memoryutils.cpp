// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "memoryutils.hpp"

#include <iostream>
#include <string>

#include <unistd.h>
#include <sys/types.h>

#include <sstream>
#include <fstream>

int getMemoryUsage(pid_t pid = 0)
{
	unsigned long int page_count;
	std::ostringstream statm_filename;

	if (pid == 0){
		pid = getpid();
	}

	statm_filename << "/proc/" << pid << "/statm";


	std::ifstream statm(statm_filename.str().c_str());
	if( statm >> page_count ) {
		return (page_count * getpagesize()) >> 10;
	} else {
		return -1;
	}

}


//EOF
