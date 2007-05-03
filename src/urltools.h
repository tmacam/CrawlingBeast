#ifndef __URLTOOLS_H
#define __URLTOOLS_H
/**@file urltools.h
 * @brief URL handling, parsing, decoding and normalization
 *
 * $Id$
 *
 * Most of the code in the module was made conforming to the specs. in
 * RFC 3986, "Uniform Resource Identifier (URI): Generic Syntax".
 *
 * We also used information from RFC 1034, "Domain Names -
 * Concepts and Facilities" for hostname validation and normalization.
 *
 * Assumptions
 * 
 *  FIXME Establishing a Base URI, RFC 3986, Section 5.1
 *  FIXME ":server/" is supposed to be a valid URL?
 *  FIXME add test examples from http://www.mnot.net/python/urlnorm.py
 *  FIXME Max URL size per HTTP protocol specs. (RFC 2616)
 *  	  -> It seems there is none per-protocol!
 *  FIXME Perhaps normalization procedures should be moved from parsing, making
 *        it easier to find what and where are we dealing with each issue
 *        related to normalization...
 *  FIXME Verify if the following is already FIXed
 *  FIXME Percent-encoding RFC 3986, Section 2.1
 *  FIXME URL Normalization, RFC 3986, Sections 6.2.2 and 6.2.3
 *
 *  FIXME userinfo content is not being validated according to the rules
 *  	  stated in section 3.2.1
 *
 */

#include "common.h"
#include "parser.h"

#include <set>

/* **********************************************************************
 *      			    SYMBOLS
 * ********************************************************************** */

/**@name URI and URL Symbols and Commom String Constants.
 *
 * Some constant defitions to ease dealing with URLs and Domain names.
 *
 * This constants have the same meaning as the corresponding rules of the
 * RFC 3986, "URI Generic Syntax, Section 2.2 Reserved Characters".
 *
 * DOMAINNAME_CHARS is based on information on RFC 1034, "Domain Names -
 * Concepts and Facilities".
 * 
 */
//@{

const std::string GEN_DELIMS  = ":/?#[]@";
const std::string SUB_DELIMS  = "!$&'()*+,;=";
const std::string RESERVED    = GEN_DELIMS + SUB_DELIMS;

//! Those should not be percent-encoded.
const std::string UNRESERVED  = LETTERS + DIGITS +  "-._~";
//FIXME Are they being handled correctly?

/**List of valid characters in the HOST component of a URI.
 *
 * This list is based in information on RFC 3986 and RFC 1034.
 */
const std::string DOMAINNAME_CHARS = LETTERS + DIGITS + ".-";

//@}


/* **********************************************************************
 *				   EXCEPTIONS
 * ********************************************************************** */

//!Base class from where all URL-decoding and handling exceptions descend.
class BaseURLException : public std::runtime_error {
public:
        BaseURLException(const std::string& __arg):
                std::runtime_error(__arg){}
};

/**Unsuported URL scheme.
 *
 * Our URL parser is rather limited. If it doesn't implement a certain 
 * funcionality, if just fails :-)
 */
class NotSupportedSchemeException: public BaseURLException{
public:
        NotSupportedSchemeException(const std::string& __arg):
                BaseURLException(__arg){}
};

/**An invalid URL was found.
 *
 * The exact problem is specified in it's message.
 * Use .what() to retrieve it.
 */
class InvalidURLException: public BaseURLException{
public:
        InvalidURLException(const std::string& __arg):
                BaseURLException(__arg){}
};


/* **********************************************************************
 *				   URL PARSER
 * ********************************************************************** */

/**Simple and limited URL Parser and Normalizer.
 *
 * This parser is really limited by the scope of it's use:
 *
 *   - It can only handle HTTP shemas - since our crawler only does HTTP.
 *   - It fails to parse URLs with query components - since our craler ignore
 *     those links.
 *   - It fails if the authority part of the URL has user and password
 *     information - we don't crawl pages that require auth, so no point
 *     in parsing them either.
 *
 * Despite those limitations, our parser does URL normalization on absolute and
 * on relative URLs. Normalization happens durring parsing an during the merge
 * of URIs.
 *
 * We also assume URL data is in UTF-8.
 * 
 * 
 *          foo://example.com:8042/over/there?name=ferret#nose
 *          \_/   \______________/\_________/ \_________/ \__/
 *           |           |            |            |        |
 *        scheme     authority       path        query   fragment
 */
class BaseURLParser : public BaseParser {

protected:
	// FIXME see if we can turn the components into protected
	// members
	
	//!@name URL Normalization and Validation
	//@{

	//!Clean port if it is equal to default http port
	inline void normalizeDefaultHTTPPort(){
		if (this->scheme == "http" and (port == "80" or port == "")){
			this->port = "";
		}
	}

