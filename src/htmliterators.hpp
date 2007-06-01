// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __HTML_ITERATORS_H
#define __HTML_ITERATORS_H
/**@file htmliterators.hpp
 * @brief Utilities to iterate over HTML content
 */


#include "htmlparser.h"
#include "strmisc.h"
#include "entityparser.h"

#include <iterator>
#include <algorithm>
#include <list>
#include <functional>


/***********************************************************************
			      HTMLContentRetriever
 ***********************************************************************/

/**Simple and forgiveful HTML pull parser.
 *
 * Entities are not converted here. Some whitespace is trimmed out of the
 * resulting content. Empty content/text nodes are supressed.
 *
 */
class HTMLContentRetriever: public SloppyHTMLParser {
protected:
	std::list<filebuf> text_contents;
public:
	HTMLContentRetriever(const filebuf& text)
	: SloppyHTMLParser(text), text_contents() {}

	void handleStartTag(const std::string& tag_name,
		attr_list_t& attrs, bool empty_element_tag);

	void safeHandleStartTag(const std::string& tag_name,
		attr_list_t& attrs, bool empty_element_tag);


	/**Parse some of the file content.
	 *
	 * This method is version of BaseHTMLParser::parse modified to
	 * work in a pull-parser fashion.
	 */
	void parse();

	void handleText(filebuf text);

	/**Checks if there is any data left to be pulled.
	 *
	 * @return true if there is NOT any data left to be pulled, false
	 * otherwise.
	 */
	inline bool eof() const { return text_contents.empty() && text.eof(); }

	/**Pull content out of the document.
	 *
	 * This is where our magic get's done.
	 *
	 * Calling this method instruct our parser to do parse a bit
	 * of the original content and, if possible, return it to us.
	 *
	 * It already removes white-space only content.
	 */
	filebuf pull();

};

/***********************************************************************
			      HTMLContentIterator
 ***********************************************************************/

/**Allows one to iterate over all text nodes from a HTML file.
 *
 * Entities are converted in the process.
 *
 */
class HTMLContentIterator
: public std::iterator<std::input_iterator_tag, std::string >
{
private:
	//!Data do be parsed (enveloped in a parser)
	HTMLContentRetriever content;

	//! Last "amount" of text retrieved from the contents
	std::string text;
public:

	HTMLContentIterator(const filebuf cont): content(cont)
	{
		++*this;
	}

	/**Default constructor.
	 *
	 * Use do build end() sentinels
	 */
	HTMLContentIterator(): content(filebuf()) {}

	//! Prefix increment operator
	HTMLContentIterator& operator++()
	{
		text = parseHTMLText( content.pull() );

		return *this;
	}

	//!Postfix increment operator helper class
	class Proxy {
		value_type tmp_val;
	public:
		Proxy(const value_type& val): tmp_val(val) {}
		value_type operator*() {return tmp_val;}
	};

	//!Postfix increment operator
	Proxy operator++(int)
	{
		Proxy d(text);
		++*this;
		return d;
	}

	value_type operator*() const {return text;}
	pointer operator->() {return &(text); }

	/**Comparison operator.
	 *
	 * Actually, it only verifies if this iterator has reached the
	 * end of the data
	 */
	inline bool operator==(const HTMLContentIterator& other)
	{
		return content.eof() && text.empty();
	}

	bool operator!=(const HTMLContentIterator& other)
	{
		return !operator==(other);
	}

};


/***********************************************************************
		       HTMLContentWideCharStreamIterator
 ***********************************************************************/

/**Gives a ilusion of a stream of initerrupted text content.
 *
 * Gives a ilusion that a HTML file has a single text node.
 * A text node is made by text inside and between tags.
 *
 * @deprecated Give poor reference locality and thus poor performance.
 */
class HTMLContentWideCharStreamIterator
: public std::iterator<std::input_iterator_tag, char >
{
private:
	//!Data do be parsed (enveloped in a parser)
	HTMLContentRetriever content;

	//! Last "amount" of text retrieved from the contents
	std::string text;
	std::string::size_type text_pos;

	//! Was the separator between two consecutive text contents issued?
	bool put_sep;

	//! The last character read - if any
	value_type c;
public:

	/**Default constructor */
	HTMLContentWideCharStreamIterator(const filebuf cont)
	: content(cont), text(), text_pos(0), put_sep(false)
	{
		++*this;
	}

	/**Default constructor.
	 *
	 * Use to build end() sentinels.
	 */
	HTMLContentWideCharStreamIterator()
	: content(filebuf()), text(), text_pos(0), put_sep(false)
	{}

	/**Copy Constructor.
	 * 
	 * This copy constructor has some issues but seems to be better
	 * then the C++ automatically generated one.
	 *
	 * Its biggest flawn is that text_pos allways rewinds to text.begin()*/
	HTMLContentWideCharStreamIterator(
		const HTMLContentWideCharStreamIterator& original)
	: content(original.content), text(original.text,original.text_pos),
	  text_pos(0), put_sep(original.put_sep)
	{}
	

	//! Prefix increment operator
	inline HTMLContentWideCharStreamIterator& operator++()
	{		
		// Is there data left in the buffer?
		if (text_pos != text.size()){
			++text_pos;
		} else {
			if ( !content.eof()) {
				if (! put_sep ) {
					text = " ";
					text_pos = 0;
					put_sep = true;
				} else {
					// Separator already issued
					// Get new content
					filebuf cont;
					while(!content.eof() &&
					      (cont = content.pull()).eof() ){ }
					text = parseHTMLText( cont );
					text_pos = 0;
					put_sep = false;
				}
			}
		}

		return *this;
	}

	//!Postfix increment operator helper class
	class Proxy {
		value_type tmp_val;
	public:
		Proxy(const value_type& val): tmp_val(val) {}
		value_type operator*() {return tmp_val;}
	};

	//!Postfix increment operator
	Proxy operator++(int)
	{
		Proxy d( *(*this) );
		++*this;
		return d;
	}

	value_type operator*() const { return text[text_pos]; }
	//inline pointer operator->() {return text_pos; }

	/**Comparison operator.
	 *
	 * Actually, it only verifies if this iterator has reached the
	 * end of the data
	 */
	inline bool operator==(const HTMLContentWideCharStreamIterator& other)
	{
		return content.eof() && (text_pos == text.size());
	}

	inline bool operator!=(const HTMLContentWideCharStreamIterator& other)
	{
		return !operator==(other);
	}

};

#endif // __HTML_ITERATORS_H

// EOF

