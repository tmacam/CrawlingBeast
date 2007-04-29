#include "urltools.h"
#include "explode.h"
#include <deque>
#include <iomanip>


BaseURLParser::BaseURLParser(const std::string _url):
	BaseParser( filebuf(_url.c_str(), _url.size()) ),
	scheme(""), 
	userinfo(""),
	host(""),
	port(""),
	path(""),
	query(""),
	fragment("")
{
	parse();
}

BaseURLParser::BaseURLParser(const filebuf _buf) : 
	BaseParser( _buf ),
	scheme(""), 
	userinfo(""), 
	host(""),
	port(""),
	path(""),
	query(""),
	fragment("")
{
	parse();
}

BaseURLParser::BaseURLParser(const BaseURLParser& other):
	BaseParser(), // Nothing to read, URL already parsed!
	scheme(other.scheme), 
	userinfo(other.userinfo), 
	host(other.host),
	port(other.port),
	path(other.path),
	query(other.query),
	fragment(other.fragment)
{} // Nothing to read, URL already parsed!

BaseURLParser& BaseURLParser::operator=(const BaseURLParser& other)
{
	this->scheme = other.scheme ; 
	this->userinfo = other.userinfo ; 
	this->host = other.host ; 
	this->port = other.port ; 
	this->path = other.path ; 
	this->query = other.query ; 
	this->fragment = other.fragment;

	return *this;

}


bool BaseURLParser::operator==(const BaseURLParser& other) const
{
	return this->scheme == other.scheme && 
		this->userinfo == other.userinfo && 
		this->host == other.host && 
		this->port == other.port && 
		this->path == other.path && 
		this->query == other.query && 
		this->fragment == other.fragment;

}

bool BaseURLParser::operator<(const BaseURLParser& other) const
{
	return this->scheme < other.scheme || 
		this->userinfo < other.userinfo || 
		this->host < other.host || 
		this->port < other.port || 
		this->path < other.path || 
		this->query < other.query || 
		this->fragment < other.fragment;

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

            readPath(this->path);
            // Paths are taken to be not empty when part of authority is set
	    normalizeTrailingSlashAfterAuthority();

/*
            self.query = self._readQuery()
            self.fragment = self._readFragment()
 */
        } catch (ParserEOFError) {
            // If there isn't what to read, there is not what to do either
	}
}


