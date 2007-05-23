// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "entityparser.h"

/* ********************************************************************** *
			       Base Entity Parser
 * ********************************************************************** */

void BaseEntityParser::parsePureText()
{
	filebuf text = readUntilDelimiter("&");
	handlePureText(text);
}

void BaseEntityParser::parseReference()
{
	consumeToken("&");
	// FIXME
	filebuf text = readUntilDelimiter("&");
	handleEntityText(text.str());
}

void BaseEntityParser::parseEntityReference()
{
	// FIXME
}

void BaseEntityParser::parseCharacterReference()
{
	// FIXME
}


unsigned int BaseEntityParser::readNumber(bool hex)
{
	// FIXME
	return 0;
}


void BaseEntityParser::parse()
{
	while(! text.eof()){
		if (*text == '&') {
			parseReference();
		} else {
			parsePureText();
		}
	}
}

// EOF
