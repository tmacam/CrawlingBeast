#ifndef __HTMLPARSER_H
#define __HTMLPARSER_H
/**@file htmlparser.h
 * @brief HTML parser
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
#include "parser.h"

#include <map>
#include <set>

#include "filebuf.h"

/* **********************************************************************
 *      			    SYMBOLS
 * ********************************************************************** */

/**@name Symbols and Commom String Constants
 *
 * This constants have the same meaning as the corresponding rules of the
 * XML Specification, Section 2.3 Common Syntatic Constructs
 */
//@{

const std::string NAME_START_CHARS = LETTERS + "_:";
const std::string NAME_CHARS = NAME_START_CHARS + ".-" + DIGITS;

//!Things that should not be in the start of a tag-like construct
const std::string TAGLIKE_START_INVALID_CHARS = "<&>";

//!Finding any of these chars while reading an Attribute List means we've
//!found it's end.
const std::string TAGLIKE_ATTR_LIST_END_CHARS = "?/>";

//@}

//!@name Extra String Constants
//@{
const std::string COMMENT_START = "<!--";
const unsigned int COMMENT_START_LEN = 4;
//@}
 




/* **********************************************************************
 *                               HTML PARSER 
 * ********************************************************************** */

/**Abstract base class for HTML parsers*/
struct AbstractHTMLParser {
	typedef std::map< std::string, filebuf> attr_list_t;

	virtual void handleText(filebuf text) = 0;

	virtual void handleStartTag(const std::string& tag_name,
			attr_list_t& attrs, bool empty_element_tag=false) = 0;

	virtual void handleEndTag(const std::string& tag_name) = 0;

	virtual void handleProcessingInstruction(const std::string& name,
			attr_list_t& attrs) = 0;

	virtual void handleComment(const filebuf& comment) = 0;

	virtual ~AbstractHTMLParser(){}
};

/**A simple, almost stupid non-validating (x)HTML push parser.
 *
 * WWW is a jungle of wild and bad-formed and invalid HTML documents.
 * Besides all the beasts you will find trying to parse a real page, you
 * still has to deal with documents that pretend to be X(H)ML, documents
 * that mixes XML, HTML and old-style markups. This parser tries to 
 * handle all those sort of documents and survive - intact.
 *
 * Most errors are handled graciously: invalid tag, characters outside
 * the allowed ranges, things that seem like valid tags but ain't, all
 * those sort of errors are handled in a uniform way: promoting
 * wanna-be-tag-content to text.
 *
 * @note Errors found during tag processing "promote" that thought-to-be-tag
 *	 content into text content.
 */
class BaseHTMLParser: public AbstractHTMLParser, BaseParser {
protected:

	//! Just to avoid copying Maps 
	attr_list_t __attrs;

	/**@name Tokenizer methods.
	 *
	 * Commom Syntatic Constructs Rules and some extensions.
	 * See XML Ref., Sec. 2.3.
	 */
	//@{

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

        /**Reads a 'name', almost according to the XML specification.
	 *
	 * Parsing restart after the end of this rule
	 *
	 * @return The lowercase string of the name found in the current
	 * position.
	 */
	std::string readName();

	/**Reads a AttValue rule and a possible preceding Eq rule.
	 *
	 * Observe that we deal here with HTML-styled attributes, these
	 * are all exemples of valid atributes:
	 * -# attribute="value"
	 * -# attribute='value'
	 * -# attribute=value
	 * -# atribute  (Yes, without a value...)
	 *
	 * Examples 3 and 4 are not valid XML but ARE valid HTML.
	 */
	filebuf readAttValue();


        /**Reads a (S Attribute)* rule */
	attr_list_t& readAttributeList();

	/**Is the next thing in 'start' a tag?
	 *
	 * @param start Position in text where we should test if there is a
	 *		tag-like content starting.
	 */
	bool tagFollows(filebuf start);

	/**Is the next thing in the input buffer a tag?  */
	bool tagFollows() { return this->tagFollows(filebuf(text));}
	//@}

	//!@name Rule processing methods
	//! Element (Tags, PI, Data) Constructs ( XML Ref., Sec 2.4+) Rules
	//@{

	/**Reads text until the start of something that *seems* like a tag.
	 *
	 * @todo in the future, this should handle PCDATA, IIRC, it means it
	 *       should handle entities and such. Ignore it for BaseHTMLParser
	 */
	void readText();

	/**Read and parse a tag-like content.
	 *
	 * If it starts with a <, and is not followed by space or & characters
	 * then it is assuped to be a tag-like content.
	 *
	 * Errors during tag-like-content decoding "promote" the tag-like to
	 * text content.
	 */
	void readTagLike();

	/**Reads a Start-Tag construction.
	 *
	 * Parsing restarts after the tag's closing ">".
	 * See Section 3.1 from XML Specification
	 */
	void readStartTag();

	/**Reads an End-Tag construction.  
	 *
	 * Parsing restarts after the tag's closing ">".
	 */
	void readEndTag();


        /**Reads a Processing Instruction construction.
	 *
	 * Parsing restarts after the tag's closing ">".
	 */
	void readProcessingInstructions();

        /**Reads a Comment.
	 *
	 * Parsing restarts after the tag's closing ">".
	 */
	void readComment();

        /**Reads a generic construction.
	 *
	 * Parsing restarts after the tag's closing ">".
	 */
	void readGenericTagConstruction();
	//@}
public:
	typedef AbstractHTMLParser::attr_list_t attr_list_t;

	virtual void parse();
	virtual void handleText(filebuf text){}

	virtual void handleStartTag(const std::string& tag_name,
			attr_list_t& attrs, bool empty_element_tag=false){}

	virtual void handleEndTag(const std::string& tag_name){}

	virtual void handleProcessingInstruction(const std::string& name,
			attr_list_t& attrs){}

	virtual void handleComment(const filebuf& comment){}

	BaseHTMLParser(const filebuf& text): BaseParser(text), __attrs() {}
};




            

/* **********************************************************************
 *			    FORGUIVEFUL HTML PARSERS
 * ********************************************************************** */


/**Our first try into a HTML parser that doesn't choke on tags like style
 * and script.
 *
 * Certain HTML tags, like STYLE, SCRIPT, and TEXTAREA, can contain data
 * that is not expected to validates as valid HTML. Parsing this data will
 * probably be problesome and such be avoided - and that's exactly what
 * this parser does. As soon as such a tags is found it forwards text parsing
 * to the end of corresponding tag, avoding all the mess what it can hold
 * inside.
 */
class SloppyHTMLParser: public BaseHTMLParser {
protected:
	static const std::set<std::string> TROUBLESOME_TAGS;

public:
   /**Skip content inside a tag if it is a troublesome one.
    *
    * @warning You should probably call handleEndTag if the the
    *		 tag was indeed skiped.
    * @return content inside the Tag, None if the Tag is not a
    * troublesome one.
    */
    bool skipTagIfTroublesome(std::string tag_name, bool empty_element_tag);

    SloppyHTMLParser(const filebuf& text): BaseHTMLParser(text) {}

};

/* **********************************************************************
 
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

#endif // __HTMLPARSER_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
