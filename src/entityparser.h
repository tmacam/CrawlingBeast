#ifndef __ENTITY_PARSER_H
#define __ENTITY_PARSER_H
/**@file entityparser.h
 * @brief Entity handling and parsing.
 *
 * $Id$
 *
 * We try as hards as possible to mimic mozilla's and konkeror's entity
 * handling behavior.
 *
 * We expect UTF-8 here as well!
 *
 */

#include "parser.h"
#include "strmisc.h"

#include <sstream>

/* **********************************************************************
 *				   EXCEPTIONS
 * ********************************************************************** */

/**An unknown entity reference was found while decoding the text.
 *
 * @see BaseEntityParser
 */
class UnknownEntityReferenceError : public ParsingError {
public:
        UnknownEntityReferenceError(const std::string& __arg):
                ParsingError(__arg){}
};

/* **********************************************************************
			     Abstract Entity Parser
 * ********************************************************************** */

/**Interface for entity parsers.
 *
 * This class defines just the interface/signature that 
 * descendents from BaseEntityParser should follow.
 *
 * In theory, this would make explicit what are the methods you @em SHOULD
 * implement in BaseEntityParser descendents.
 *
 * Observe that the callbacks can be called in any order and there is no
 * implicit text delimiter between calls, i.e., successive or interleaved
 * calls to both handlePureText and handleEntityText can happen even if we
 * are decoding a single (but complete) word.
 */
class AbstractEntityParser {
public:
	/**Callback for text.
	 *
	 * This callback is called always that low-fat, entity-free text is
	 * found. This callback can be called multiple times and the users
	 * should not assume that separate calls means splited text.
	 */
	virtual void handlePureText(const filebuf&) =0;

	/**Callback for decoded entity cotent.
	 *
	 * This callback is called always that an entity was found and
	 * correctly decoded.
	 */
	virtual void handleEntityText(const std::string&) =0;

	virtual ~AbstractEntityParser(){}
};

/* **********************************************************************
			       Base Entity Parser
 * ********************************************************************** */

/**Base class for entity parsers.
 *
 * This class provides a simple infraestructure upon with parsers for HTML
 * text content can be created. HTML text content can have Entity and Character
 * References. Both are commonly called by the generic name "Entity".
 *
 * This entity parser tries to mimic the bahaviour of browsers like firefox
 * and konkeror WRT the handling of xml-invalid entities.
 *
 * This base class provides a simple interface for a push parser upon which
 * more elaborated entity parsers can be built.
 *
 * @note We are assuming UTF-8 encoding in the input and in the output.
 *
 * @see UnknownEntityReferenceError
 */
class BaseEntityParser : public BaseParser, public AbstractEntityParser {
protected:

	void parsePureText();

	void parseReference();
	void parseEntityReference();
	void parseCharacterReference();

	/**Reads a number, in decimal or hexadecimal format, from the stream.
	 * 
	 * @param hex Should it read the number as hexadeximal or as decimal?
	 * 	  Defaults to rading decimal values.
	 * @return The number read.
	 */
	unsigned int readNumber(bool hex=false);

public:
	/**Default constructor.
	 *
	 * @param _url The string representation of the URL.
	 *
	 */
	BaseEntityParser(const std::string text="")
	:BaseParser( filebuf(text.c_str(), text.size()) ) {}

	BaseEntityParser(const filebuf _buf)
	:BaseParser( _buf ) {}

	/**Parses the URL.
	 *
	 * @note This method is @b NOT automatically called by the constructor.
	 */
	void parse();
};



/* **********************************************************************
				 Entity Parser
 * ********************************************************************** */

/**Simple Entity Parser.
 *
 * This a really simple and to the point Entity Parser.
 * This parser automatically calls its parse() method.
 */
class EntityParser : public BaseEntityParser {
	std::ostringstream output;
public:

	EntityParser(const std::string text="")
	:BaseEntityParser( filebuf(text.c_str(), text.size()) ), output()
	{
		parse();
	}

	EntityParser(const filebuf _buf)
	:BaseEntityParser( _buf ), output()
	{
		parse();
	}

	void handlePureText(const filebuf& text)
	{
		output << text.str();
	}

	void handleEntityText(const std::string& entity_text)
	{
		output << entity_text;
	}

	std::string str() { return output.str(); }
};

template <class Tp>
std::string parseHTMLText (Tp input)
{
	EntityParser p(input);

	return p.str();
}

#endif // __ENTITY_PARSER_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