	//!Lowercase and remove trailing '.' from hostnames.
	inline void normalizeHostName(std::string& host)
	{
		std::string::reverse_iterator r;
		std::string cleaned;

		to_lower(host);
		r = host.rbegin();
		if ((host.size() != 0) && ((*r) == '.')) {
			cleaned = host.substr(0,host.size()-1);
			host = cleaned;
		}
	}

	/**Checks if the hostname is valid according o RFC 1034.
	 *
	 * Also check agains empty hostnames
	 *
	 * @see readAuthority
	 *
	 * @note We don't validate for valid IPv4 address and we can't cope
	 * with IPv6 addresses.
	 *
	 * @throw InvalidURLException if a hostname is found to be invalid.
         */
	void validateHostName(const std::string& hostname);

	/**Checks if the port number is valid according o RFC 3986, section 3.2.3.
	 *
	 * All characters in port number must be in the ACII range '0'-'9'.
	 *
	 * @see readAuthority
	 *
	 * @throw InvalidURLException if a port number is found to be invalid.
         */
	void validatePort(const std::string& port);

	/**Remove special "." and ".." from a path segment according to rules
	 * in RFC 3986, Section 5.2.4.
	 *
	 * @param[in] path The path you want to streamline.
	 *
	 * @return The path with streamlined acording with RFC's rules.
	 * 
	 * One extra rule was added between rules 2D and 2E: rule 2D-E.
	 * It does the following: s#^//#/#
	 * 
	 * >>> u = BaseURLParser()
	 * >>> u.removeDotSegments("/a/b/c/./../../g")
	 * u'/a/g'
	 * >>> u.removeDotSegments("mid/content=5/../6")
	 * u'mid/6'
	 * >>> u.removeDotSegments("mid/content=5////../6")
	 * u'mid/6'
	 */
	static std::string removeDotSegments(const std::string& path);

	/**Merges a relative-path URI to a base URI (base).
	 * 
	 * @param base A URL instance with the Base URL
	 * @param rel  A URL instance with the relative-path URI
	 *
	 * @return The base's path merged with rel's path.
	 * 
	 * We follow the algorithm from RFC 3986, Section 5.2.3.
	 */ 
	static std::string mergePath(const BaseURLParser& base, const BaseURLParser& rel);


	//!Read a single Percent-Encoded character and parses it accordingly.
	static std::string decodeAndFixPE(const std::string pe_data);

	//! char -> "%" + HEX(c)
	static std::string charToPE(char _c);


	std::string fixPercentEncoding(const std::string path);

	/*! Observe that http://www.exemple.com/ == http://www.exemple.com ,
	 * as inscructed by RFC 3986, Section 6.2.3,
	 */
	inline void normalizeTrailingSlashAfterAuthority()
	{
		if (this->path.empty() and this->hasAuthority()) {
			this->path = "/";
		}
	}

	//@}
	


	/**@name Syntax Components (S. 3) Rules.
	 * 
	 * The following rules correspond to each one of the 5 components of
	 * a URL, as per RFC 3986, Section 3.
	 *
	 * Observe that the authority section is stored in this class
	 * splited into its 3 subcomponents: userinfo, host and port.
	 */
	//@{
	
	/**Reads the 'scheme' component of a URL, if present.
	 *
	 * See URI Generic Syntax, Section 3.1, Scheme
	 *
	 * @return The value of the scheme component. If a scheme component is
	 * not found or if the one found is invalid, an empty string is
	 * returned. Notice that per RFC 3986, S. 3.1, an empty scheme is
	 * invalid.
	 *
	 * @warning We don't raise InvalidCharError on bad schemes.
	 *
	 * @see URLTest::testEmptyScheme()
	 *
	 */
	std::string readScheme();


	/**Reads an authority component, if present.
	 *
	 * Authority component is preceeded by "//", and we expect to see
	 * those characters on the current reading position.
	 * 
	 * @param[out] userinfo User authentication information
	 * @param[out] host 	Hostname...
	 * @param[out] port 	Port
	 * 
	 * Observe that:
	 *
	 *     - hostname is the only non-optional component of authority and,
	 *     - then whole authority can be empty, and still be a valid URL
	 *       by RFC, (empty host == localhost?)
	 *
	 * but it just doesn't make any sence for us! Thus, IFF the '//' is
	 * present, then a VALID hostname MUST be present in the current URL.
	 * 
	 * @warning We don't validate host and port information.
	 * @todo We don't validate host and port information.
	 * 
	 * @see AuthorityParsingTests
	 *
	 * @throw InvalidURLException  if an error is found during authority
	 * decoding. 
	 *
	 * Exemples of bad URLs:
	 * @code
	 * -  http:// invalid.url:80/
	 * -  http://#urlsite#/estilo.css
	 * @endcode
	 *
	 * Observe that we DON'T support international URLs such as the following:
	 * @code
	 * http://www.ficções.net/biblioteca_conto/
	 * @endcode
	 */
	void readAuthority(std::string& userinfo, std::string& host,
				std::string& port);

