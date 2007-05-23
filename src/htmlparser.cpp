#include "htmlparser.h"
#include "strmisc.h"

#include <sstream>

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
        name = std::string( start , length);
        // Parsing restart after the end of this rule

        return to_lower(name);
}



filebuf BaseHTMLParser::readAttValue()
{
	/* [25] Eq        ::= S? '=' S?
	 * [10] AttValue  ::= '"' ([^<&"] | Reference)* '"'
	 *		             |  "'" ([^<&'] | Reference)* "'"
	 */

	filebuf previous_start(this->text); // Just in case we need to go back
	filebuf value;
	std::string token;

	this->readSpace();
	if (*text != '=') {
		// plain HTML attribute with no value
		text.current = previous_start.current; // Get back
		value = filebuf(); //value = None
	} else {

		this->consumeToken('='); // Get past the '='
		this->readSpace();

		switch (*text) {
		case '\'':
			token = "'";
			this->consumeToken(token);  // Get past the starting '
			value =  this->readUntilDelimiter(token);
			this->consumeToken(token);    // Get past the ending '
			break;
		case '"':
			token = "\"";
			this->consumeToken(token);    // Get past the starting "
			value =  this->readUntilDelimiter(token);
			this->consumeToken(token);    // Get past the ending "
			break;
		default:
			// Old HTML-style attribute
			value =  this->readUntilDelimiter(WHITESPACE + ">");
		}
	}
	return value;
}

BaseHTMLParser::attr_list_t& BaseHTMLParser::readAttributeList()
{
	std::string name;
	filebuf val;

	// We are reading a new attribute list - clear previously
	// read attibutes
	this->__attrs.clear();

	while (this->readSpace() and !is_in(*text,TAGLIKE_ATTR_LIST_END_CHARS)){
		name = this->readName();
		val = this->readAttValue();
		this->__attrs[name] = val;
	}
	return this->__attrs;
}


bool BaseHTMLParser::tagFollows(filebuf start)
{
	const char* next_pos = start.current + 1;
	const char* end = start.end;
	// After the < there MUST be a character that is not a white-space,
	// not a &, not another < and not a >
	// Besides, there must be at least a third character to form the
	// smallest tag possible
	if ( (next_pos >= end) or is_in(*next_pos, WHITESPACE) or 
			is_in(*next_pos,TAGLIKE_START_INVALID_CHARS) )
	{
		// not a tag, actually
		return false;
	}
	return true;
}

void BaseHTMLParser::readText()
{
	const char* start = text.current;
	int length = 0;

	while ( not text.eof() and (*text != '<' or not this->tagFollows()) ){
		++text;
		++length;
	}
	// We may have reached the end or found a possible tag start
	// in any case text[start:i] has all that matter for us -- nothing
	// more, nothing less.
	this->handleText( filebuf(start,length) );

	// Parsing should re-start at current position - OK
}

void BaseHTMLParser::readTagLike()
{
	filebuf previous_start (this->text);

	// Is this really  a tag-like content?
	if (not this->tagFollows()) {
		// not a tag, actually, just text... 
		// just eat the ">" as text...
		this->handleText( filebuf(this->text.current, 1));
		++text;
		return;
	}

	// Bellow here things start to get weird. We should do as a normal
	// grammar-based parser would...
	// _tagFollows garantees this is a valid reading position
	filebuf next(text);
	++next; // OK, now we are in the next char...

	try{
		if ( isalpha(*next) ) {
			// Start-Tag, section 3.1
			this->readStartTag();
		} else switch(*next) {
		case '/':
			// End-Tag, section 3.1
			this->readEndTag();
			break;
		case '?':
			// Processing Instruction, section 2.6
			this->readProcessingInstructions();
			break;
		case '!':
			// let's hope is a Comment
			//const std::string COMMENT_START = "<!--";
			if ( (text.len() > COMMENT_START_LEN)  && 
			     (std::string(text.current,COMMENT_START_LEN)
			     	== COMMENT_START) ) 
			{
				this->readComment();
			}else{
				// FIXME
				this->readGenericTagConstruction();
			}
			break;
		default:
			this->readGenericTagConstruction();
		}
	} catch (ParserEOFError) {
		// EOF found before we could finish parsing this tag-like
		// content?
		// Just turn everything we had from previous_start to _end into
		// text content.
		this->handleText( previous_start);
	} catch (InvalidCharError) {
		// InvalidCharError found?
		// Go past the offending character
		++text;
		// Just turn everything we had from previous_start to
		// the current position into text content.
		int length = this->text.current - previous_start.current;
		this->handleText( filebuf(previous_start.current,length) );
	}
}

void BaseHTMLParser::readStartTag()
{       
	// [40] STag ::= '<' Name (S  Attribute)* S? '>'
	// [44] EmptyElemTag ::= '<' Name (S  Attribute)* S? '/>'
	std::string name;
	bool empty_element_tag = false;

	this->consumeToken('<'); // go past the '<'
	name = this->readName();
	BaseHTMLParser::attr_list_t& attrs = this->readAttributeList();
	this->readSpace();

	// Is this an empty element tag?
	// readSpace already did a _checkForEOF for us...
	if ( *text == '/') {
		empty_element_tag = true;
		this->consumeToken('/');  // Go past the '/'
	}
	this->consumeToken('>');  // Go past the >

	// Tag read - invoke callback(s)
	this->handleStartTag(name, attrs, empty_element_tag);
	if ( empty_element_tag ) {
		this->handleEndTag(name);
	}

	// we are already past the '>', so there is no need to update _start
	// parsing should restart after the >
}

