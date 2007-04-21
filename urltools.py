import sys
# -*- coding: utf-8 -*-

# Assumptions
#
# FIXME Establishing a Base URI, RFC 3986, Section 5.1
# FIXME ":server/" is supposed to be a valid URL?
# FIXME add test examples from http://www.mnot.net/python/urlnorm.py
# FIXME Max URL size per HTTP protocol specs. (RFC 2616)
# FIXME Perhaps normalization procedures should be moved from parsing, making
#       it easier to find what and where are we dealing with each issue related
#       to normalization...

# FIXME Verify if the following is already FIXed
# FIXME Percent-encoding RFC 3986, Section 2.1
# FIXME URL Normalization, RFC 3986, Sections 6.2.2 and 6.2.3


import string
from parser import  *

# This constants have the same meaning as the corresponding rules of the
# URI Generic Syntax, Section 2.2 Reserved Characters

GEN_DELIMS  = u":/?#[]@"
SUB_DELIMS  = u"!$&'()*+,;="
RESERVED    = GEN_DELIMS + SUB_DELIMS

# FIXME those should not be percent-encoded
UNRESERVED  = LETTERS + DIGITS +  u"-._~"


class NotSupportedSchemeException(Exception):
    """Our URL parser is rather limited. If it doesn't implement a certain funcionalyty, if just fails :-)"""

class BaseURLParser(BaseParser):
    """ Simple and limited URL Parser and Normalizer.

    This parser is really limited by the scope of it's use:
      * It can only handle HTTP shemas - since our crawler only does HTTP.
      * It fails to parse URLs with query components - since our craler ignore
        those links.
      * It fails if the authority part of the URL has user and password
        information - we don't crawl pages that require auth, so no point
        in parsing them either.

    Despite those limitations, our parser does URL normalization on absolute and
    on relative URLs. Normalization happens durring parsing an during the merge
    of URIs.

    We also assume URL data is in UTF-8.


         foo://example.com:8042/over/there?name=ferret#nose
         \_/   \______________/\_________/ \_________/ \__/
          |           |            |            |        |
       scheme     authority       path        query   fragment
    """
    def __init__(self, url=u""):
        super(BaseURLParser,self).__init__(url)

        # Setup instance variables 
        self.scheme = None
        self.userinfo = None
        self.host = None
        self.port = None
        self.path = ''
        self.query = None
        self.fragment = None

        self.parse()

    def parse(self):
        """Parses an URL
          URI-reference = URI / relative-ref    S 4.1

          Section 3
          URI         = scheme ":" hier-part [ "?" query ] [ "#" fragment ]

          hier-part   = "//" authority path-abempty
                      / path-absolute
                      / path-rootless
                      / path-empty

          Section 4.2
          relative-ref  = relative-part [ "?" query ] [ "#" fragment ]

          relative-part = "//" authority path-abempty
                    / path-absolute
                    / path-noscheme
                    / path-empty
        """
        try:
            self.scheme = self._readScheme()
            if self.scheme:
                if self.scheme != u'http':
                    raise NotSupportedSchemeException("We only deal with plain HTTP.")
            
            # Now, what follows? Authority?
            self.userinfo, self.host, self.port = self._readAuthority()

            self.path = self._readPath()
            # Paths are taken to be not empty when part of authority is set
            if self.path == u'' and self.host is not None:
                self.path = u'/'

            self.query = self._readQuery()
            self.fragment = self._readFragment()
        except ParserEOFError:
            # If there isn't what to read, there is not what to do either
            pass

        return self


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
    
    def _readScheme(self):
        """Reads the 'scheme' component of a URL, if present.

        If a scheme component is not found or if the one found is invalid,
        returns None.

        @warning We don't raise InvalidCharError on bad schemes.

        See URI Generic Syntax, Section 3.1, Scheme
        """
        # scheme = ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
        scheme = None
        self._checkForEOF()
        i = self._start
        if not (self._text[i] in LETTERS):
            return None
        i += 1 # go to next char in scheme
        # Find the end of this scheme
        while i <= self._end and (self._text[i] in LETTERS + DIGITS + u"+-."):
                i += 1
        # Finaly, check for the ':', if present...
        if i <= self._end and self._text[i] == ':' :
            scheme = self._text[ self._start : i ].lower()
            self._start = i + 1
        else:
            # This is not a valid scheme...
            scheme = None
        
        # Parsing restart after the end of this rule
        return scheme

    def _readAuthority(self):
        """Reads an authority component, if present.

        Authority component is preceeded by "//", and we expect to see
        those characters on the current reading position.
        
        @warning We don't validate host and port information.

        >>> u = BaseURLParser("http:// invalid.url:80/")
        InvalidUrlException()

        We DON'T support international URLs
        >>> u = BaseURLParser("http://www.ficções.net/biblioteca_conto/")
        InvalidUrlException()
        """
        # authority = [ userinfo "@" ] host [ ":" port ]
        userinfo, host, port = None, None, None
        previous_start = self._start
        try:
            self._consumeToken("//")
            # The authority component is preceded by a double slash ("//")
            # and is terminated by the next slash ("/"), question mark ("?"),
            # or number sign ("#") character, or by the end of the URI.
            authority = self._readUntilDelimiter(u"/?#")
            # Parse userinfo
            if authority.count(u'@'):
                userinfo, hostport = authority.split(u"@",1)
            else:
                hostport = authority
            # Parser host and port
            if hostport.count(u':'):
                host, port = hostport.split(u":",1)
                # Clean port if it is equal to default http port
                if self.scheme == u'http' and (port == u'80' or port == u''):
                    port = None
            else:
                host = hostport
            host = host.lower()
            if host[-1] == '.':
                host = host[:-1]
        except InvalidCharError:
            # Oops! This doesn't seem like an authority section...
            # Rollback any move we made
            self._start = previous_start
        return (userinfo, host, port)

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




