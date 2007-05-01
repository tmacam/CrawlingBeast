#ifndef __PAGEDOWNLOADER_H
#define __PAGEDOWNLOADER_H
/**@file pagedownloader.h
 * @brief Downloads a page
 *
 * FIXME dynamic pages should NOT be index or retrieved, but normalized
 *
 * FIXME PageDownloader::parse -> this is where dynamic and fragments
 *	 should be removed
 */

#include "unicodebugger.h"
#include "htmlparser.h"
#include "urltools.h"
#include "urlretriever.h"

#include <iostream>
#include <ext/hash_set>

/* ********************************************************************** *
				    TYPEDEFS
 * ********************************************************************** */

struct eqstr
{
  bool operator()(const std::string& s1, const std::string& s2) const
  {
    return s1 == s2;
  }
};


struct str_hash
{
  __gnu_cxx::hash<const char*> H;
  size_t operator()(const std::string& s1) const
  {
    return H(s1.c_str());
  }
};

struct url_path_hash
{
  __gnu_cxx::hash<const char*> H;
  size_t operator()(const BaseURLParser& s1) const
  {
    return H(s1.path.c_str());
  }
};

struct equrl
{
  bool operator()(const BaseURLParser& s1, const BaseURLParser& s2) const
  {
    return s1 == s2;
  }
};


typedef __gnu_cxx::hash_set<BaseURLParser,url_path_hash,equrl> URLSet;


/* ********************************************************************** *
				 PAGEDOWNLOADER
 * ********************************************************************** */


//!Simple downloader for a page/URL
class PageDownloader {
	
	//! The origina URL
	BaseURLParser url; 

public:
	/**Document base URL.
	 *
	 * Redirects and META BASE tags may difficult the task of
	 * rebuilding URLS from relative URLs. The base address
	 * is thus an address where this document was really obtained
	 */
	BaseURLParser base;

	//!Should this page be followed
	bool follow;

	//!Should this page be indexed
	bool index;

        //!To what pages/URLs it points to.
	URLSet links;

        /**This page encoding. 
	 * Defaults to utf-8, since it's stricter and
	 * almost anything is valid latin1
	 */
        std::string encoding;

	AutoFilebuf contents;

	typedef URLSet url_set_t;
	static const std::string DEFAULT_ENCODING;

	/**Constructs the PageDownloader.
	 *
	 * @todo FIXME we don't perform URL sanitization here
	 * because BaseURLParser just igores URLs'
	 * query and fragment components.
	 */
	PageDownloader(BaseURLParser _url = BaseURLParser("")):
		url(_url), base(url),
		follow(true), index(true),
		links(), encoding(DEFAULT_ENCODING),
		contents()
	{ }

	/**Retrieve and process this page.
	 *
	 * @return A reference to itself.
	 */
	PageDownloader& get();

	PageDownloader& download();

	/**Does URL normalization and sanitization.
	 *
	 * Remove query and fragments from a URL.
	 *
	 * @todo BaseURLParser we have now performns normalization
	 *       already,  and since it doesn't parse query and 
	 *       fragment components, we don't have to remove them.
	 *
	 *       FIXME should be part of urltools
	 *
	 * @deprecated Just don't use this! It is here for historical
	 * 		purposes.
	 */
	/*BaseURLParser&*/void sanitizeURL(BaseURLParser& url){}

	/**Parses page content.
	 *
	 * Unicod'ification and metadata and link extraction happens here.
	 *
	 * @throw CannotFindSuitableEncodingException
	 */
	PageDownloader& parse();

	//!Writes metadata about this page to a istream
	void writeMeta(std::ostream& fh);

};

/*
def get_page(url):
	p = PageDownloader(url)
	p.get()
	return (p.base, p.parser.links, p.links, p.encoding)

if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1 and sys.argv[1] != '-v':
        for url in sys.argv[1:] :
            (base, links, norm_links, encoding) = get_page(url)
            print "URL:",url
            print "\tBASE:",base
            print "\tencoding:", encoding
            print "\tlinks (não normalizados):"
            for l in links: print "\t    ", l
            print "\tlinks normalizados:"
            for l in norm_links: print "\t    ", l
            print "\n\n"
    else:
        _test()
        print "Done."

*/




#endif // __PAGEDOWNLOADER_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
