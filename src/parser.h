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
 *  FIXME missing: <!DOCBOOK, <![CDATA ]]>
 *
 */


#include "common.h"

#include <string>
#include <stdexcept>
#include <map>
#include <algorithm>
#include <string.h>

#include "filebuf.h"

/* **********************************************************************
 *			     STRING TRANFORMATIONS
 * ********************************************************************** */

/**In place*/
std::string& to_lower(std::string& s);


inline bool is_in(unsigned char c, const std::string& where)
{
        return where.find(c) != std::string::npos;
}


/* **********************************************************************
 *      			    SYMBOLS
 * ********************************************************************** */

/**@name Symbols and Commom String Constants
 *
 * This constants have the same meaning as the corresponding rules of the
 * XML Specification, Section 2.3 Common Syntatic Constructs
 */
//@{

const std::string LETTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const std::string WHITESPACE = " \t\n\x0b\x0c\r";
const std::string DIGITS = "0123456789";

const std::string NAME_START_CHARS = LETTERS + "_:";
const std::string NAME_CHARS = NAME_START_CHARS + ".-" + DIGITS;

//!Things that should not be in the start of a tag-like construct
const std::string TAGLIKE_START_INVALID_CHARS = "<&>";

//@}


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
	ParserEOFError(const std::string& __arg):
		ParsingError(__arg){}
};


/** A parser just has to know how to parse a text, right? */
struct AbstractBaseParser {
	virtual void parse() = 0;
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
	 */
	void consumeToken(const std::string& token);

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
	 */
	filebuf readUntilDelimiter(const std::string& delimiters);

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
	BaseParser(filebuf& text) : text(text) {};
	virtual void parse() = 0;
};


/* **********************************************************************
 *                               HTML PARSER 
 * ********************************************************************** */

/**Abstract base class for HTML parsers*/
struct AbstractHTMLParser {
	typedef std::map<std::string, std::string> attr_list_t;

	virtual void handleText(filebuf text) = 0;

	virtual void handleStartTag(const std::string& tag_name,
			attr_list_t& attrs, bool empty_element_tag=false) = 0;

	virtual void handleEndTag(const std::string& tag_name) = 0;

	virtual void handleProcessingInstruction(const std::string& name,
			attr_list_t& attrs) = 0;

	virtual void handleComment(const std::string& comment) = 0;
};

/**A simple, almost stupid non-validating (x)HTML push parser.
 *
 *
 * @note Errors found during tag processing "promote" that thought-to-be-tag
 *	 content into text content.
 */
class BaseHTMLParser: public AbstractHTMLParser, BaseParser {
protected:

	//! Just to avoid copying Maps 
	attr_list_t __attrs;

	/**@name Tokenizer methods
	 * Commom Syntatic Constructs (S 2.3) Rules
	 */
	//@{
	//bool _tagFollows(start=None);

        /**Reads a 'name', almost according to the XML specification.
	 *
	 * Parsing restart after the end of this rule
	 */
	std::string readName();

	/**Reads a 'S*' rule, as close as possible to the XML specification.
	 *
	 * Returns True if any space was read/consumed. After calling this
	 * function the current reading position should be on a non-space
	 * character.
	 *
	 * @warning This function calls checkForEOF
	 * @throw ParserEOFError
	 */
	bool readSpace(bool optional=true);

	/**Returns whatever exists until a end-tag w/ name tag_name is found.
	 *
	 * After callig this function the reading position is located one
	 * character AFTER the EndTag construct.
	 *
	 * Notice:
	 * - Tag matching is case insensitive.
	 * - You will lose an EndTag event for this tag.
	 */
	void readUntilEndTag(const std::string& tag_name);


	std::string& readAttValue();
	attr_list_t& readAttributeList();
	//@}

	//!@name Rule processing methods
	//@{
	void readText();
	void readTagLike();
	void readStartTag();
	void readEndTag();
	void readProcessingInstructions();
	void readComment();
	void readGenericTagConstruction();
	//@}
public:
	typedef AbstractHTMLParser::attr_list_t attr_list_t;