class TestURLParser(BaseURLParser):
    """Simple Test class for the parser.

    The aim of this class is to be simple fixure upon which unittests can be
    easly build. For now, doctests are being used.

    >>> u = TestURLParser("http://user:pass@Exemple.com.:80/f/file?query#frag")
    >>> u.scheme
    u'http'
    >>> u.userinfo
    u'user:pass'
    >>> u.host
    u'exemple.com'
    >>> u.port is None
    True
    >>> u.path
    u'/f/file'
    >>> u.query
    u'query'
    >>> u.fragment
    u'frag'

    # 6.2.3.  Scheme-Based Normalization
    >>> TestURLParser("http://www.Exemple.com.:80/") == TestURLParser("HTTP://WWW.exemple.com")
    True

    >>> TestURLParser('http://example.com') == TestURLParser('http://example.com/') == TestURLParser('http://example.com:/') == TestURLParser('http://example.com:80/')
    True

    >>> import operator
    >>> list_of_relatives = ['/p4', 'p4/', '../blah/..', './isso.html', 'file.html']
    >>> relative_urls = [TestURLParser(u) for u in list_of_relatives]
    >>> reduce(operator.and_, [u.isRelative() for u in relative_urls] )
    True
    >>> [u.path for u in relative_urls]
    [u'/p4', u'p4/', u'', u'isso.html', u'file.html']

    >>> absolute = TestURLParser("http://a.com/base/index.html")
    >>> [str(absolute + u) for u in relative_urls]
    ['http://a.com/p4', 'http://a.com/base/p4/', 'http://a.com/base/index.html', 'http://a.com/base/isso.html', 'http://a.com/base/file.html']

    >>> TestURLParser("http://www.exemple.com") != TestURLParser("http://exemple.com")
    True

    # 6.2.4.  Protocol-Based Normalization
    >>> TestURLParser("http://www.exemple.com/sub/") != TestURLParser("http://www.exemple.com/sub")
    True

    >>> TestURLParser("http://www.exemple.com/?") != TestURLParser("http://www.exemple.com/")
    True

    >>> TestURLParser(u"http://a.com/%7e é %41") == TestURLParser(u"http://a.com/~%20%C3%A9%20A")
    True

    >>> u = BaseURLParser("http://AeXAMPLE/a/./b/../b/%63/%7bfoo%7d")
    >>> v = BaseURLParser("http://aexample://a/b/c/%7Bfoo%7D")
    >>> u == v
    True
    """
    pass

def _test():
    import doctest
    doctest.testmod()


if __name__ == '__main__':
    _test()
    print "Done."





# vim:sts=4:ts=4:et:sw=4:ai:fo+=tcroq:fileencoding=utf-8:
