#include "urltools.h"
#include "explode.h"


BaseURLParser::BaseURLParser(const std::string _url):
	BaseParser( filebuf(_url.c_str(), _url.size()) ),
	scheme(""), 
	userinfo(""),
	host(""),
	port(""),
	path(""), path_defined(false), // FIXME
	query(""), query_defined(false),
	fragment(""), fragment_defined(false)
{
	parse();
}

BaseURLParser::BaseURLParser(const filebuf _buf) : 
	BaseParser( _buf ),
	scheme(""), 
	userinfo(""), 
	host(""),
	port(""),
	path(""), path_defined(false), // FIXME
	query(""), query_defined(false),
	fragment(""), fragment_defined(false)
{
	parse();
}

BaseURLParser::BaseURLParser(const BaseURLParser& other):
	BaseParser(), // Nothing to read, URL already parsed!
	scheme(other.scheme), 
	userinfo(other.userinfo), 
	host(other.host),
	port(other.port),
	path(other.path), path_defined(other.path_defined), // FIXME
	query(other.query), query_defined(other.query_defined),
	fragment(other.fragment), fragment_defined(other.fragment_defined)
{} // Nothing to read, URL already parsed!


bool BaseURLParser::operator==(const BaseURLParser& other) const
{
	return this->scheme == other.scheme && 
		this->userinfo == other.userinfo && 
		this->host == other.host && 
		this->port == other.port && 
		this->path_defined == other.path_defined && 
		this->path == other.path && 
		this->query_defined == other.query_defined && 
		this->query == other.query && 
		this->fragment_defined == other.fragment_defined && 
		this->fragment == other.fragment;

}


/*
 *   URI-reference = URI / relative-ref    S 4.1
 * 
 *   Section 3
 *   URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
 * 
 *   hier-part   = "//" authority path-abempty
 * 	      / path-absolute
 * 	      / path-rootless
 * 	      / path-empty
 * 
 *   Section 4.2
 *   relative-ref  = relative-part [ "?" query ] [ "#" fragment ]
 * 
 *   relative-part = "//" authority path-abempty
 * 	    / path-absolute
 * 	    / path-noscheme
 * 	    / path-empty
 */
void BaseURLParser::parse()
{
        try {
            scheme = readScheme();
            if ( hasScheme() and this->scheme != "http" ) {
                    throw NotSupportedSchemeException(
		    		"We only deal with plain HTTP.");
	    }
            
            // Now, what follows? Authority?
            readAuthority(this->userinfo, this->host, this->port);
/*

            self.path = self._readPath()
            # Paths are taken to be not empty when part of authority is set
            if self.path == u'' and self.host is not None:
                self.path = u'/'

            self.query = self._readQuery()
            self.fragment = self._readFragment()
 */
        } catch (ParserEOFError) {
            // If there isn't what to read, there is not what to do either
	}
}


BaseURLParser& BaseURLParser::operator+(const BaseURLParser& other)
{
	return *this;
}

std::string BaseURLParser::readScheme()
{
	// scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
	std::string empty_scheme = "";
	std::string scheme;
	const char* start = this->text.current;
	int length = 0;

	try {
		checkForEOF();

		// Read scheme's first char (ALPHA)
		if (not is_in(*text, LETTERS)){
			return empty_scheme;
		}
		++text; // go to next char in scheme
		++length;
		// Find the end of this scheme
		while ( (not text.eof()) 
			and is_in( *text, LETTERS + DIGITS + "+-.") )
		{
			++text;
			++length;
		}

		// Finaly, check for the ':', if present...
		if ( (not text.eof()) and *text == ':' ){
			consumeToken(":");
			scheme = std::string(start,length);
			to_lower(scheme);
			return scheme;
		} else{
			// This is not a scheme!
			// Call the exception handler
			throw ParserEOFError("while reading a URL scheme.");
		}
	} catch (ParserEOFError) {
		// If anything went wrong then is because
		// this is not a valid scheme

		// get back to previous position
		text.current = start;
		return empty_scheme;
	}
        
        // Parsing restart after the end of this rule
}


void BaseURLParser::readAuthority(std::string& userinfo, std::string& host,
				  std::string& port)
{
	filebuf previous_start(this->text); // Just in case we need to go back
	std::string authority;
	std::string hostport;
	std::vector<std::string> res_split;


	// authority = [ userinfo "@" ] host [ ":" port ]


	// Clear userinfo, host and port information
	userinfo.clear();
	host.clear();
	port.clear();

	try {
		consumeToken("//");

		// The authority component is preceded by a double slash ("//")
		// and is terminated by the next slash ("/"), question mark ("?"),
		// or number sign ("#") character, or by the end of the URI.
		authority = this->readUntilDelimiter("/?//").str();

		// Parse userinfo
		if ( authority.find("@") != authority.npos ) {
			res_split = split(authority,"@",1);
			userinfo = res_split[0];
			hostport = res_split[1];
		} else {
			hostport = authority;
		}

		// Parse host and port
		if (hostport.find(":") != hostport.npos) {
			res_split = split(hostport,":",1);
			host = res_split[0];
			port = res_split[1];
			// Clean port if it is equal to default http port
			normalizeDefaultHTTPPort();
		}else {
			host = hostport;
		}
		// check if the hostname is valid according to RFC 1034 rules
		validateHostName(host);
		validatePort(port);

		// Now we know they are valid, normalize hostname
		normalizeHostName(host);

	} catch (InvalidCharError) {
		// Oops! This doesn't seem like an authority section...
		// Rollback any move we made
		text.current = previous_start.current;
	}
	// Done!
	return;
}

    

void BaseURLParser::validateHostName(const std::string& hostname)
{
	std::string::const_iterator c;

	if (hostname.empty()) {
		throw InvalidURLException(
		      std::string("Empty hostname in an authority section: '") +
				this->text.str() + "'");
	}
	for( c = hostname.begin(); c != hostname.end(); ++c) {
		if ( not is_in(*c, DOMAINNAME_CHARS) ) {
			throw InvalidURLException(
				std::string("Invalid character '") +
				*c + "' in hostname '"+ hostname +
				"'");
		}
	}
}

void BaseURLParser::validatePort(const std::string& port)
{
	std::string::const_iterator c;

	for( c = port.begin(); c != port.end(); ++c) {
		if ( not is_in(*c, DIGITS) ) {
			throw InvalidURLException(
				std::string("Invalid character '") +
				*c + "' in port number '"+ port +
				"'");
		}
	}
}



// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
