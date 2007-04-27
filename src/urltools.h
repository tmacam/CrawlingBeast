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

	//@}
	
	//!@name Aux. Functions
	bool hasScheme() {return scheme.size() != 0;}



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
        bool path_defined;

        std::string query;
        bool query_defined;

        std::string fragment;
        bool fragment_defined;
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

	
	/**Parses the URL.
	 *
	 * @warning You should NOT call this method! It is called by
	 * BaseURLParser constructor. Any further call to this method
	 * will raise a ParserEOFError.
	 */
	void parse();

	bool operator==(const BaseURLParser& other) const;


	BaseURLParser& operator+(const BaseURLParser& other);

	std::string getScheme(){return this->scheme;}


/* **********************************************************************



    def isRelative(self):
        # Notice that even if this URL has an authority component it still is a
        # relative URL
        return self.scheme is None

    def isDynamic(self):
        return self.query is not None

    def needsAuthentication(self):
        return self.userinfo is not None

    def getComponents(self):
        return ( self.scheme, self.userinfo, self.host, self.port, self.path,\
                self.query, self.fragment )

    def __eq__(self,b):
        return self.getComponents() == b.getComponents()

    def __add__(self,R):
        """Return reference URL R converted into a target URL T using this
        instance as base URL.
        
        We follow RFC 3986, Section 5.2.2 procedures.
        """
        T = BaseURLParser()
        Base = self
        if R.scheme:
            # This is an absolute URL already!
            T.scheme    = R.scheme
            # authority
            T.userinfo, T.host, T.port = Base.userinfo, Base.host, Base.port
            T.path      = self.removeDotSegments(R.path)
            T.query     = R.query
        else:
            if R.host: # This is the only non-opitional part of authority
                T.userinfo, T.host, T.port = R.userinfo, R.host, R.port
                T.path      = self.removeDotSegments(R.path);
                T.query     = R.query;
            else:
                if R.path == "":
                    T.path = Base.path;
                    if R.query is not None:
                        T.query = R.query
                    else:
                        T.query = Base.query
                else:
                   if R.path.startswith("/"):
                        T.path = self.removeDotSegments(R.path);
                   else:
                      T.path = self.mergePath(Base, R);
                      # we  MUST call remove_dot_segments again...
                      T.path = self.removeDotSegments(T.path);
                   T.query = R.query
                #endif R.path
                # T.authority = Base.authority
                T.userinfo, T.host, T.port = Base.userinfo, Base.host, Base.port
            # endif R.authority
            T.scheme = Base.scheme
        # endif R.scheme
        T.fragment = R.fragment

        return T

    def removeDotSegments(self,path):
        """Remove special "." and ".." from a path segment according to rules
        in RFC 3986, Section 5.2.4

        One extra rule was added between rules 2D and 2E: rule 2D-E.
        It does the following: s#^//#/#

        >>> u = BaseURLParser()
        >>> u.removeDotSegments("/a/b/c/./../../g")
        u'/a/g'
        >>> u.removeDotSegments("mid/content=5/../6")
        u'mid/6'
        >>> u.removeDotSegments("mid/content=5////../6")
        u'mid/6'
        """
        # NOTE: The RFC algorithm is writen with a string/buffer in mind
        # while here we deal with an list of segments (created by split("/")).
        # This introduce some peculiarities to our implementation.
        # This is particularly important to have in mind, specially when
        # dealing with absolute dirs, i.e, paths that start with '/'.
        # In our case, absolute dirs are identified by an empty
        # string in the first position of a list of segments.
        # 
        # Adding an element to the output segment list automatically adds a
        # preceding "/" to it's element.  as well. The only case were you
        # should realy take care is when adding/removing segments from/to an
        # absolute paths in the input buffer.
        #
        input = path.split('/')
        output = []
        #print "1", "\t-\t", "/".join(output), "\t-\t", "/".join(input)
        while len(input) > 0:
            # Rule 'A'
            if len(input) > 1 and input[0] in [u"..", u"."]:
                del input[0]
                #print "2A", "\t-\t", "/".join(output), "\t-\t", "/".join(input)
                continue
            # Rule 'B'
            elif len(input) > 1 and input[0:2] == [u'', u'.']:
                input = [u''] + input[2:]
                #print "2B", "\t-\t", "/".join(output), "\t-\t", "/".join(input), input
                continue
            # Rule 'C'
            # if the input buffer begins with a prefix of "/../" or "/.."
            elif len(input) > 1 and input[0:2] == [u'', u'..']:
                # then replace that prefix with "/" in the input buffer
                input = [u''] + input[2:]
                # ... and remove the last segment and its preceding "/"
                # (if any) from the output buffer
                output.pop()
                if len(output) and output[-1] == u"":
                    output.pop()
                #print "2C", "\t-\t", "/".join(output), "\t-\t", "/".join(input), "\t", input
                continue
            # Rule 'D'
            elif input == [u".."] or input == [u"."]:
                input.pop()
                #print "2D", "\t-\t", "/".join(output), "\t-\t", "/".join(input), "\t", input
                continue
            # RULE 'D-E': Our extra rule
            elif len(input) > 1 and input[:2] == [u"",u""]:
                # We may end up with "//" in the input string. Convert
                # it to a single "/"
                del input[0]
                #print "2DE", "\t-\t", "/".join(output), "\t-\t", "/".join(input), "\t", input
                continue
            # Rule 'E'
            else:
                # Move the first path segment in the input buffer to the end of
                # the output buffer, including the initial "/" character (if
                # any) and any subsequent characters up to, but not including,
                # the next "/" character or the end of the input buffer
                if len(input) > 1 and input[0] == u"":
                    # If output buffer is still empty, this is an absolute
                    # path and a initial "/" must be added to output
                    if len(output) == 0:
                        output.append(u"")
                    # Remove the first segment, including it's  "/"
                    output.append( input[1] )
                    del input[1]
                    del input[0]
                else:
                    output.append( input[0] )
                    del input[0]
                # Now, if there still is something left in the input,
                # add the initial "/" back
                if len(input) and input[0] != u"":
                    input.insert(0,u"")
                #print "2E", "\t-\t", "/".join(output), "\t-\t", "/".join(input), "\t", input
                continue

        #print "3", "\t-\t", "/".join(output), "\t-\t", "/".join(input), len(input)
        return u"/".join(output)


    def mergePath(self,base, rel):
        """Merges a relative-path URI to a base URI (base).

        @param base A URL instance with the Base URL
        @param rel  A URL instance with the relative-path URI
        
        We follow the algorithm from RFC 3986, Section 5.2.3.
        """
        if base.host and (base.path == u"" or base.path == u"/"):
            return u"/" + rel.path
        else:
            rightmost = base.path.rfind("/")
            if rightmost < 0:
                # This code should never be reached: ALL URIs have a
                # at least "/" as path! This is automatically added by
                # parse, just after readPath
                return rel.path
            else:
                return base.path[:rightmost] + "/" + rel.path
            

    def __str__(self):
        """Recompose the parsed URI elements from this URL into a string.
        
        We follow the algorithm from RFC 3986, Section 5.3
        """
        # FIXME Test me
        
        result = u""

        if self.scheme:
            result += self.scheme
            result += u":"
        
        if self.host or self.port:
            result += "//"
            if self.userinfo:
                result += self.userinfo    
                result += "@"
            result += self.host
            if self.port:
                result += ":"
                result += self.port

        result += self.path

        if self.query:
            result += u"?"
            result += self.query
        if self.fragment:
            result += u"#"
            result += self.fragment
        
        return result;

    # Syntax Components (S. 3) Rules ##########################################



    def _readPath(self):
        """Reads a path component.
        
        Empty paths in absolute URIs are handled by parse().
        
        Observe that http://www.exemple.com/ == http://www.exemple.com ,
        as inscructed by RFC 3986, Section 6.2.3,

        >>> u = BaseURLParser("http://www.exemple.com/")
        >>> v = BaseURLParser("http://www.exemple.com")
        >>> u == v
        True
        >>> v.path
        u'/'

        and by  and by RFC 2616, Section 3.2.3.

        >>> i = BaseURLParser('http://abc.com:80/~smith/home.html')
        >>> j = BaseURLParser('http://ABC.com/%7Esmith/home.html')
        >>> k = BaseURLParser('http://ABC.com:/%7esmith/home.html')
        >>> i == j == k
        True
        
        But for absolute URIs a defined path, the last "/" does produce
        different URIs

        >>> u = BaseURLParser("http://www.exemple.com/foo")
        >>> v = BaseURLParser("http://www.exemple.com/foo/")
        >>> u != v
        True
    
        Dot segments ("./..///a/b/../c" -> /a/c) and Percent-encoding
        normalization is handled here as well.

        Observe: empty URLs doesn't mean undefined paths, but empty paths!
        This is per algorithm from RFC 3986, Section 5.3
        """
        path = self._readUntilDelimiter(u"?#")
        path = self.removeDotSegments(path)
        path = self.fixPercentEncoding(path)

        # There is not such a thing as empty path
        if not path:
            path = u''

        return path

    def fixPercentEncoding(self,path):
        """

        >>> u = BaseURLParser(u'')
        >>> u.fixPercentEncoding(u'%41%42%43%61%62%63%25 é%e9%23%3F')
        u'ABCabc%25%20%C3%A9%E9%23%3F'
        """
        fixed = u''
        i = 0
        path_len = len(path)
        while i < path_len:
            c = path[i]
            if c == u'%':
                # percent encoded data?
                if i + 2 < path_len :
                    fixed += self.decodeAndFixPE(path[i:i+3]) # %XX
                    i += 3
                else:
                    # Invalid data? Just pass the rest as-is
                    fixed += path[i:]
                    i = path_len
            elif c in RESERVED or c in UNRESERVED:
                fixed += c
                i += 1
            else: 
                # non-ascii is always percent-encoded
                # Anything that is not in RESERVED or in UNRESERVED
                # Should also be percent encoded
                fixed += "%%%02X" % ord(c) 
                i += 1
        return fixed

    def decodeAndFixPE(self,pe_data):
        """Read a single Percent-Encoded character and parses it accordingly.

        >>> u = BaseURLParser('')
        >>> u.decodeAndFixPE("%20")
        '%20'
        >>> u.decodeAndFixPE("%24")
        '%24'
        >>> u.decodeAndFixPE("%5F")
        '_'
        """
        # Validate string
        value_str = pe_data[1:]
        if value_str[0] not in string.hexdigits or \
           value_str[1] not in string.hexdigits:
                raise InvalidCharError("Invalid percent-encoded data '%s'" % \
                                      pe_data)
        value = int(value_str,16)
        c=chr(value)
        if value < 128 and  c in UNRESERVED:
            return c
        else:
            # anything that is not UNRESERVED should be percent-encoded
            return "%%%02X" % value

        

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

 
};

#endif // __URLTOOLS_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq: