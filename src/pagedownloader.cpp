#include "pagedownloader.h"


const std::string PageDownloader::DEFAULT_ENCODING = "UTF-8";

PageDownloader&  PageDownloader::get()
{
	download();
	parse();

	return *this;
}

PageDownloader&  PageDownloader::download()
{
	BaseURLParser original_url = url;
	URLRetriever page(url.str());
	page.go();
	BaseURLParser redirected_url;

	std::string ct; // Content-Type

	// Obtain information from HTTP headers
	try{
		//FIXME we should have usued sanitizeURL here we 
		//were handling URLs's fragment and query components
		redirected_url = page.getLocation();
	} catch (NotSupportedSchemeException) {
		base = page.getLocation();
		throw;
	}

	if  ( original_url != redirected_url ) {
		base = redirected_url;
	}

	URLRetriever::headers_t& headers = page.getHeaders();
	if ( headers.count("content-type") ){
		ct = headers["content-type"];
		// Verify if this is HTML or XML
		verifyContentType(ct);
		// Get encoding
		encoding = FindEncParser::get_charset_from_content_type(ct);
		if ( encoding.empty() ) {
			encoding = DEFAULT_ENCODING;
		}
	}
	filebuf page_contents = page.getData();
	contents.copy(page_contents);

	return *this;
}

PageDownloader&  PageDownloader::parse()
{
	std::set<std::string>::const_iterator li;
	// Converting to unicode
	UnicodeBugger unicoder(contents.getFilebuf());
	unicode_contents.reset(unicoder.convert());
	encoding = unicoder.getEncoding();

	// extracting link and meta information
	LinkExtractor parser(unicode_contents.getFilebuf());
	parser.parse();
	if (not parser.base.empty()) {
		base = BaseURLParser(parser.base);
	}
	follow = parser.follow;
	index = parser.index;

	// Prepare to get all the links from the page
	const BaseURLParser base_url(base);
	for(li = parser.links.begin(); li != parser.links.end(); ++li){
		try{
			BaseURLParser l(*li);
			// FIXME this is where dynamic and fragments should be removed
			if (l.isRelative()){
				links.insert( (base_url + l).strip() );
			} else {
				links.insert(l.strip());
			}
		} catch (NotSupportedSchemeException) {
			// We just blindly ignore unsupported and invalid URLs
		} catch (InvalidURLException ) {
			// We just blindly ignore unsupported and invalid URLs
		}
		
	}

	return *this;
}

void PageDownloader::writeMeta(std::ostream& fh)
{
	std::string index = (this->index) ? "index" : "noindex";
	std::string follow = (this->follow) ? "follow" : "nofollow";

	fh << "encoding: "<< this->encoding << "\nrobots: " <<
		follow << "," << index << std::endl;

}

void PageDownloader::verifyContentType(std::string ct)
{
	to_lower(ct);

	if ( (ct.find("html") == ct.npos) and (ct.find("xml") == ct.npos) ){
		// This is neither HTML nor XML
		throw NotHTMLException();
	}
	
}

// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
