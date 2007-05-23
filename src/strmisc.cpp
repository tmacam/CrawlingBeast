// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "strmisc.h"

#include "parser.h" // FIXME this dependency should be removed
#include <algorithm>
#include <ctype.h>

/* **********************************************************************
 *			     STRING TRANFORMATIONS
 *			  AND PYTHON COMPAT. FUNCTIONS
 * ********************************************************************** */

std::string& to_lower(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int(*)(int))tolower);
	return s;
}

std::string& to_upper(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int(*)(int))toupper);
	return s;
}


bool startswith(const filebuf& u, const std::string& start )
{
	filebuf data(u);

	// Sanity check
	if (start.size() > data.len()) { return false; }

	return std::equal(start.begin(), start.end(),data.current);
	
}

bool startswith(const std::string& data, const std::string& start )
{
	// Sanity check
	if (start.size() > data.size()) { return false; }

	return std::equal(start.begin(), start.end(),data.begin());
	
}

bool endswith(const std::string& u, const std::string& end )
{
	// Sanity check
	if (end.size() > u.size()) { return false; }

	return std::equal(end.rbegin(), end.rend(),u.rbegin());
	
}


std::string strip(const std::string _s)
{
	std::string::size_type start;
	std::string::size_type end;

	std::string s = _s; // STFU!

	// take spaces out of the start...
	start = s.find_first_not_of(WHITESPACE);
	if (start == std::string::npos) {
		// String made of spaces...
		s.clear();
		return s;
	}
	s = s.substr(start);

	// take space out of the end of the string
	end = s.find_last_not_of(WHITESPACE);
	if (end == std::string::npos) {
		// Segurança morreu de seguro...
		// WE SHOULD NOT BE HERE but...
		// String made of spaces...
		s.clear();
		return s;
	}
	s = s.substr(0,end +1);
	return s;
}

filebuf& lstrip(filebuf& s)
{
	while( !s.eof() && is_in(*s, WHITESPACE) ){
		++s;
	}

	return s;
}


/* ********************************************************************** *
			SPLIT, EXPLODE, JOIN AND FRIENDS
 * ********************************************************************** */

std::vector<std::string>
split (const std::string &inString, const std::string &separator,
		unsigned int maxsplit)
{
	std::vector<std::string> returnVector;
	std::string::size_type start = 0;
	std::string::size_type end = 0;
	unsigned int count = maxsplit;

	while ( ((end=inString.find (separator, start)) != std::string::npos) &&
			(maxsplit ? count-- : true) )
	{
		returnVector.push_back (inString.substr (start, end-start));
		start = end+separator.size();
	}

	returnVector.push_back (inString.substr (start));

	return returnVector;

} 



