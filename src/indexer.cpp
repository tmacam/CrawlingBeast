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

#include <fstream>
#include <sstream>
#include <iomanip>

#include <locale.h>


/***********************************************************************
			     Typedefs and constants
 ***********************************************************************/

typedef __gnu_cxx::hash_map < std::string, int,
                        std::tr1::hash<std::string> > StrIntMap;

typedef std::map<int,std::string> IntStrMap;


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
				 RUN ITERATORS
 ***********************************************************************/

/** Stores <term_id, doc_id, freq> triples.
 *
 * Theses triples are mostly used while generating "runs", hence the the
 * name "run_triple"
 */
struct run_triple{

	uint32_t termid;
	uint32_t docid; // docid_t is a unit32_t
	uint16_t freq;

	run_triple(uint32_t tid = 0, docid_t did=0, uint16_t f=0)
	: termid(tid), docid(did), freq(f)
	{}

	/**Comparison operator (less than).
	 *
	 * The smaller run_triple is the one with the smaller termid.
	 * Docid is used to resolve ties,  winning the one with the smallest
	 * one.
	 */
	inline bool operator< (const run_triple& other) const
	{
		return (termid < other.termid) ||
			(termid == other.termid && docid < other.docid);
	}
	
} __attribute__((packed));

/**Inserter or output interator for runs.
 *
 * This is just syntatic sugar for writing runs to disk.
 *
 * @warning This class ONLY flushes all it's data on descruction.
 */
class run_inserter :public std::iterator<std::output_iterator_tag, run_triple, void,
					void, void>
{
private:
	std::string path_prefix;
	size_t max_length; //!< Max ammount of bytes we reserve to store triples
	size_t max_triples; //!< Max number of tripes we hold in memory
	
	run_triple* run_buf; //!< Triples buffer
	run_triple* cur_triple; //!< The position of the next triple writing pos
	run_triple* end; //!< Past-the-last-triple-in-buffer position

	int n_runs; //!< Number of runs flushed to disk so far


	//! Not default constructible
	run_inserter();

	//! Prevent Copying and assignment.
	run_inserter(const run_inserter& );

	//! Prevent Copying and assignment.
	run_inserter& operator=(const run_inserter&);
public:
	/**Constructor.
	 *
	 * @param prefix prefix where the run_?? file will be stored.
	 *        It can include a full path.
	 * @param length max ammount in bytes of memory will be used to hold
	 * 		 a run in memory before flushing it to a run file.
	 *
	 */
	run_inserter(const std::string prefix, size_t length)
	: path_prefix(prefix),
	  max_length(length),
	  max_triples( max_length / sizeof(run_triple)),
	  run_buf( new run_triple[ max_triples ]),
	  cur_triple(run_buf),
	  end(&cur_triple[max_triples]),
	  n_runs(0)
	{}

	~run_inserter()
	{
		try {
			flush();
		} catch(...) {
			std::cerr << "An error ocurred in run_inserter" << 
				std::endl;
		}

		delete run_buf;
	}

	inline run_inserter& operator=(const run_triple& val)
	{
		// Add current
		*cur_triple++ = val;

		if(cur_triple == end) {
			flush();
		}

		return *this;
	}

	inline run_inserter& operator*() {return *this;}	// deref
	inline run_inserter& operator++() {return *this;}	// prefix
	inline run_inserter& operator++(int) {return *this;}	// postfix

	/** Write the current pending/remaining triples to disk.
	 *
	 * You should call this function before instance desctruction.
	 * Although this class destructor calls flush any possible
	 * raised exception will be consumed and ignored if called inside
	 * the destructor.
	 */
	void flush()
	{

		// We only run if there are pending triples
		if (cur_triple == run_buf){
			return;
		}

		// sort triples
		std::sort(run_buf,cur_triple);

		// Ok. There are pending triples.
		unsigned int n_triples_pending = cur_triple - run_buf;

		// Let's write 'em to disk
		// FIXME woudn't UNIX I/O rotines perform better?
		std::ostringstream filename_stream;
		std::ofstream out;
		// Turn on exception reporting for file operations
		out.exceptions( std::ios_base::badbit|std::ios_base::failbit);
		filename_stream << path_prefix << "/run_" <<
			std::hex <<  std::setw(4) << std::setfill('0') <<
			n_runs; //"%s/run_%04x"
		out.open(filename_stream.str().c_str(),
			 std::ios::binary | std::ios::out);
		out.rdbuf()->pubsetbuf(0, 0); // unbuffering out
		out.write( (char *)run_buf,
			   n_triples_pending * sizeof(run_triple));

		// Update the number of runs written
		++n_runs;
		//reset reading/writing position
		cur_triple = run_buf;
	}

	int getNRuns() const {return n_runs;}

};



/***********************************************************************
		     VOCABULARY RETRIEVAL AND NORMALIZATION
 ***********************************************************************/


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
 * 	 http://www.decodeunicode.org/w3.php?viewMode=block&ucHex=0080
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


/**Retrieve the term or word frequency for a given document.
 *
 * @param f The HTML file from which the term frequency will be extracted.
 * @param[out] wfreq The term frequency dictionary. Will be cleared upon
 * 			function start.
 * @param wconv	wide-string converter. Used only for caching and
 * 		performance purposes.
 *
 *
 * wfreq is cleared at every function call.
 */
