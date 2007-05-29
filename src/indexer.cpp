// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "zfilebuf.h"
#include "htmlparser.h"
#include "strmisc.h"
#include "entityparser.h"

#include <iterator>
#include <algorithm>
#include <list>
#include <ext/hash_map>
#include <tr1/functional>
#include <functional>

#include <locale.h>


/***********************************************************************
			     Typedefs and constants
 ***********************************************************************/

typedef __gnu_cxx::hash_map < std::string, int,
                        std::tr1::hash<std::string> > StrIntMap;


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


/***********************************************************************
				      MAIN
 ***********************************************************************/

template<class T>
void imprime_conteudo(T is, T ie)
{
	std::ostream_iterator< typename T::value_type> os(std::cout,"");
//        for(; is != ie; ++is, ++os) {
//                *os = *is;
//        }
	std::copy(is, ie , os);
}

void assert_equal(HTMLContentWideCharStreamIterator i, HTMLContentWideCharStreamIterator& e, HTMLContentWideCharStreamIterator end)
{
	while(i != end){
		assert(*i == *e);
		++i; ++e;
	}
}




struct Isalpha : std::unary_function<char, bool> {
	bool operator()(char c) {
		return not (std::ispunct(c) || std::isspace(c));
	}
};

struct WIsalpha : std::unary_function<wchar_t, bool> {
	static const wchar_t control_code_start = 0x007f; // [control] DELETE
	static const wchar_t control_code_end = 0x00A0; // NBSP
	bool operator()(wchar_t c) {
		return not ( (c >= control_code_start && c <= control_code_end)
				|| std::iswpunct(c) || std::iswspace(c));
	}
};

/**Normalize a term or word.
 *
 * For now, normalization means:
 * - lowercasing
 * - converting accented letters to their corresponding representations without
 *   accents.
 *
 * FIXME wouldn't a hashtable be better?
 * FIXME Are we really ignoring non-latin1 vowels? see
 * 	 http://www.windspun.com/unicode-test/unicode.xml
 * 	 and look for letters such as U+0100 ..
 */
inline std::wstring& normalize_term(std::wstring& word)
{
	to_lower(word);
	//filter_accents(word);
	for(size_t i = 0; i < word.size(); ++i){
		wchar_t& c = word[i];
		if (c >= 224 && c <= 230 ) { c = L'a';}
		if (c == 231 ) { c = L'c';}
		if (c >= 232 && c <= 235 ) { c = L'e';}
		if (c >= 236 && c <= 239 ) { c = L'i';}
		if (c == 241 ) { c = L'n';}
		if (c >= 242 && c <= 248 ) { c = L'o';}
		if (c >= 249 && c <= 252 ) { c = L'u';}
		if (c >= 253 && c <= 255 ) { c = L'y';}
	}
	return word;
}


/**Blah.
 *
 * wfreq is cleared at every function call.
 */
void getWordFrequency(filebuf f, StrIntMap& wfreq, WideCharConverter wconv)
{
	WIsalpha isalpha;

	HTMLContentIterator ci(f), ce;
	std::wstring word;

	// Clear word frequency
	// FIXME wfreq.clear();


	// For each text node (text inside and between tags) content
	for(; ci != ce; ++ci){
		std::wstring text_node = wconv.mbs_to_wcs(*ci);
		std::wstring::const_iterator i(text_node.begin()),
				      end(text_node.end());
		// Get all words from this text node 
		while(i != text_node.end()){
			// new word
			word.clear();
			// Find word start
			i = std::find_if(i,end,isalpha);
			// Go until word end
			while(i != end && isalpha(*i)){
				word += *i;
				++i;
			}
			// Grabbed a full word. 
			if(!word.empty()) {
				normalize_term(word);
				std::string w(wconv.wcs_to_mbs(word));
				wfreq[w] = wfreq[w] + 1;
			}
		}

	}
}


void dumpWFreq(filebuf& f, StrIntMap& wfreq)
{
	WideCharConverter wcconv;

	getWordFrequency(f, wfreq, wcconv);
	StrIntMap::const_iterator i;
//        for(i = wfreq.begin(); i != wfreq.end(); ++i){
//                std::cout << i->first << ": " << i->second 
//                        << std::endl;
//        }

}

int main(int argc, char* argv[])
{
//        std::string degenerado("aÇão weißbier 1ªcolocada palavra«perdida»©");
//        filebuf degen(degenerado.c_str(), degenerado.size());
//        dumpWFreq(degen);
//
//        WideCharConverter wconv;
//        std::string in("ßá");
//        std::wstring out =  wconv.mbs_to_wcs(in);
//        std::cout << "Out: " << wconv.wcs_to_mbs(out) << std::endl;

//        return 1;


	StrIntMap wfreq;


	for(int i = 1; i < argc; ++i) {
//                try{
			AutoFilebuf dec(decompres(argv[i]));
			filebuf f = dec.getFilebuf();
			dumpWFreq(f, wfreq);

//                } catch(...) {
//                        throw;
			// pass
//                }
	}
	std::cout << wfreq.size() << std::endl;
	std::map<std::string, int> ordenado(wfreq.begin(), wfreq.end());

	std::map<std::string, int>::const_iterator i;
	for(i = ordenado.begin(); i != ordenado.end(); ++i){
		std::cout << i->first << ": " <<
				i->second << std::endl;
	}


	std::cout << "Done." << std::endl;
//        std::string press_enter;
//        std::cin >> press_enter;




}