void BaseHTMLParser::readEndTag()
{
        // [42] ETag ::= '</' Name  S? '>'
	std::string name;

        this->consumeToken("</"); // go past the '</'
        name = this->readName();
        this->readSpace();
        // Mozilla just ignores whatever comes after the 'Name S?' sequence
        // but before a '>' Let's just do the same!
        this->readUntilDelimiter(std::string(">"));
        this->consumeToken('>');  // go past the '>'
        
        this->handleEndTag(name);
        // we are already past the '>', so there is no need to update _start
        // parsing should restart after the >
}

void BaseHTMLParser::readProcessingInstructions()
{        
	// [16] PI ::=  '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
	filebuf content;
	std::string name;

	this->consumeToken("<?"); // go past the '<?'
	this->readSpace(); // It won't hurt reading spaces here...
	name = this->readName();
	BaseHTMLParser::attr_list_t& attrs = this->readAttributeList();
	this->readSpace();
	this->consumeToken("?>") ;
	this->handleProcessingInstruction(name,attrs);
}

void BaseHTMLParser::readComment()
{
	// [15] Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
	filebuf content;

	this->consumeToken(COMMENT_START); // go past the <!--
	content = this->readUntilDelimiterMark("-->");
	// we are already past the '-->' mark. No need to update _start
	this->handleComment(content);
}

void BaseHTMLParser::readGenericTagConstruction()
{
        // See Section 3.1 from XML Specification
        // [40] STag ::= '<' Name (S  Attribute)* S? '>'
	
	filebuf buf;
	std::string contents;

	BaseHTMLParser::attr_list_t empty_list;

        // just get the text between < and >
	this->consumeToken("<"); // go past the '<'
	buf = this->readUntilDelimiter(">");
	this->consumeToken(">"); // go past the '>'

	contents = std::string(buf.current, buf.len());
	
        this->handleStartTag(contents,empty_list,false);

        // parsing should restart after the >
}


/* **********************************************************************
 *			    FORGUIVEFUL HTML PARSERS
 * ********************************************************************** */



static const char* __TROUBLESOME_TAGS[] = {"script", "style", "textarea"};

const std::set<std::string> SloppyHTMLParser::TROUBLESOME_TAGS(
	__TROUBLESOME_TAGS, __TROUBLESOME_TAGS + 3);

bool SloppyHTMLParser::skipTagIfTroublesome(std::string tag_name,
					       bool empty_element_tag)
{
	bool skiped = false;
	
	// We should only get tag_names in lower case, anyway, but is better
	// safe than sorry...
	to_lower(tag_name);

	// Empty tags have no content, so we only check a tag if it is not
	// empty...

	if ( (not empty_element_tag) &&  
		(SloppyHTMLParser::TROUBLESOME_TAGS.count(tag_name) > 0) )
	{
		// A troublesome tag with troublesome content. Skipt it.
		this->readUntilEndTag(tag_name);
		skiped = true;
	}

	return skiped;
}

typedef std::pair<std::string, std::string> StrPair;

static const std::pair<std::string, std::string> __LINK_TAGS[] = {
	StrPair("a"      , "href"),
	StrPair("link"   , "href"),
	StrPair("iframe" , "src"),
	StrPair("frame"  , "src"),
	StrPair("area"   , "href") 
}; 


const std::map<std::string, std::string> LinkExtractor::LINK_TAGS(
	__LINK_TAGS, __LINK_TAGS + 5);

void LinkExtractor::handleStartTag(const std::string& tag_name,
		attr_list_t& attrs, bool empty_element_tag)
{
	this->safeHandleStartTag(tag_name, attrs, empty_element_tag);
	if (this->skipTagIfTroublesome(tag_name,empty_element_tag)) {
		this->handleEndTag(tag_name);
	}
}

void LinkExtractor::safeHandleStartTag(const std::string& tag_name,
		attr_list_t& attrs, bool empty_element_tag)
{
	LinkExtractor::link_tag_map_t::const_iterator _i;
	std::string _attr;

	std::string name = tag_name;
	to_lower(name);
	// Base should be treat separately
	if ((name == "base") and (attrs.find("href") != attrs.end()) ) {
		this->base = attrs["href"].str();
	} else if ( name == "meta" ){
		this->handleMetaTag(tag_name, attrs);
	}
	// Extract Links
	//  - Is this a tag link?
	//  - If so, is the corresponding attribute of this tag present?
	_i = this->LINK_TAGS.find(name);
	if ( (_i != this->LINK_TAGS.end()) and 
	     ( attrs.find( _attr = _i->second ) != attrs.end() ))
	{
		this->links.insert( attrs[_attr].str() );
	}


}

void LinkExtractor::handleMetaTag( const std::string& tag_name,
attr_list_t& attrs)
{
	std::string value;
	std::string content;

	attr_list_t::const_iterator name_attr = attrs.find("name");
	

	if (name_attr != attrs.end()){
		value = name_attr->second.str();
		to_lower(value);
		if (value == "robots"){
			if ( attrs.count("content") ) {
				content = attrs["content"].str();
				handleRobotsMetaContent(content);
			}
		}
	}
}

void LinkExtractor::handleRobotsMetaContent(std::string content)
{
	std::vector<std::string> diretives;
	std::vector<std::string>::iterator dir;

	to_lower(content);
	diretives = split(content,",");
	for(dir = diretives.begin(); dir != diretives.end(); ++dir ) {
		*dir = strip(*dir);

		if	  (*dir == "follow")  { this->follow = true;
		} else if (*dir == "nofollow"){ this->follow = false;
		} else if (*dir == "index")   { this->index = true;
		} else if (*dir == "noindex") { this->index = false;
		} else if (*dir == "all"){
			this->index = true;
			this->follow = true;
		} else if (*dir == "none"){
			this->index = false;
			this->follow = false;
		}
	}

}



// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
