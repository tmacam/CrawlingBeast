#include "parser.h"
#include <sstream>

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
