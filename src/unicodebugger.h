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
#include "iconv.h"

/* ********************************************************************** *
 *				   EXCEPTIONS
 * ********************************************************************** */

/**The name says it all.
 * We gave up.
 */
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


/**Libiconv reported an error.
 *
 * Some call to one of libiconv's functions returned an error.
 * Was it iconv_open, iconv, iconv_close -- does it matter?
 * All that matters is that something went - and now we know it.
 *
 */
class IconvError: public std::runtime_error{
public:
	IconvError(std::string msg="") :
		std::runtime_error(msg) {}
};



/* ********************************************************************** *
		   CHARACTERSET INSTRUCTION DETECTION PARSER
 * ********************************************************************** */


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
            


/* ********************************************************************** *
				LIBICONV WRAPPER
 * ********************************************************************** */

/**Simple and bareboned wraper for libiconv facilities.
 *
 * On errors, IconvError is thrown.
 *
 * De default to-charset encoding is UTF-8.
 */
class IconvWrapper {
private:
	//!This class is non-copyable
	IconvWrapper(const IconvWrapper&);

	//!This class is non-copyable
	IconvWrapper& operator=(const IconvWrapper&);

	iconv_t cd;
public:
	IconvWrapper(const std::string from, const std::string to="UTF-8" )
	{
		std::string _from = from;
		std::string _to = to;

		// All iconv encoding names are uppercased.
		_from = to_upper(_from);
		_to = to_upper(_to);

		cd = iconv_open(_to.c_str(), _from.c_str());
		if (cd == iconv_t(-1) ) {
			throw IconvError(
				"While trying to create a iconv descriptor");
		}
	}

	/**Converts input data using the wrapper's settings.
	 *
	 * @param input The data to pe converted.
	 *
	 * @return The data converted to the "to" encoding.
	 *
	 * @warning It's your responsabiliy to deallocate the memory created
	 * to store the converted data!
	 */
	filebuf convert(const filebuf& input);


	~IconvWrapper() { iconv_close(cd); }
	
};


/* ********************************************************************** *
			     MANAGED FILEBUFER REF
 * ********************************************************************** */

/**A RAII-like wrapper to automatically free the memory pointed by a filebuf.
 *
 * @warning It assumes memory was allocated with new[], so do not make it
 * control a filebuf whose memory was malloc'ed!
 *
 * @warning It is non-copyable!
 */
class AutoFilebuf {
	filebuf f;

	//!This class is non-copyable
	AutoFilebuf(const AutoFilebuf&);
	//!This class is non-copyable
	AutoFilebuf& operator=(const AutoFilebuf&);
public:
	//!This class has default constructor is harmless - I hope.
	AutoFilebuf() : f() {}

	/**Constructs from another filebuf, acquiring its memory.
	 *
	 * After calling this constructor, this AutoFilebuf will take the
	 * responsability of "delete[]"ing the memory pointed by the
	 * suplied filebuf.
	 *
	 */
	AutoFilebuf(const filebuf f): f(f) {};

	~AutoFilebuf(){ if (f.start) delete[] f.start;}

	filebuf getFilebuf() { return f;}

	/**Forcibly deletes the managed object.
	 *  @param  p  new filebuf to manage
	 *
	 *  This object now @e owns the object pointed to by @a p.  The
	 *  previous object has been deleted.
	 */
	void reset(filebuf p)
	{
		if (f.start && p.start != f.start) delete[] f.start;
		f = p;
	}

	/**Deletes the old managed object and copy the
	 * contents of anoher filebuf.
	 *
	 * Calling this function will make this AutoFilebuf deallocate
	 * any memory it was responsable for.
	 *
	 * It will alocate and copy the contents of another filebuf.
	 * It will not take responsability for the memory of the copied
	 * filebuf.
	 */
	void copy(const filebuf original)
	{
		// allocate space for copy
		int length = original.len();
		char* new_data = new char[length];

		//copy data
		memcpy(new_data, original.current,length);

		// Delete old and stablish ownsership
		// of the new filebuf
		reset(filebuf(new_data, length));
	}
};


/* ********************************************************************** *
				 UNICODEBUGGER
 * ********************************************************************** */


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
	filebuf data; //! Our input data
	filebuf converted_data; //!Our output, i.e., input converted do utf-8
	std::list<std::string> suggested_encodings;
	std::set<std::string> tried_encodings;
	std::string encoding;
	char* iconv_buffer;

	bool findEncodingByParsing();
public:
	typedef std::pair<std::string, std::string> enc_pair_t;
	typedef std::list<enc_pair_t> enc_list_t;

	//!@name UTF-X and XML detection patters
	//! Used for BOM/XML charset recognition
	//@{
	//! XML in UTF-X marks (X >= 16)
	static const enc_list_t XML_MARKS;
	//!UTF Byte Order Marks
	static const enc_list_t BOM_MARKS;
	//!ALL_MARKS = XML_MARKS + BOM_MARKS
	static const enc_list_t ALL_MARKS;
	//@}

	/**We read up to MAX_META_POS characters in data to find an indicative
	 * character encoding (In XML header of in a HTML Meta Tag.
	 */
	static const int MAX_META_POS = 1024;

	/**Constructor.
	 *
	 * @param suggested_encodings List of suggested encodings to use, 
	 *  			  in order of precedence.
	 */
	UnicodeBugger(const filebuf& data,
			std::list<std::string> suggested_encodings = std::list<std::string>())
		: data(data), suggested_encodings(suggested_encodings),
		tried_encodings(), encoding(), iconv_buffer(0) {}

	/**Request data conversion o UTF-8.
	 *
	 * @return Data converted to UTF-8 or and empty filebuf.
	 *
	 * @throw CannotFindSuitableEncodingException if, after trying
	 * all possible endocings (found, suggested, guessed), it still cant
	 * covert this shit to unicode.
	 *
	 * @warning It's your responsability to deallocate the data inside
	 * 	    the filebuf!
	 */
	filebuf convert();


	/**Try a converting data from a given encoding.
	 *
	 * If successful, updates converted_data.
	 *
	 * @return true on successful convertions, false otherwise;
	 */
	bool convertFrom(std::string encoding);


	/**Uses XML and Unicode BOM heuristics to find charset.
	 *
	 * @return true if a suitable UTF-x charset was found,
	 * 	   false otherwhise.
	 */
	bool detectUTFEncoding();

	~UnicodeBugger() { delete iconv_buffer;}

	std::string getEncoding() {return this->encoding; }

};

#endif // __UNICODEBUGGER_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
