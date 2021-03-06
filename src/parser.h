#ifndef __PARSER_H
#define __PARSER_H
/**@file parser.h
 * @brief Abstract parser definition
 *
 * Assumptions
 * 
 *  - KISS
 *  - Text is read in a single pass.
 *  - We will try to be as i18n and unicode-aware as possible, but we know that
 *    our aim is latin1 and that most of our content is in west-european
 *    languages.
 *  - For every rule-parsing function: after it's execution, the parser state
 *    should be ready do process the NEXT rule. So, _start should be updated
 *    accordingly
 *
 *
 * Some of the funcionality implemented here was moved to strmisc.h
 * @see xmlconstants
 * 
 */


#include "common.h"

#include <string>
#include <stdexcept>

#include "filebuf.h"
#include "strmisc.h"

/* **********************************************************************
 *				 GENERIC PARSER
 * ********************************************************************** */

/**Something unexpected happened while parsing the text content.
 *
 * This is the root of all parsing errors
 */
class ParsingError : public std::runtime_error {
public:
	ParsingError(const std::string& __arg):
		std::runtime_error(__arg){}
};

/**An error ocurred while parsing a non-terminal rule.
 *
 * Users should not really see this error as any well-formness errors and
 * "unexpected char" errors that ocurr in a tag-like element promote this
 * tag-like into a text data element.
 */
class InvalidCharError : public ParsingError {
public:
	InvalidCharError(const std::string& __arg) :
		ParsingError(__arg){}
};

/** We unexpectedly found the end of the parsing data. */
class ParserEOFError: public ParsingError {
public:
	ParserEOFError(const std::string& __arg =""):
		ParsingError(__arg){}
};


/** A parser just has to know how to parse a text, right? */
struct AbstractBaseParser {
	virtual void parse() = 0;
	virtual ~AbstractBaseParser() {}
};

/** Common infrastructure for building generic recursive descendent parsers. 
 *
 * @warning This still is an abstract class!
 */
class BaseParser : public AbstractBaseParser {
protected:
	/** Text being parsed */
	filebuf text;

	/**@name Tokenizer functions
	 *
	 * This-ought-to-have-a-tokenizer auxiliary functions 
	 */
	//@{
	/**Garantees that the end of the parsed data was not reached.
	 *
	 * @param msg Opitional message to be added to the exception raised
	 * 	      if the end of the parsed text was indeed found.
	 *
	 * @throw ParserEOFError
	 */
	inline void checkForEOF(const std::string& msg="")
	{
		if (text.eof()) {
			throw ParserEOFError(msg);
		}
	}

	/**Verifies that the current reading is valid and equal to token.
	 *
	 * After callig this function the reading position is located one
	 * character after the token's last character.
	 *
	 * EOF-ness is not verified at the end of the function.
	 *
	 * @param token The string you expect to be present in the current
	 * 		reading pos.
	 *
	 * @throw ParserEOFError
	 * @throw InvalidCharError
	 */
	void consumeToken(const std::string& token);

	void consumeToken(char c){ consumeToken(std::string(1,c));}

	/**Advance current reading position in text by one character.
	 *
	 * This function should be used when you must advance the reading
	 * position and read content immediately after. This situation usually
	 * arises during rule processing, where a given token is expected and
	 * consumed and immediately after you should validate the existence
	 * of/check for some other token.
	 *
	 * @throw ParserEOFError if we go past the end of the buffer.
	 * @deprecated
	 */
	inline void advanceReadingPosition() {
		text.read(1);
		checkForEOF();
	}



	/**Returns whatever exists until the one of the delimiters is found.
	 *
	 * After callig this function the reading position is advanced to the
	 * position of the first delimiter found.
	 *
	 * @param delimiters A list with all the delimiters. It can be a single
	 * 		     string (each character is a delimiter) or a a list
	 * 		     with single character strings, each being a
	 * 		     delimiter.
	 *
	 * @return Returns whatever exists between current possition up to, but
	 *         not including, the position where the first delimiter is
	 *         found.
	 *
	 * @warning This method DOESN't raise ParserEOFError - if the delimiter
	 *          is not found until EOF, all text from current position to
	 *          the end of the buffer will be returned.
	 */
	filebuf readUntilDelimiter(const std::string& delimiters);

	/**Returns whatever exists until the one of the delimiters is found.
	 *
	 * Overloaded for a single char
	 */
	filebuf readUntilDelimiter(char delimiter);

	/**Returns whatever exists until the delimiter mark is found.
	 *
	 * After callig this function the reading position is located one
	 * character AFTER last demilimiter mark character. The seach is case
	 * sensitive.
	 *
	 * @param delimiter A string.
	 *
	 * @return the text found until (but not cotaining) the delimiter mark
	 *
	 * @throw ParserEOFError if the delimiter mark is not found
	 */
	filebuf readUntilDelimiterMark(const std::string& mark);
	//@}

public:
	BaseParser() : text() {};
	BaseParser(const filebuf& text) : text(text) {};
	virtual void parse() = 0;
};

#endif // __PARSER_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
