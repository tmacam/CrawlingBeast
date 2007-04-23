#include "parser.h"

#include <sstream>

/* **********************************************************************
 *			     STRING TRANFORMATIONS
 * ********************************************************************** */


std::string& to_lower(std::string& s)
{
	std::transform(s.begin(), s.end(), s.begin(), (int(*)(int))tolower);
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
        return filebuf(data_start, length - 1);
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


/* **********************************************************************
 *                               HTML PARSER 
 * ********************************************************************** */

void BaseHTMLParser::parse()
{
	char c = 0;

	while (not this->text.eof()){
            c = *text;
            if (c == '<'){
                this->readTagLike();
            } else {
                this->readText();
	    }
	}
}

std::string BaseHTMLParser::readName()
{
        const char* start = text.current;
	int length = 0;
	std::string name;

	// First character must be in NAME_START_CHARS
	if (not is_in(*text, NAME_START_CHARS)){
		throw InvalidCharError("Error parsing 'Name' rule.");
	}
	++text; // go to next char in name
	++length;
        // Find the end of this name
        while ( (not text.eof()) && is_in(*text, NAME_CHARS) ){
                ++text;
		++length;
	}
        name = std::string( start , length  -1);
        // Parsing restart after the end of this rule

        return to_lower(name);
}


bool BaseHTMLParser::readSpace(bool optional)
{
        bool found = false;

        if ( (not optional) and (not is_in(*text,WHITESPACE)) ) {
            throw InvalidCharError("Error parsing 'Name' rule.");
	}
        // Find the end of this space
        while ( (not text.eof()) and is_in(*text,WHITESPACE) ) {
                ++text;
                found = true;
	}
        // Parsing restart after the end of this rule
        // in this case, if may be in the same place it started...
        checkForEOF();

        return found;
}

void BaseHTMLParser::readUntilEndTag(const std::string& tag_name)
{
        // [42] ETag ::= '</' Name S? '>'
        bool found = false;
	std::string tmp;
	std::string lower_tagname = tag_name;
	to_lower(lower_tagname);

        while (not found ) {
            readUntilDelimiterMark("</");
	    tmp = std::string(text.current, tag_name.size());
	    if (to_lower(tmp) != lower_tagname) {
		    // This is not the tag we are looking for...
		    // Keep looking...
		    continue;
	    }
	    text += tag_name.size();
	    readSpace();
            if (*text != '>'){
		std::string msg = "While looking for the EndTag for " +\
                                      tag_name;
                throw ParserEOFError(msg);
	    }
            consumeToken(std::string(">"));
            // Yeah! The EndTag was found. We are done
            found = true;
	}
}






// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
