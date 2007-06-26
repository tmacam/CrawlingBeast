// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "zfilebuf.h"
#include "strmisc.h"
#include "isamutils.hpp"
#include "htmlparser.h"
#include "fnv1hash.hpp"

#include <fstream>
#include <sstream>

#include "mkstore.hpp" // For store_hdr_entry_t and store_data_entry_t

/***********************************************************************
                             Typedefs and constants
 ***********************************************************************/

typedef hash_map <uint32_t, std::string> TIdUrlMap;

//!To read the header of an entry in the header file
typedef store_hdr_entry_t meta_hdr_entry_t;

//!To read the header of an entry in a data file
typedef store_data_entry_t meta_data_entry_t;

/**To read the contents of an data entry.
 *
 * The idea is that a data entry has an header and contents
 * and that the header @p len is the length of the data entry
 * contents.
 *
 */
struct meta_contents_entry_t {
	uint32_t url;	//!< document's url length
	uint32_t title;	//!< document's title length

	meta_contents_entry_t(uint32_t _u=0, uint32_t _t=0)
	: url(_u), title(_t)
	{}

} __attribute__((packed));


/***********************************************************************
				 TitleExtractor
 ***********************************************************************/

class TitleExtractor : public SloppyHTMLParser {
public:
	std::string title;
	bool in_title;

	TitleExtractor(const filebuf& text)
	: SloppyHTMLParser(text),
	  title(),
	  in_title(false)
	{}

	void handleStartTag(const std::string& tag_name,
			attr_list_t& attrs, bool empty_element_tag=false)
	{
		// This should be SloppyHTMLParser default behaviour...
		safeHandleStartTag(tag_name, attrs, empty_element_tag);
		if (skipTagIfTroublesome(tag_name,empty_element_tag)) {
			handleEndTag(tag_name);
		}
	}


	void safeHandleStartTag(const std::string& tag_name,
			attr_list_t& attrs, bool empty_element_tag=false)
	{
		std::string name = tag_name;
		to_lower(name);
		if(name == "title") {
			in_title = true;
		}
	}

	void handleEndTag(const std::string& tag_name)
	{
		std::string name = tag_name;
		to_lower(name);
		if(name == "title") {
			in_title = false;
			// Just finish parsing...
			text.read(text.len());
		}

	}

	void handleText(filebuf text)
	{
		if (in_title) title += text.str();
	}

};




/***********************************************************************
			      TitleExtractorVisitor
 ***********************************************************************/


class TitleExtractorVisitor {
	// Statistics
	docid_t d_count;
	uint64_t byte_count;
	uint64_t last_byte_count;
	time_t last_broadcast;
	time_t time_started;

	TIdUrlMap& id2url;
	IndexedStoreOutputer<meta_hdr_entry_t>& outputer;
public:

	TitleExtractorVisitor( TIdUrlMap& urls,
			IndexedStoreOutputer<meta_hdr_entry_t>& out)
	: d_count(0), byte_count(0), last_byte_count(0),
	  last_broadcast(time(NULL)), time_started(time(NULL)),
	  id2url(urls), outputer(out)
	{
	}

	void operator()(uint32_t count, const store_hdr_entry_t* hdr,
			filebuf store_data)
	{
		const size_t len = sizeof(meta_data_entry_t);
		meta_data_entry_t* data_header = 0;
		data_header = (meta_data_entry_t*)store_data.read(len);

		assert(data_header->docid == hdr->docid);


		// read document
		AutoFilebuf dec(decompress(store_data.readf(data_header->len)));
		filebuf f = dec.getFilebuf();

		// Get URL
		std::string url =  id2url[data_header->docid];

		// Get title
		TitleExtractor parser(f);
		parser.parse();
//                std::cout << data_header->docid << " - " << parser.title <<
//                        std::endl;


		outputMetadata(data_header->docid, url, parser.title);

		// Statistics and prefetching
		++d_count;
		byte_count += f.len();
		if (d_count  % 1000 == 0) {
			print_stats();
		}
	}

	void print_stats()
	{
		time_t now = time(NULL);

		if (now == last_broadcast) return; // Avoid FPErr

		uint64_t byte_amount = byte_count - last_byte_count;
		std::cout << "# docs: " << d_count << " bytes: " <<
			byte_amount << " / " << byte_count << " bps: "<<
			byte_amount/(now - last_broadcast) << 
			" elapsed " << now - time_started << std::endl;

		last_broadcast = now;
		last_byte_count = byte_count;
	}

	void outputMetadata(uint32_t docid, const std::string& url,
			const std::string& title)
	{
		uint16_t fileno;
		uint32_t pos;

		size_t cont_len = sizeof(meta_contents_entry_t) +
					url.size() +
					title.size();
		size_t needed =	sizeof(meta_data_entry_t) + cont_len;

		filebuf data = outputer.getDataOutputBuffer(needed,fileno,pos);
		outputer.putIndexEntry(meta_hdr_entry_t(docid,fileno,pos));

		meta_data_entry_t data_header(docid, cont_len);
		meta_contents_entry_t contents_header(url.size(),title.size());

		dumpToFilebuf(data_header, data);
		dumpToFilebuf(contents_header, data);
		dumpStrToFilebuf(url, data);
		dumpStrToFilebuf(title, data);

		assert(data.eof());
	}

};


/*******************************************************************************
				 Aux. Functions
 *******************************************************************************/


inline TIdUrlMap& read_ids_and_urls(const char * docid_list, TIdUrlMap& id2url)
{
	std::ifstream known_docids(docid_list);
	std::string url;
	docid_t docid;
	while(known_docids >> docid >> url){
		id2url[docid] = url;
	}

	return id2url;

}

void show_usage()
{
	std::cout <<
		"Usage:\t mkmeta docid_list store_dir output_dir\n"
		"\n"
		"\tdocid_list\tList of docid-url for all documents in store.\n"
		"\tstore_dir\tWhere the crawled data (in store) is.\n"
		"\toutput_dir\tWhere the metadata ISAM will be written.\n"
		<< std::endl;
}


/*******************************************************************************
				      MAIN
 *******************************************************************************/

void go(char* argv[])
{
	const char* docids_list = argv[1];
	const char* store_dir = argv[2];
	const char* output_dir = argv[3];

	std::cout << "# Loading the id-to-url mapping..." << std::endl;
	TIdUrlMap id2url;
	read_ids_and_urls(docids_list, id2url);

	// Setup result outputter
	IndexedStoreOutputer<meta_hdr_entry_t> metaout(	output_dir,
							"meta",
							id2url.size() );
	TitleExtractorVisitor visitor(id2url, metaout);

	std::cout << "# Extracting titles ..." << std::endl;
	VisitIndexedStore<store_hdr_entry_t>(store_dir, "store", visitor);
	visitor.print_stats();

}

int main(int argc, char* argv[])
{
	/* Parse command line */
	if(argc < 4) {
		std::cerr << "Wrong number of argments" << std::endl;
		show_usage();
		exit(EXIT_FAILURE);
	}

	go(argv);

	exit(EXIT_SUCCESS);
}


//EOF
