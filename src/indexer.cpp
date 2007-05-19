// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "zfilebuf.h"
#include "htmlparser.h"
#include "strmisc.h"

#include <iterator>
#include <algorithm>
#include <list>

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


void HTMLContentRetriever::handleStartTag(const std::string& tag_name,
	attr_list_t& attrs, bool empty_element_tag)
{
	this->safeHandleStartTag(tag_name, attrs, empty_element_tag);
	if (this->skipTagIfTroublesome(tag_name,empty_element_tag)) {
		this->handleEndTag(tag_name);
	}
}

void HTMLContentRetriever::safeHandleStartTag(const std::string& tag_name,
		attr_list_t& attrs, bool empty_element_tag)
{
	//FIXME we should retrieve title, alt and other kinds of
	//      attributes here.
}

void HTMLContentRetriever::handleText(filebuf text)
{
	if ( ! lstrip(text).eof()) {
		text_contents.push_back(text);
	}
}

void HTMLContentRetriever::parse()
{
	char c = 0;

	if (not this->eof()){
            c = *text;
            if (c == '<'){
                this->readTagLike();
            } else {
                this->readText();
	    }
	}
}

filebuf HTMLContentRetriever::pull()
{
	filebuf res;

	// Popule the text_content list
	while( not text.eof() && text_contents.empty() ) {
		parse();
	}

	if ( ! text_contents.empty() ){
		res = text_contents.front();
		text_contents.pop_front();
	}

	return res;
}


/***********************************************************************
			      HTMLContentIterator
 ***********************************************************************/

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
		text = content.pull().str();

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
		return content.eof();
	}

	bool operator!=(const HTMLContentIterator& other)
	{
		return !operator==(other);
	}

};




/***********************************************************************
				      MAIN
 ***********************************************************************/

int main(int argc, char* argv[])
{
	for(int i = 1; i < argc; ++i) {
		try{
			AutoFilebuf dec(decompres(argv[i]));
			filebuf f = dec.getFilebuf();
			HTMLContentIterator it(f), endit;
			std::ostream_iterator<std::string> os(std::cout, " ");
			std::copy(it, endit, os);
		} catch(...) {
			// pass
		}
	}

	std::cout << "Done." << std::endl;
//        std::string press_enter;
//        std::cin >> press_enter;

}
