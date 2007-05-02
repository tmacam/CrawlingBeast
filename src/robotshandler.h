#ifndef __ROBOTSHANDLER_H
#define __ROBOTSHANDLER_H
/**@file robotshandler.h
 * @brief Routines and classes for handling robots.txt files
 */

#include "parser.h"

#include <vector>
#include <string>
#include <list>

class BogusLineException : public ParsingError {
public:
	BogusLineException (std::string msg="") :
		ParsingError(msg){}
};

class EmptyLineException : public ParsingError {
public:
	EmptyLineException (std::string msg="") :
		ParsingError(msg){}
};



/**A simple robots.txt parser.
 *
 * It only matches the "*" user agent
 *
 * It follows the standatd available at
 * http://www.robotstxt.org/wc/norobots.html
 * and is capable of reading files with lines
 * ending in CR only, with LF, only and with CRLF,
 * just as the specs require.
 *
 * It can also handle "Allow" rules, although they
 * are not part of the old spec, but seems to be
 * supported by a proposed RFC and being in wide use.
 *
 * Errors parsing a robots.txt will not be reported back.
 *
 */
class RobotsParser :  public BaseParser {
public:
	typedef std::pair<std::string, std::string> key_val_t;
        typedef std::pair<std::string, bool> rule_t;
        typedef std::list<rule_t> robots_rules_t;
protected:
	std::string linedelimiter;
	robots_rules_t rules;


	key_val_t getKeyValue();
	std::string readLine();
	void findLineDelimiter();

	//!Returns true if we found a record for the  "*" agent
	bool parseRecord();
public:
	RobotsParser(const filebuf& text)
	: BaseParser(text)
	{ 
		findLineDelimiter();
	}

	void parse();

	robots_rules_t getRules() const{return rules;}
};


#endif // __ROBOTSHANDLER_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
