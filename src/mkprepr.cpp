// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "zfilebuf.h"
#include "strmisc.h"
#include "isamutils.hpp"
#include "mkstore.hpp"
#include "urltools.h"
#include "htmlparser.h"
#include "fnv1hash.hpp"

#include <fstream>
#include <sstream>



/*******************************************************************************
				    Typedefs
 *******************************************************************************/

typedef hash_map < int, std::string> TIdUrlMap;

//!@name URL Fingerprint containers
//!@{
typedef hash_set < uint64_t > TURLFingerpritSet;

typedef std::vector<uint64_t> TURLFingerprintVec;
//!@}


/*******************************************************************************
			      LinkExtractorVisitor
 *******************************************************************************/


struct LinkExtractorVisitor {
	// Statistics
	docid_t d_count;
	uint64_t byte_count;
	uint64_t last_byte_count;
	time_t last_broadcast;
	time_t time_started;

	int nlinks; // FIXME

	TIdUrlMap& id2url;

	LinkExtractorVisitor(TIdUrlMap& urls)
	: d_count(0), byte_count(0), last_byte_count(0),
	  last_broadcast(time(NULL)), time_started(time(NULL)),
	  nlinks(0), // FIXME
	  id2url(urls)
	{}

	void operator()(uint32_t count, const store_hdr_entry_t* hdr,
			filebuf store_data)
	{
		uint32_t* docid = (uint32_t*) store_data.read(sizeof(uint32_t));
		uint32_t* len = (uint32_t*) store_data.read(sizeof(uint32_t));
		assert(*docid == hdr->docid);


		// read document
		AutoFilebuf dec(decompress(store_data.readf(*len)));
		filebuf f = dec.getFilebuf();

		TURLFingerprintVec links(getLinks(*docid, f));
		nlinks += links.size(); // FIXME

		// FIXME
		// ouput data...
		// FIXME

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

	TURLFingerprintVec getLinks(uint32_t docid, filebuf data);

};


TURLFingerprintVec LinkExtractorVisitor::getLinks(uint32_t docid,
						     filebuf data)
{
	assert( id2url.find(docid) != id2url.end() );
	BaseURLParser base(id2url[docid]);

	TURLFingerpritSet links;

	LinkExtractor parser(data);
	if (not parser.base.empty()) {
		base = BaseURLParser(parser.base);
	}
	// XXX we are ignoring nofollow

	// Prepare to get all the links from the page
	const BaseURLParser base_url(base);
	std::set<std::string>::const_iterator li;
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
		"Usage:\t mkprepr docid_list store_dir\n"
		"\n"
		"\tdocid_list\tList of docid-url for all documents in store.\n"
		"\tstore_dir\tWhere the crawled data (in store) is\n"
		<< std::endl;
}


/*******************************************************************************
				      MAIN
 *******************************************************************************/


int main(int argc, char* argv[])
{
	const char* docids_list;
	const char* store_dir;

	/* Parse command line */
	if(argc < 3) {
		std::cerr << "Wrong number of argments" << std::endl;
		show_usage();
		exit(EXIT_FAILURE);
	}
	docids_list = argv[1];
	store_dir = argv[2];

	std::cout << "# Loading the id-to-url mapping..." << std::endl;
	TIdUrlMap id2url;
	read_ids_and_urls(docids_list, id2url);

	// Setup result outputter // FIXME
	LinkExtractorVisitor visitor(id2url);

	std::cout << "# Extracting links ..." << std::endl;
	VisitIndexedStore<store_hdr_entry_t>(store_dir, "store", visitor);

	sleep(1);
	visitor.print_stats();

	exit(EXIT_SUCCESS);
}


//EOF
