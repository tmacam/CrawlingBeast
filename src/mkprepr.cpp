// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "zfilebuf.h"
#include "strmisc.h"
#include "isamutils.hpp"
#include "urltools.h"
#include "htmlparser.h"

#include <fstream>
#include <sstream>

#include "mkprepr.hpp"

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

	//!@name To filter found FPs
	//!{
	TURLFingerprintSet valid_fps;
	IsInMap<TURLFingerprintSet> is_a_valid_fp;
	//!}
public:

	LinkExtractorVisitor(TIdUrlMap& urls,
			IndexedStoreOutputer<prepr_hdr_entry_t>& out)
	: d_count(0), byte_count(0), last_byte_count(0),
	  last_broadcast(time(NULL)), time_started(time(NULL)),
	  nlinks(0), // FIXME
	  id2url(urls), outputer(out), valid_fps(),
	  is_a_valid_fp(valid_fps)
	{
		// Populate the list of valid fingerprints
		TIdUrlMap::const_iterator i;
		for(i = id2url.begin(); i != id2url.end(); ++i){
			const std::string& url = i->second;
			valid_fps.insert(FNV::hash64(url));
		}
	}

	void operator()(uint32_t count, const store_hdr_entry_t* hdr,
			filebuf store_data)
	{
		store_data_entry_t* data_header = NULL;
		data_header = readFromFilebuf<store_data_entry_t>(store_data);

		assert(data_header->docid == hdr->docid);

		// read document
		AutoFilebuf dec(decompress(store_data.readf(data_header->len)));
		filebuf f = dec.getFilebuf();

		// Get self fingerprint
		uint64_t fp =  FNV::hash64(id2url[data_header->docid]);

		// Out out-links
		TURLFingerprintVec page_links(getLinks(data_header->docid, f));
		// Filter for valid out-links
		TURLFingerprintVec valid_links;
		valid_links.reserve(page_links.size());
		copy_if( page_links.begin(), page_links.end(),
			 std::back_inserter(valid_links),
			 is_a_valid_fp);

		nlinks += valid_links.size(); // FIXME

		outputLinkdata(data_header->docid, fp, valid_links);

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

	TURLFingerprintSet links;

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

	size_t cont_len = (fingerprints.size() * sizeof(uint64_t) );
	size_t needed =	sizeof(prepr_data_entry_t) + cont_len;

	filebuf data = outputer.getDataOutputBuffer(needed,fileno,pos);
	outputer.putIndexEntry(prepr_hdr_entry_t(docid,fileno,pos));

	prepr_data_entry_t data_header(docid, cont_len, fp,
					fingerprints.size() );

	dumpToFilebuf(data_header, data);
	dumpVecToFilebuf(fingerprints, data);

	assert(data.eof());
}



/***********************************************************************
				 Aux. Functions
 ***********************************************************************/


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
