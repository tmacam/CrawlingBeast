#include "robotshandler.h"
#include "explode.h"

static char CRLF[] = "\x0D\x0A";

typedef RobotsParser::key_val_t key_val_t;

//! Find which line delimiter is being used: CR, LF or CRLF
void RobotsParser::findLineDelimiter()
{
	// Trying with CRLR
	linedelimiter = std::string(CRLF);
	filebuf previous_text = this->text;
	try{
		readUntilDelimiterMark(linedelimiter);
		// If we didn't get a ParserEOFError, then
		// CRLR is the line linedelimiter
		if (not text.eof()) {
			text = previous_text;
			return;
		}
	} catch(...){
		// Ignore any error..
	}
	// Reset file reading position
	text = previous_text;

	// Trying with CR
	linedelimiter = std::string("\x0D");
	try{
		readUntilDelimiter(linedelimiter);
		// If we didn't get a ParserEOFError, then
		// CR is the line linedelimiter
		checkForEOF();
		if (not text.eof()) {
			text = previous_text;
			return;
		}
	} catch(...){
		// Ignore any error..
	}
	// Reset file reading position
	text = previous_text;

	// Ok, we assume it's LR and that's it
	linedelimiter = std::string("\x0A");
}

//! It ignores comment lines and in-lined coments
std::string RobotsParser::readLine()
{
	std::vector<std::string> takecomments;
	filebuf line_buf;
	std::string line;

	do {
		try {
			line_buf = readUntilDelimiterMark(linedelimiter); 
		} catch (ParserEOFError) {
			// Reached the end of the file and didn't find a 
			// line delimiter.
			// Is there something left to be read?
			checkForEOF();
			line_buf = text.readf(text.len());
		}
		if (line_buf.len() == 0) {
			throw EmptyLineException();
		}
		line = line_buf.str();
		takecomments = split(line,"#");
		line = takecomments[0];
		strip(line);
	} while (not text.eof() and  line.empty());

	return line;
}

RobotsParser::key_val_t RobotsParser::getKeyValue()
{
	std::vector<std::string> content;
	std::string line;
	key_val_t res;

	while(not text.eof()) {
		line = readLine();
		line = strip(line);
		content = split(line,":",1);
		if ( content.size() != 2 ) {
			// WTF?! ignore this line
			continue;
		}
		std::string& field = content[0];
		std::string& value = content[1];

		value = strip(value);
		field = to_lower(field);
		field = strip(field);
		res = key_val_t(field,value);
		break; // OK, get out and return
	}

	return res;

}


bool RobotsParser::parseRecord()
{
	key_val_t kv;
	bool interesting_record = false;
	const char* previous_start = text.current;

	// Read user agent lists
	while(not text.eof()){
		previous_start = text.current;
		kv = getKeyValue();
		std::string& key = kv.first;
		std::string& value = kv.second;
		
		if( key != "user-agent") {
			// Not an user-agent? Perhaps this is a Allow/Disallow
			// section...  Assuming this is the case, then
			// get back to the start of this line
			text.current = previous_start;
			if ( not interesting_record) {
				// Bah! Boring! Ignore the rest
				// until the start of the next record
				readUntilDelimiterMark(linedelimiter+linedelimiter);
				return false;
			} else {
				break;
			}
		} else if (value == "*") {
			interesting_record = true;
		}
		
	}

	// read Allow/Disalow lines
	while(not text.eof()){
		kv = getKeyValue();
		std::string& key = kv.first;
		std::string& value = kv.second;

		strip(value);
		if (value.empty() or value[0] != '/'){
			// Ignore this line
			continue;
		}

		if (key == "allow") {
			rules.push_back(rule_t(value,true));
		} else if (key == "disallow") {
			rules.push_back(rule_t(value,false));
		}
	}

	return interesting_record;
}

void RobotsParser::parse()
{
	std::vector<std::string> content;

	while(not this->text.eof()){
		try {
			parseRecord();
		} catch (EmptyLineException) {
			// Record delimiter found...
			// Or something else.. who cares...
			continue;
		} catch (...) {
			// This is probably a ParserEOFError
			// We are done...
			break;
		}
	};
}
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
