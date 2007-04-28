#ifndef __UNICODEBUGGER_H
#define __UNICODEBUGGER_H
/**@file unicodebugger.h
 * @brief A simple, almost stupid conversor of webdata to unicode.
 *
 * Just convert to Unicode, you bastard!!
 *
 * This modules define UnicodeBugger, a class that tries to convert
 * whatever it is fed with to unicode - utf-8.
 *
 * it tries to follows the standards and the common knowlege (aka., browser
 * heuritics) as close as possible.
 * This code (and it"s python prototype) are  nspired in UnicodeDammit, from
 * BeautifuSoup.  UnicodeDammit, in turn, is inspired on Mark Pilgrim"s
 * Universal Feed Parser.
 *
 * @author Tiago Alves Macambira
 *
 * $Id$
 *
 * @todo proper unit tests for this module.
 *
 */

#include <string>
#include <list>
#include <set>

#include "filebuf.h"
#include "htmlparser.h"

/* ********************************************************************** *
 *				   CONSTANTS
 * ********************************************************************** */


// Used for BOM/XML charset recognition
//!@name UTF-X and XML detection patters
//@{
//! XML in UTF-X marks (X >= 16)
static const char* XML_MARKS[][2] = {
    {"utf-32LE","\x3c\x00\x00\x00"},
    {"utf-32BE","\x00\x00\x00\x3c"},
    {"utf-16LE","\x3c\x00\x3f\x00"},
    {"utf-16BE","\x00\x3c\x00\x3f"},
};

//!UTF Byte Order Marks
static const char* BOM_MARKS[][2] = { 
    {"utf-8",   "\xef\xbb\xbf"},
    {"utf-32LE","\xff\xfe\x00\x00"},
    {"utf-32BE","\x00\x00\xfe\xff"},
    {"utf-16LE","\xff\xfe"},
    {"utf-16BE","\xfe\xff"},
};
//@}

// FIXME ALL_MARKS = XML_MARKS + BOM_MARKS


/* ********************************************************************** *
 *				   EXCEPTIONS
 * ********************************************************************** */

    


//!The name says it all. We gave up.
class CannotFindSuitableEncodingException: public std::runtime_error{
public:
	CannotFindSuitableEncodingException(std::string msg="") :
		std::runtime_error(msg) {}
};

/**Helper exception to notify that we did find and charset...
 *
 * Not really an error, just used to stop parsing and notify that an
 * indicative encoding was found.
 *
 * @see FindEncParser
 */
class CharsetDetectedException: public std::runtime_error{
public:
	CharsetDetectedException(std::string msg="") :
		std::runtime_error(msg) {}
};

/**Tries to find a document encoding declaration on the document.
 * 
 * - If the document starts with an XML declaration <?xml .... ?>, this
 *   determines encoding by XML rules.
 * -  If the document contains the HTML hack <meta http-equiv="Content-Type"
 *    ...>, any charset declared here is used.
 */
class FindEncParser: public SloppyHTMLParser {
protected:
	std::string enc;
public:

	FindEncParser(const filebuf& text):
		SloppyHTMLParser(text), enc() {}

	void parse();
	
	void handleProcessingInstruction(const std::string& name,
                        attr_list_t& attrs);

	void handleStartTag(const std::string& tag_name,
		attr_list_t& attrs, bool empty_element_tag=false);

	void safeHandleStartTag(const std::string& name,
                attr_list_t& attrs, bool empty_element_tag);
	
	std::string getEnc() {return this->enc;}

	static std::string get_charset_from_content_type(std::string content="");
};
            



/**Tries to convert (x)HTML-like content to UTF-8.
 * 
 * It follows the following heuristics to convert text from
 * unknown encoding into UTF-8, stoping at the first one that succeeds:
 * 
 * - Tries to use any one of the suggested encodings to convert,
 * - Tries to guess the encoding using UTF-X Byte Order Mark (BOM) or
 *   a XML content encoded into UTF-16 or UTF-32.
 * - Tries to guess the encoding using some meta-information availiable on
 *   the document (XML declaration or HTML Meta tag).
 * - Tries to convert it from UTF-8
 * - Tries to convert it from Latin1
 */
class UnicodeBugger {
protected:
	filebuf data;
	std::list<std::string> suggested_encodings;
	std::set<std::string> tried_encodings;
	std::string encoding;
public:
	// We read up to MAX_META_POS characters in data to find an indicative
	// character encoding (In XML header of in a HTML Meta Tag
	static const int MAX_META_POS = 1024;

	/**Constructor.
	 *
	 * @param suggested_encodings List of suggested encodings to use, 
	 *  			  in order of precedence.
	 */
	UnicodeBugger(const filebuf& data,
			std::list<std::string> suggested_encodings = std::list<std::string>())
		: data(data), suggested_encodings(suggested_encodings),
		tried_encodings(), encoding();
	{}

	/**Request data conversion o UTF-8.
	 *
	 * @return Data converted to UTF-8 or and empty filebuf.
	 *
	 * @throw CannotFindSuitableEncodingException if, after trying
	 * all possible endocings (found, suggested, guessed), it still cant
	 * covert this shit to unicode.
	 */
	filebuf convert();	

};
/*

filebuf UnicodeBugger::convert()
{
	// auto_ptr, doubles the mem...
        char *u = NULL
        data = self.data
        if isinstance(data,unicode):
            # No need to do anything
            return data

        # Try the proposed encodings
        for e in self.suggested_encodings:
            u = self._convertFrom(e)
            if u:
                return u
        
        # Ok, try to find the BOM or XML header...
        u = self._detectEncoding()
        if u:
            return u

        # Damn! Does this thing has a XML header or HTML Meta tag w/ encoding?
        max_pos = self.MAX_META_POS
        if len(data) < max_pos:
            max_pos = len(data)
        # latin1 is a good guess anyway, and won"t complain in most cases
        # about bad char. conversions
        e = FindEncParser(unicode(data[:max_pos],"latin1","ignore"))
        try:
            e.parse()
        except Exception:
            pass
        if e.getEnc():
            u = self._convertFrom(e.getEnc())
            if u:
                return u

        # we are out options here! Last options: utf-8 and latin1
        for e in ["utf-8", "latin1"]:
            u = self._convertFrom(e)
            if u:
                return u

        raise CannotFindSuitableEncodingException("Giving up")
}


    def _convertFrom(self,encoding):
        u = None
        # Why bother trying all over again?
        if encoding not in self.tried_encodings:
            try:
                self.tried_encodings.add(encoding)
                u = unicode(self.data,encoding)
                self.encoding = encoding
            except UnicodeDecodeError:
                pass
        return u

    def _detectEncoding(self):
        """Uses XML and Unicode BOM heuristics to find charset."""
        length = len(self.data)
        u = None
        for enc, mark in ALL_MARKS:
            if length > len(mark) and self.data.startswith(mark):
                u = self._convertFrom(enc)
                if u:
                    break
        return u

        


*/


#endif // __UNICODEBUGGER_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
