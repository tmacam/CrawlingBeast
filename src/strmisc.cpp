// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "strmisc.h"

#include "parser.h" // FIXME this dependency should be removed


filebuf& lstrip(filebuf& s)
{
	while( !s.eof() && is_in(*s, WHITESPACE) ){
		++s;
	}

	return s;
}