	/**Reads a path component.
	 * 
	 * Empty paths in absolute URIs are handled by parse().
	 * 
	 *
	 * @see PathParsingTests
	 *
	 * Observe that http://www.exemple.com/ == http://www.exemple.com ,
	 * as inscructed by RFC 3986, Section 6.2.3,
	 *
	 * @see fixTrailingSlashAfterPath
	 * 
	 * and by  and by RFC 2616, Section 3.2.3. :
	 * 
	 * @code
	 * >>> i = BaseURLParser('http://abc.com:80/~smith/home.html')
	 * >>> j = BaseURLParser('http://ABC.com/%7Esmith/home.html')
	 * >>> k = BaseURLParser('http://ABC.com:/%7esmith/home.html')
	 * >>> i == j == k
	 * True
	 * @endcode
	 * 
	 * But for absolute URIs a defined path, the last "/" does produce
	 * different URIs
	 * 
	 * @code
	 * >>> u = BaseURLParser("http://www.exemple.com/foo")
	 * >>> v = BaseURLParser("http://www.exemple.com/foo/")
	 * >>> u != v
	 * True
	 * @endcode
	 * 
	 * Dot segments ("./..///a/b/../c" -> /a/c) and Percent-encoding
	 * normalization is handled here as well.
	 * 
	 * Observe: empty URLs doesn't mean undefined paths, but empty paths!
	 * This is per algorithm from RFC 3986, Section 5.3
	 */
	void readPath(std::string& path);

	//@}
	
	//!@name Aux. Functions
	bool hasScheme() const {return ! scheme.empty();}

	/**Verifies if this URL has the a authority section "defined".
	 *
	 * Actually, all it does is verify if it has a non-empty
	 * host(name), as this is the only non-optional part of authority
	 */
	bool hasAuthority() const {return ! host.empty();}

	bool hasQuery() const { return ! query.empty(); }



public:
	/**@name URL components
	 * Each component has an attribute that stores it's string
	 * representation and another that determines if it is defined
	 * or not.
	 */
	//@{
        std::string scheme;

        std::string userinfo;

        std::string host;

        std::string port;

        std::string path;

        std::string query;

        std::string fragment;
	//@}
	

	/**Default constructor.
	 *
	 * By default, this URL defaults to "", i.e., and empty
	 * path.
	 *
	 * @param _url The string representation of the URL.
	 *
	 */
	BaseURLParser(const std::string _url="");

	BaseURLParser(const filebuf _buf);

	/**Copy constructor.
	 *
	 * Copy assignment is handled automatically by the compiler, i.e.,
	 * an atribute by atribute copy.
	 */
	BaseURLParser(const BaseURLParser& other);

	BaseURLParser& operator=(const BaseURLParser& other);

	
	/**Parses the URL.
	 *
	 * @warning You should NOT call this method! It is called by
	 * BaseURLParser constructor. Any further call to this method
	 * will raise a ParserEOFError.
	 */
	void parse();

	bool operator==(const BaseURLParser& other) const;

	bool operator!=(const BaseURLParser& other) const
	{ return ! (*this == other);}


	/**Return reference URL R converted into a target URL T using this
	 * instance as base URL.
	 * 
	 * We follow RFC 3986, Section 5.2.2 procedures.
	 */
	BaseURLParser operator+(const BaseURLParser& R) const;

	bool operator<(const BaseURLParser& R) const;

	std::string getScheme() const {return this->scheme;}
	std::string getPath() const {return this->path;}

        //! Notice that even if this URL has an authority component it still is a
        //! relative URL per RFC 3986.
	bool isRelative() const { return !(this->hasScheme()); }

	/**Recompose the parsed URI elements from this URL into a string.
	 *
	 * We follow the algorithm from RFC 3986, Section 5.3
	 */
	std::string str() const;

	/**Remove userinfo, query and fragment components from a URL.
	 * 
	 * Return a reference of the URL, with the above fragments
	 * stripped.
	 */
	BaseURLParser& strip();


};

/* **********************************************************************

    def isDynamic(self):
        return self.query is not None

    def needsAuthentication(self):
        return self.userinfo is not None

    def getComponents(self):
        return ( self.scheme, self.userinfo, self.host, self.port, self.path,\
                self.query, self.fragment )


            

    # Syntax Components (S. 3) Rules ##########################################




       

    def _readQuery(self):
        """Reads a query component, if present."""
        query = self._readUntilDelimiter(u'#') 
        if query:
            return query[1:]    # get past the 
        else:
            return None

    def _readFragment(self):
        """Read a fragment component, if present."""
        fragment = self._text[self._start: self._end + 1]
        if fragment:
            return fragment[1:]
        else:
            return None


 * ********************************************************************** */

 

#endif // __URLTOOLS_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