void getWordFrequency(filebuf f, StrIntMap& wfreq,
			const WideCharConverter& wconv)
{
	WIsalpha isalpha;

	HTMLContentIterator ci(f), ce;
	std::wstring word;

	// Clear word frequency
	wfreq.clear();


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


/***********************************************************************
				      TEST
 ***********************************************************************/

void testTripleInserter()
{
	const int KB = 1<<10;

	run_inserter run("/tmp/down/", 10*KB);

	for(int i = int(7.5*float(KB)); i > 0 ; --i){
		*run++ = run_triple(i,i,i);
	}

	run.flush();
	std::cout << "Foram geradas " << run.getNRuns() <<
		" runs" << std::endl;

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

inline std::string make_filename(std::string store_dir, docid_t docid)
{
	std::ostringstream id_hex;

	id_hex << std::uppercase << std::hex << std::setw(8) <<
                std::setfill('0') << docid;
	std::string id_hex_str = id_hex.str();

	std::string id_path =  store_dir + "/" + 
				id_hex_str.substr(0,2) + "/" +
				id_hex_str.substr(2,2) + "/" +
				id_hex_str.substr(4,2) + "/" +
				id_hex_str.substr(6,2) + "/data.gz"; // FIXME data.gz constant
	return id_path;
}

inline void mapIdToTerms(const StrIntMap& term2id, IntStrMap& id2term)
{
	StrIntMap::const_iterator v;

	for(v = term2id.begin(); v != term2id.end(); ++v){
		const std::string& term = v->first;
		const int& id = v->second;

		id2term[id] = term;
	}
}

inline void dump_vocabulary(const StrIntMap& vocabulary, const char* output_dir)
{
	// Setup dump files' filename
	std::string voc_prefix(output_dir);
	voc_prefix += "/vocabulary";
	std::string voc_hdr_filename =  voc_prefix + ".hdr";
	std::string voc_data_filename =  voc_prefix + ".data";

	// Create, turn exception reporting on and open the files
	std::ofstream header;
	std::ofstream data;
	header.exceptions( std::ios_base::badbit|std::ios_base::failbit);
	data.exceptions( std::ios_base::badbit|std::ios_base::failbit);
	header.open(voc_hdr_filename.c_str(),std::ios::out | std::ios::binary);
	data.open(voc_data_filename.c_str(), std::ios::out | std::ios::binary);

	IntStrMap id2term;
	IntStrMap::const_iterator i2ti;
	mapIdToTerms(vocabulary, id2term);

	int expected_id = 0;
	uint32_t pos;

	/* id2term is a (ordered) map. So we assume we will sequentially
	 * iterate from full [0,max_term_id] range.
	 *
	 * If this is found to be incorrect.. well, the assert will tell us.
	 */

	for(i2ti = id2term.begin(); i2ti != id2term.end(); ++i2ti){
		const int& tid = i2ti->first;
		const std::string& term = i2ti->second;

		pos = data.tellp(); // this term start position in .data
		data.write(term.c_str(), term.size() +1); // account for \0
		header.write((char*)&pos, sizeof(pos));

		//FIXME this is here just for DEBUGing purposes
		assert(tid == expected_id);
		++expected_id;

	}
}

void index_files(const char* store_dir, const char* docids_list,
		const char* output_dir)
{
	std::ifstream known_docids(docids_list);
	std::string url;
	docid_t docid;

	const unsigned int KB = 1<<10;
	run_inserter runs(output_dir, 100*KB );

	StrIntMap vocabulary; // term -> term_id
	StrIntMap wfreq; // term -> frequency in current doc
	WideCharConverter wcconv;

	StrIntMap::const_iterator w; // intra-document word iterator

	// For every docid / retrieved document
	while(known_docids >> docid >> url){
		std::string filename = make_filename(store_dir, docid);

		//FIXME
		std::cout << "DOCID " << docid << std::endl;

		// read document (decompressing)
		AutoFilebuf dec(decompres(filename.c_str()));
		filebuf f = dec.getFilebuf();

		// parse document and get intra-ducument term frequency
		getWordFrequency(f, wfreq, wcconv);

		// For every term in the document
		for(w = wfreq.begin(); w != wfreq.end(); ++w){
			const std::string& term = w->first;
			const int& freq =  w->second;

			// Add to vocabulary if this is an unknown term
			if (vocabulary.find(term) == vocabulary.end()) {
				// We MUST get the current number of elements
				// inside the vocabulary separatly: assigning
				// voc[term] = size() in a single statement
				// makes termid start couting from 1...
				int len = vocabulary.size();
				vocabulary[term] = len;
			}

			// add this triple to the current run(s)
			*runs++ = run_triple(	vocabulary[term],
						docid,
						freq);
		} // end for each word in document
	} // end for each document

	dump_vocabulary(vocabulary, output_dir);
}


/***********************************************************************
				      MAIN
 ***********************************************************************/

int main(int argc, char* argv[])
{
	const char* store_dir;
	const char* docid_list;
	const char* output_dir;

	if(argc < 4) {
		std::cerr << "wrong number of arguments" << std::endl;
		std::cerr << "indexer store_dir docid_list output_dir" << std::endl;
		exit(1);
	}

	store_dir = argv[1];
	docid_list = argv[2];
	output_dir = argv[3];

	index_files(store_dir, docid_list, output_dir);
	exit(0);

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