	virtual void parse();
};




            
/* **********************************************************************

std::string& BaseHTMLParser::readAttValue()
{
    def readAttValue(self):
        """Reads a AttValue rule and a possible preceding Eq rule.
        
        Observe that we deal here with HTML-styled attributes, so """
        # [25] Eq	    ::= S? '=' S?
        # [10] AttValue	::= '"' ([^<&"] | Reference)* '"'
		#	             |  "'" ([^<&'] | Reference)* "'"
        previous_start = self._start # Just in case we need to go back
        value = None
        self._readSpace()
        if self._text[self._start] != u'=':
            # plain HTML attribute with no value
            self._start = previous_start # Get back
            value = None
        else:
            self._consumeToken('=') # Get past the '='
            self._readSpace()
            if self._text[self._start] == u"'":
                token = u"'"
                self._consumeToken(token)  # Get past the first '
                value =  self._readUntilDelimiter(token)
                self._consumeToken(token)    # Get past the end '
            elif self._text[self._start] == u'"':
                token = u'"'
                self._consumeToken(token)    # Get past the first "
                value =  self._readUntilDelimiter(token)
                self._consumeToken(token)    # Get past the end "
            else:
                # Old HTML-style attribute
                value =  self._readUntilDelimiter(WHITESPACE + u">")
        return value
}

    def _readAttributeList(self):
        """Reas a (S Attribute)* rule"""
        attrs = {}
        while self._readSpace() and self._text[self._start] not in u'?/>':
            name = self._readName()
            val = self._readAttValue()
            attrs[name] = val
        return attrs


    # Element (Tags, PI, Data) Constructs (S 2.4+) Rules #######################
    
    def _readText(self):
        """Reads text until the start of something that *seems* like a tag."""
        # XXX   in the future, this should handle PCDATA, IIRC, it means it
        #       should handle entities and such. Ignore it for BaseHTMLParser
        i = self._start
        while i <= self._end and \
            (self._text[i] != '<' or not self._tagFollows(i)):
                i = i + 1
        # We may have reached the end or found a possible tag start
        # in any case text[start:i] has all that matter for us -- nothing
        # more, nothing less.
        self.handleText( self._text[self._start : i] )
         
        # Parsing should re-start at current position
        self._start = i

    def _tagFollows(self,start=None):
        """Is the next thing to be read a tag?
        
        @param start Position in text where we should test if there is a
                     tag-like content starting.
        """
        if not start:
            start = self._start
        next_pos = start + 1
        end = self._end
        data = self._text
        # After the < there MUST be a character that is not a white-space,
        # not a &, not another < and not a >
        # Besides, there must be at least a third character to form the
        # smallest tag possible
        if next_pos > end or data[next_pos].isspace() or \
           data[next_pos] in TAGLIKE_START_INVALID_CHARS:
                # not a tag, actually
                return False
        return True

    def _readTagLike(self):
        """Read and parse a tag-like content.
        
        If it starts with a <, and is not followed by space or & characters
        then it is assuped to be a tag-like content.

        Errors during tag-like-content decoding "promote" the tag-like to 
        text content.
        """
        previous_start = self._start
        next_pos = self._start + 1
        next = None

        # Is this really  a tag-like content?
        if not self._tagFollows():
            # not a tag, actually, just text... 
            self.handleText( self._text[ self._start : self._start +1])
            self._start += 1
            return

        # Bellow here things start to get weird. We should do as a normal
        # grammar-based parser would...
        # _tagFollows garantees this is a valid reading position
        next = self._text[next_pos]
        
        try:
            if next.isalpha():
                # Start-Tag, section 3.1
                self._readStartTag()
            elif next == u'/':
                # End-Tag, section 3.1
                self._readEndTag()
            elif next == u'?':
                # Processing Instruction, section 2.6
                self._readProcessingInstructions()
            elif next == u'!':
                # let's hope is a Comment
                if self._text[next_pos:].startswith(u"!--"):
                    self._readComment()
                else:
                    # FIXME
                    self._readGenericTagConstruction()
            else:
                self._readGenericTagConstruction()
        except ParserEOFError:
            # EOF found before we could finish parsing this tag-like content?
            # Just turn everything we had from previous_start to _end into
            # text content.
            self.handleText( self._text[ previous_start : self._end +1])
        except InvalidCharError:
            # InvalidCharError found?
            # Just turn everything we had from previous_start to _end into
            # text content.
            self.handleText( self._text[ previous_start : self._end +1])
            # Go past the offending character
            self._start += 1
        

    def _readStartTag(self):
        """Reads a Start-Tag construction.
        
        Parsing restarts after the tag's closing ">"."""
        # See Section 3.1 from XML Specification
        # [40] STag ::= '<' Name (S  Attribute)* S? '>'
        # [44] EmptyElemTag ::= '<' Name (S  Attribute)* S? '/>'
        name = None
        attrs = {}
        empty_element_tag = False

        self._consumeToken('<') # go past the '<'
        name = self._readName()
        attrs = self._readAttributeList()
        self._readSpace()

        # Is this an empty element tag?
        i = self._start # _readSpace already did a _checkForEOF for us...
        if self._text[i] == '/':
            empty_element_tag = True
            self._consumeToken('/')  # Go past the '/'
        self._consumeToken('>')  # Go past the >

        self.handleStartTag(name, attrs, empty_element_tag)
        if empty_element_tag:
            self.handleEndTag(name)

        # we are already past the '>', so there is no need to update _start
        # parsing should restart after the >

    
    def _readEndTag(self):
        """Reads an End-Tag construction.
        
        Parsing restarts after the tag's closing ">"."""
        # [42] ETag ::= '</' Name  S? '>'
        self._consumeToken('</') # go past the '</'
        name = self._readName()
        self._readSpace()
        # Mozilla just ignores whatever comes after the 'Name S?' sequence
        # but before a '>' Let's just do the same!
        self._readUntilDelimiter(">")
        self._consumeToken('>')  # go past the '>'
        
        self.handleEndTag(name)
        # we are already past the '>', so there is no need to update _start
        # parsing should restart after the >

    def _readProcessingInstructions(self):
        """Reads a Processing Instruction construction.
        
        Parsing restarts after the tag's closing ">"."""
        # [16] PI ::=  '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
        content = None
        self._consumeToken('<?') # go past the '<?'
        #content = self._readUntilDelimiterMark("?>")
        # we are already past the '?>' mark. No need to update _start
        #self.handleProcessingInstruction(content)

        self._readSpace()
        name = self._readName()
        attrs = self._readAttributeList()
        self._readSpace()
        self._consumeToken('?>') 
        self.handleProcessingInstruction(name,attrs)

    def _readComment(self):
        """Reads a Comment.
        
        Parsing restarts after the tag's closing ">"."""
        # [15] Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
        content = None
        self._consumeToken('<!--') # go past the '<'
        content = self._readUntilDelimiterMark("-->")
        # we are already past the '-->' mark. No need to update _start
        self.handleComment(content)


    def _readGenericTagConstruction(self):
        """Reads a generic construction.
        
        Parsing restarts after the tag's closing ">"."""
        # See Section 3.1 from XML Specification
        # [40] STag ::= '<' Name (S  Attribute)* S? '>'
        i = self._start + 1 # go past the '>'
        while i <= self._end and self._text[i] != '>':
            i = i + 1

        # just get the text between < and >
        self.handleStartTag( self._text[ self._start + 1: i],{},False)

        # parsing should restart after the >
        self._start = i + 1


########################################################################
#                     UNIT/DOC TESTS AND DEBUGING CLASSES
########################################################################


class TestParser(BaseHTMLParser):
    """Simple Test class for the parser.

    The aim of this class is to be simple fixure upon which unittests can be
    easly build. For now, doctests are being used.

    >>> TestParser("a c ").parse().items
    [('TEXT', u'a c ')]

    >>> TestParser("a <> c ").parse().items
    [('TEXT', u'a <> c ')]

    >>> TestParser("a<b>c").parse().items
    [('TEXT', u'a'), ('TAG', u'b', {}), ('TEXT', u'c')]

    >>> TestParser("<b>").parse().items
    [('TAG', u'b', {})]

    >>> TestParser("<a href=http://www.uol.com.br/>").parse().items
    [('TAG', u'a', {u'href': u'http://www.uol.com.br/'})]

    >>> TestParser('#<a duplas="x y"'+" simples='what is that' html=antigo attrhtml />#").parse().items == [('TEXT', u'#'), ('TAG', u'a', {u'duplas': u'x y', u'simples': u'what is that', u'html': u'antigo', u'attrhtml': None }), ('ENDTAG', u'a'), ('TEXT', u'#')]
    True

    >>> TestParser("a<b style=").parse().items
    [('TEXT', u'a'), ('TEXT', u'<b style=')]
    
    >>> TestParser("a<b style=b ").parse().items
    [('TEXT', u'a'), ('TEXT', u'<b style=b ')]

    # EndTag 
    #
    # Mozilla just ignores whatever comes after the 'Name S?' sequence
    # but before a '>' Let's just do the same!
    >>> TestParser("a<b style='>'></b 1 asd asd asd>").parse().items
    [('TEXT', u'a'), ('TAG', u'b', {u'style': u'>'}), ('ENDTAG', u'b')]

    
    >>> TestParser("a <? xml blah ?><!-- <b> --> c").parse().items
    [('TEXT', u'a '), ('PI', u'xml', {u'blah': None}), ('COMMENT', u' <b> '), ('TEXT', u' c')]

    >>> TestParser("a<b style=b 1").parse().items
    [('TEXT', u'a'), ('TEXT', u'<b style=b 1')]
    """

    #>>> TestParser('#<a&whatever duplas="x"'+" simples='what' html=antigo attrhtml />#").parse().items
    #[('TEXT', u'#'), ('TAG', u'a&whatever', {u'duplas': u'x', u'simples': u'what', u'html': u'antigo', u'attrhtml': None }), ('TEXT', u'#')]
    def __init__(self,text):
        # setup base class
        super(TestParser,self).__init__(text)
        self.items = []

    def parse(self):
        super(TestParser,self).parse()
        return self
        
    def handleText(self,text):
        self.items.append(("TEXT",text))

    def handleStartTag(self, tag_name, attrs={}, empty_element_tag=False):
        self.items.append(("TAG",tag_name,attrs))

    def handleEndTag(self,tag_name):
        self.items.append(("ENDTAG",tag_name))

    def handleProcessingInstruction(self,name,attrs):
        self.items.append(("PI",name,attrs))

    def handleComment(self,comment):
        self.items.append(("COMMENT",comment))


########################################################################
#                            HTML FORGUIVEFUL PARSING
########################################################################


class SloppyHtmlParser(BaseHTMLParser):
    """Our first try into a HTML parser that treats style and script
    correctly.
    """
    TROUBLESOME_TAGS = [u'script', u'style']

    def skipTagIfTroublesome(self, tag_name, empty_element_tag):
        """Skip content inside a tag if it is a troublesome one.

        @warning You should probably call handleEndTag if the the
                 tag was indeed skiped.
        
        @return content inside the Tag, None if the Tag is not a
                troublesome one.
        """
        content = None
        if tag_name.lower() in self.TROUBLESOME_TAGS  and not empty_element_tag:
            # A troublesome tag with troublesome content. Skipt it.
            content = self._readUntilEndTag(tag_name)
        return content

class LogParser(SloppyHtmlParser):

    def handleText(self,text):
        print self._start, "TEXT",text

    def handleStartTag(self, tag_name, attrs={}, empty_element_tag=False):
        print self._start, "TAG",tag_name,attrs

        if self.skipTagIfTroublesome(tag_name, empty_element_tag):
            self.handleEndTag(tag_name)

    def handleEndTag(self,tag_name):
        print self._start, "ENDTAG", tag_name

    def handleProcessingInstruction(self,name,attrs):
        print self._start, "PI",text,attrs

    def handleComment(self,comment):
        print self._start, "COMMENT",comment


class LinkExtractor (SloppyHtmlParser):
    """Simple link extractor.

    It parses a HTML page and extracts it's links and some useful
    meta-information from it.

    Atributes:

     links: set of links found on the page, as string and unparsed.
     base:  head/meta/base/[@href] contents, or none if non-existent
     follow: should this page be followed? Defaults to True and is
            modified according to the contents of a meta robots tag.
     index: should this page be index? Defaults to True and is
            modified according to the contents of a meta robots tag.


    """
    # FIXME metainformation outside the page's head should be ignored
    LINK_TAGS = { u'a'      : u'href',
                  u'link'   : u'href',
                  u'iframe' : u'src',
                  u'frame'  : u'src',
                  u'area'   : u'href',
                  }

    def __init__(self,text):
        # setup base class
        super(LinkExtractor,self).__init__(text)
        self.links = set()
        self.base = None
        self.index = True
        self.follow = True

    def handleStartTag(self, tag_name, attrs={}, empty_element_tag=False):
        self.safeHandleStartTag(tag_name, attrs, empty_element_tag)
        if self.skipTagIfTroublesome(tag_name, empty_element_tag):
            self.handleEndTag(tag_name)

    def safeHandleStartTag(self, tag_name, attrs={}, empty_element_tag=False):
        name = tag_name.lower()
        # Base should be treat separately
        if name == 'base' and u'href' in attrs:
            self.base = attrs[u'href']
        elif name == 'meta':
            self.handleMetaTag(tag_name, attrs)
        # Extract Links
        if name  in self.LINK_TAGS and self.LINK_TAGS[name] in attrs:
            self.links.add( attrs[ self.LINK_TAGS[name] ])

    def handleMetaTag(self, name, attrs):
        """Handles meta tag with ROBOTS.txt information"""
        name = name.lower()
        if attrs.get('name','').lower() == 'robots':
            content = attrs.get('content','').lower()
            if 'nofollow' in content:
                self.follow = False
            if 'noindex' in content:
                self.index = False


def _test():
    import doctest
    doctest.testmod()


if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1 and sys.argv[1] != '-v':
        filename = sys.argv[1]
        data=unicode(open(filename,'r').read(),'latin1')
        p = LinkExtractor(data)
        p.parse()
        print "File:", filename
        if p.base:
            print "\t BASE URL", p.base
        for i in p.links:
            print "\t", i
    else:
        _test()

        data = u"asdajsdh\tAsasd.ASdasd < ajk>dajkaXXX<dh title=''>XXX \x00\xa4 "
        parser = TestParser(data)
        parser.parse()
        print "\n\nOriginal stream:", data
        for content in parser.items:
            print content

#        data=unicode(open('/tmp/uol.html','r').read(),'latin1')
#        p=LogParser(data)
#        p.parse()



*/

#endif // __PARSER_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
