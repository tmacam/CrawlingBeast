// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "zfilebuf.h"
#include "strmisc.h"
#include "isamutils.hpp"
#include "urltools.h"
#include "htmlparser.h"
#include "fnv1hash.hpp"

#include <fstream>
#include <sstream>

#include "mkstore.hpp" // For store_hdr_entry_t and store_data_entry_t


/***********************************************************************
				    Typedefs
 ***********************************************************************/

typedef hash_map <uint32_t, std::string> TIdUrlMap;

//!@name URL Fingerprint containers
//!@{
typedef hash_set < uint64_t > TURLFingerpritSet;

typedef std::vector<uint64_t> TURLFingerprintVec;
//!@}

//!@name Pre-PR Index Store structures
//!@{
//!To read the header of an entry in the header file
typedef store_hdr_entry_t prepr_hdr_entry_t;

//!To read the header of an entry in a data file
typedef store_data_entry_t prepr_data_entry_t;

/**To read the contents of an data entry.
 *
 * The idea is that a data entry has an header and contents
 * and that the header @p len is the length of the data entry
 * contents.
 *
 * After this data header an array of uint64_t[n_outlinks]
 * follows.
 *
 */
struct prepr_contents_entry_t {
	uint64_t fp;	//!< fingerprint
	uint32_t n_outlinks;//!< title of the document

	prepr_contents_entry_t(uint32_t fingerprint=0, uint32_t n=0)
	: fp(fingerprint), n_outlinks(n)
	{}

} __attribute__((packed));

//!@}




/***********************************************************************
			      LinkExtractorVisitor
 ***********************************************************************/

class LinkExtractorVisitor {
	// Statistics
	docid_t d_count;
	uint64_t byte_count;
	uint64_t last_byte_count;
	time_t last_broadcast;
	time_t time_started;

	int nlinks; // FIXME

	TIdUrlMap& id2url;
	IndexedStoreOutputer<prepr_hdr_entry_t>& outputer;
public:

	LinkExtractorVisitor(TIdUrlMap& urls,
			IndexedStoreOutputer<prepr_hdr_entry_t>& out)
	: d_count(0), byte_count(0), last_byte_count(0),
	  last_broadcast(time(NULL)), time_started(time(NULL)),
	  nlinks(0), // FIXME
	  id2url(urls), outputer(out)
	{
		sleep(2);
	}

	void operator()(uint32_t count, const store_hdr_entry_t* hdr,
			filebuf store_data)
	{
		const size_t len = sizeof(prepr_data_entry_t);
		prepr_data_entry_t* data_header = 0;
		data_header = (prepr_data_entry_t*)store_data.read(len);

		assert(data_header->docid == hdr->docid);

		// read document
		AutoFilebuf dec(decompress(store_data.readf(data_header->len)));
		filebuf f = dec.getFilebuf();

		// Get self fingerprint
		uint64_t fp =  FNV::hash64(id2url[data_header->docid]);

		// Out out-links
		TURLFingerprintVec links(getLinks(data_header->docid, f));
		nlinks += links.size(); // FIXME

		outputLinkdata(data_header->docid, fp, links);

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
			" elapsed " << now - time_started << " nlinks "<<
			nlinks << std::endl;

		last_broadcast = now;
		last_byte_count = byte_count;
	}

	TURLFingerprintVec getLinks(uint32_t docid, filebuf data);

	void outputLinkdata(uint32_t docid, uint64_t fp,
		const TURLFingerprintVec& fingerprints);

};


TURLFingerprintVec LinkExtractorVisitor::getLinks(uint32_t docid,
						     filebuf data)
{
	assert( id2url.find(docid) != id2url.end() );
	BaseURLParser base(id2url[docid]);

	TURLFingerpritSet links;

	LinkExtractor parser(data);
	parser.parse();
	if (not parser.base.empty()) {
		BaseURLParser doc_base(parser.base);
		// Sanity check
		if ( not doc_base.isRelative() ) {
			base = doc_base;
		}
	}
	// XXX we are ignoring nofollow

	// Sanity check
	if( base.isRelative() ) {
		return TURLFingerprintVec();
	}

	// Prepare to get all the links from the page
	const BaseURLParser base_url(base);
	LinkExtractor::link_set_t::const_iterator li;
	for(li = parser.links.begin(); li != parser.links.end(); ++li){
		try {
			BaseURLParser l(*li);
			std::string link =  (base_url + l).strip().str();
			uint64_t fp = FNV::hash64(link);
			links.insert(fp);
		} catch (NotSupportedSchemeException) {
			// We just blindly ignore unsupported and invalid URLs
		} catch (InvalidURLException ) {
			// We just blindly ignore unsupported and invalid URLs
		}
	}

	return TURLFingerprintVec(links.begin(),links.end());
}

void LinkExtractorVisitor::outputLinkdata(uint32_t docid, uint64_t fp,
		const TURLFingerprintVec& fingerprints)
{
	uint16_t fileno;
	uint32_t pos;

	size_t cont_len = sizeof(prepr_contents_entry_t) +
				(fingerprints.size() * sizeof(uint64_t) );
	size_t needed =	sizeof(prepr_data_entry_t) + cont_len;

	filebuf data = outputer.getDataOutputBuffer(needed,fileno,pos);
	outputer.putIndexEntry(prepr_hdr_entry_t(docid,fileno,pos));

	prepr_data_entry_t data_header(docid, cont_len);
	prepr_contents_entry_t contents_header(fp,fingerprints.size());

	dumpToFilebuf(data_header, data);
	dumpToFilebuf(contents_header, data);
	dumpVecToFilebuf(fingerprints, data);

	assert(data.eof());
}



/***********************************************************************
				 Aux. Functions
 ***********************************************************************/


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
		"Usage:\t mkprepr docid_list store_dir output_dir\n"
		"\n"
		"\tdocid_list\tList of docid-url for all documents in store.\n"
		"\tstore_dir\tWhere the crawled data (in store) is\n"
		"\toutput_dir\tWhere the metadata ISAM will be written.\n"
		<< std::endl;
}


/***********************************************************************
				      MAIN
 ***********************************************************************/

void go(char* argv[])
{
	const char* docids_list = argv[1];
	const char* store_dir = argv[2];
	const char* output_dir = argv[3];

	std::cout << "# Loading the id-to-url mapping..." << std::endl;
	TIdUrlMap id2url;
	read_ids_and_urls(docids_list, id2url);

	// Setup result outputter
	IndexedStoreOutputer<prepr_hdr_entry_t> preprout(output_dir,
							"prepr",
							id2url.size() );
	LinkExtractorVisitor visitor(id2url, preprout);

	std::cout << "# Extracting links ..." << std::endl;
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

	go(argv); // XXX Why, Oh!, Why do I have to code this workarounds for
		// these silly C++ issues/bugs/ghots...


	exit(EXIT_SUCCESS);
}


//EOF
