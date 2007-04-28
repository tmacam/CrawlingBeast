#include "parser.h"

#include <sstream>
#include <algorithm>
#include <ctype.h>

/* **********************************************************************
 *			     STRING TRANFORMATIONS
 * ********************************************************************** */


std::string& to_lower(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int(*)(int))tolower);
	return s;
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



/* **********************************************************************
 *				 GENERIC PARSER
 * ********************************************************************** */


void BaseParser::consumeToken(const std::string& token)
{
	std::string::const_iterator c;

	for(c = token.begin(); c != token.end(); c++){
		checkForEOF();
		if ( *c != *text ) {
			std::ostringstream msg;
			msg << "Expected '" << *c << "' but found '" << 
			       *text <<  "'";
			throw InvalidCharError(msg.str());
		}
		++text;
	}
}


filebuf BaseParser::readUntilDelimiter(const std::string& delimiters)
{
	int length = 0;
	const char* data_start = text.current;

	while( (not text.eof()) and not is_in(*text, delimiters) ) {
		++text;
		++length;
	}
        // Parsing restart after the end of this rule

	// remember: data doesn't include the delimiter
        return filebuf(data_start, length);
}
    
filebuf BaseParser::readUntilDelimiterMark(const std::string& mark)
{
	std::string::size_type where = 0;
	const std::string find_bitch(text.current, text.len());
	filebuf data;

        where = find_bitch.find(mark);
        if (where == std::string::npos){
		//Not found?
			std::string msg ("While looking for delimiter mark '");
			msg += mark;
			msg += "'";
			throw ParserEOFError(msg);

        } else {
		// where == length of the text before the mark
		data = filebuf(text.current, where);
		// Parsing restart after the mark
		text += where + mark.size();
	}

        return data;
}

// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