BaseURLParser BaseURLParser::operator+(const BaseURLParser& R) const
{
	BaseURLParser T = BaseURLParser();
	const BaseURLParser& Base = *this;

	if ( R.hasScheme() ) {
		// This is an absolute URL already!
		T.scheme    = R.scheme;
		// authority
		T.userinfo = R.userinfo;
		T.host = R.host;
		T.port = R.port;
		// the rest of the url...
		T.path      = removeDotSegments(R.path);
		T.query     = R.query;
	} else {
		if ( R.hasAuthority() ) { 
			// Authority...
			T.userinfo = R.userinfo;
			T.host = R.host;
			T.port = R.port;
			// The rest...
			T.path      = removeDotSegments(R.path);
			T.query     = R.query;
		} else {
			if ( R.path.empty() ){
				T.path = Base.path;
				if ( R.hasQuery() ) {
					T.query = R.query;
				} else {
					T.query = Base.query;
				}
			} else {
				if (R.path.size() > 0 && R.path[0] == '/') {
					T.path = removeDotSegments(R.path);
				} else {
					T.path = mergePath(Base, R);
					// we  MUST call remove_dot_segments
					// again...
					T.path = removeDotSegments(T.path);
				}
				T.query = R.query;
			}//endif R.path
			// T.authority = Base.authority
			T.userinfo = Base.userinfo;
			T.host = Base.host;
			T.port = Base.port;
		} // endif R.authority
		T.scheme = Base.scheme;
	}// endif R.scheme
	T.fragment = R.fragment;

	return T;
}

 
std::string BaseURLParser::mergePath(const BaseURLParser& base, const BaseURLParser& rel)
{
	std::string::size_type rightmost;

        if (base.hasAuthority() and (base.path.empty() or base.path == "/") ) {
            return std::string("/") + rel.path;
        } else {
            rightmost = base.path.rfind("/");
            if  ( rightmost == base.path.npos ) {
		// No "/" found!
		//
                // This code should never be reached: ALL URIs have a
                // at least "/" as path! This is automatically added
		// during parsing, just after readPath
                return rel.path;
            } else {
		// rightmost "/" included in the substring
                return base.path.substr(0,rightmost +1) + rel.path;
	    }
	}
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


void BaseURLParser::readPath(std::string& path)
{
        path = readUntilDelimiter("?#").str();
        path = removeDotSegments(path);

	/* We cannot fix percent encoding before
	 * removing dot-segments.
	 *
	 * For instance, after fixing-PE
	 *
	 *   .../foo%2Fbar/... becames  .../foo/bar/...
	 *
	 * But, in reallity, what the user requested
	 * was a file inside a directory named "foo/bar".
	 * In most OS there cannot be such a directory, as the
	 * "/" is reserved as directory delimiter.
	 *
	 * At least, this is apache's behaviour, and is the one
	 * we are following here.
	 */
        path = fixPercentEncoding(path);

	// Quite a small method, ain't?
}

std::string BaseURLParser::removeDotSegments(const std::string& path)
{
        // NOTE: The RFC algorithm is writen with a string/buffer in mind
        // while here we deal with an list of segments (created by split("/")).
        // This introduce some peculiarities to our implementation.
        // This is particularly important to have in mind, specially when
        // dealing with absolute dirs, i.e, paths that start with '/'.
        // In our case, absolute dirs are identified by an empty
        // string in the first position of a list of segments.
        // 
        // Adding an element to the output segment list automatically adds a
        // preceding "/" to it's element.  as well. The only case were you
        // should realy take care is when adding/removing segments from/to an
        // absolute paths in the input buffer.
	typedef std::deque<std::string> buffer_t; 
        
	buffer_t input = split<buffer_t>(path, "/");
        buffer_t output;

	while ( ! input.empty() ){
		// Rule 'A'
		// If the input buffer begins with a prefix of "../" or "./"
		if ( (input.size() > 1) and (input[0] == ".." or input[0] == ".") ){
			// hen remove that prefix from the input buffer
			input.pop_front();
			continue;
		// Rule 'B'
		// if the input buffer begins with a prefix of "/./" or "/.",
		} else if ( input.size() > 1 and input[0] == "" and input[1] == "." ){
			//then replace that prefix with "/" in the input buffer
			// Python: input = [u''] + input[2:]
			input.pop_front();
			input.pop_front();
			input.push_front("");
			continue;
		// Rule 'C'
		// if the input buffer begins with a prefix of "/../" or "/.."
		} else if ( input.size() > 1 and  input[0] == ""
				and input[1] == ".." )
		{
			// then replace that prefix with "/" in the input buffer
			// Python: input = [u''] + input[2:]
			input.pop_front();
			input.pop_front();
			input.push_front("");
			// ... and remove the last segment and its preceding "/"
			// (if any) from the output buffer
			if ( not output.empty() ){ output.pop_back();}
			// ...remove the preceding "/"
			//    (only if this was the last segment)
			if ( (not output.empty()) and
					(output[output.size() -1] == ""))
			{
				output.pop_back();
			}
			continue;
		// Rule 'D'
		// if the input buffer consists only of "." or ".."
		} else if ( (input.size() == 1 ) and 
				(input[0] == ".." or input[0] == ".") ) 
		{
			// remove that from the input buffer
			input.pop_back();
			continue;
		// RULE 'D-E': Our extra rule
		} else if ( input.size() > 1 and input[0] == "" and input[1] == ""){
			// We may end up with "//" in the input string. Convert
			// it to a single "/"
			input.pop_front();
			continue;
		// Rule 'E'
		} else {
			// Move the first path segment in the input buffer to the end of
			// the output buffer, including the initial "/" character (if
			// any) and any subsequent characters up to, but not including,
			// the next "/" character or the end of the input buffer
			if (input.size() > 1 and input[0] == "") {
				// If output buffer is still empty, this is an absolute
				// path and a initial "/" must be added to output
				if (output.empty()) {
					output.push_back("");
				}
				// Remove the first segment, including it's  "/"
				output.push_back( input[1] );
				// Python: del input[1]; del input[0]
				input.pop_front();
				input.pop_front();
			} else {
				output.push_back( input[0] );
				input.pop_front();
			}
			// Now, if there still is something left in the input,
			// add the initial "/" back
			if (not input.empty() and input[0] != "") {
				input.push_front("");
			}
			continue;
		}
	}

        return join("/",output);
}

std::string BaseURLParser::charToPE(char _c)
{
	unsigned char c = _c;
	int value = c;

	std::ostringstream out;

	// Meu deus! Que diabo prolixo!!!!! Pro inferno com essa linguagem!
	out << "%" << std::uppercase << std::hex << std::setw(2) << 
		std::setfill('0') << value;
	return out.str();
}


std::string BaseURLParser::decodeAndFixPE(const std::string pe_data)
{
	char c;
	unsigned int value;


	// Validate string
	if (pe_data.size() != 3 and pe_data[0] != '%') {
		throw std::runtime_error(
			"Unexpected error in decodeAndFixPE\n");
	}
	std::string value_str = pe_data.substr(1);
	if ( not isxdigit(value_str[0]) or not isxdigit(value_str[1]) ) {
		throw InvalidURLException("Invalid percent-encoded data '" +
				pe_data + "'");
	}

	std::istringstream in(value_str);
	in >> std::hex >> value;
	c = value;

	if ( (value < 128) and  is_in(c,UNRESERVED) ){
		return std::string(1,c);
	} else {
		// anything that is not UNRESERVED should be percent-encoded
		return charToPE(c);
	}
}


std::string BaseURLParser::fixPercentEncoding(const std::string path)
{
	std::string fixed;
	std::string::size_type i = 0;
	char c;


	while ( i < path.size() ) {
		c = path[i];
		if (c == '%') {
			// percent encoded data?
			if ( i + 2 < path.size() ) {
				fixed += decodeAndFixPE(path.substr(i,3)); // %XX
				i += 3;
			} else {
				// Invalid data? Just pass the rest as-is
				fixed += path.substr(i);
				i = path.size();
			}
		} else if ( is_in(c, RESERVED) or is_in(c,UNRESERVED) ){
			fixed += c;
			i += 1;
		} else { 
			// non-ascii is always percent-encoded
			// Anything that is not in RESERVED or in UNRESERVED
			// Should also be percent encoded
			fixed += charToPE(c);
			i += 1;
		}
	}
	return fixed;
}

std::string BaseURLParser::str() const
{
	//FIXME Test me

	std::string result = "";

	if ( hasScheme() ) {
		result += scheme;
		result += ":";
	}

	if ( hasAuthority() ) {
		result += "//";
		if ( not userinfo.empty() ) {
			result += userinfo;
			result += "@";
		}
		result += host;
		if (not port.empty() ) {
			result += ":";
			result += port;
		}
	}

	result += path;

	if (not query.empty()) {
		result += "?";
		result += query;
	}

	if (not  fragment.empty()) {
		result += "#";
		result += fragment;
	}

	return result;
}





// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
