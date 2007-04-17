import sys
# -*- coding: utf-8 -*-

# Assumptions
#
# FIXME Percent-encoding RFC 3986, Section 2.1
# FIXME Establishing a Base URI, RFC 3986, Section 5.1
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


class NotSupported(Exception):
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

    Despite thos limitations, our parser does URL normalization on absolute and
    on relative URLs.

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
        self.path = None
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
                    raise NotSupported("We only deal with plain HTTP.")
            
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
        if R.scheme is not None:
            # This is an absolute URL already!
            T.scheme    = R.scheme
            # authority
            T.userinfo, T.host, T.port = Base.userinfo, Base.host, Base.port
            # T.path      = remove_dot_segments(R.path)
            # remove_dot_segments was called during path's parsing,
            # in _readPath
            T.query     = R.query
        else:
            if (R.userinfo is not None) or (R.host is not None) or \
               (R.port is not None):
                    T.userinfo, T.host, T.port = R.userinfo, R.host, R.port
                    # T.path      = remove_dot_segments(R.path);
                    # remove_dot_segments was called during path's parsing,
                    # in _readPath
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
                        #T.path = remove_dot_segments(R.path);
                        # remove_dot_segments was called during path's parsing,
                        # in _readPath
                        pass
                   else:
                      T.path = self.mergePath(Base.path, R.path);
                      # we  MUST call remove_dot_segments again...
                      T.path = self.removeDotSegments(T.path);
                   T.query = R.query
                #endif R.path
                T.userinfo, T.host, T.port = Base.userinfo, Base.host, Base.port
            # endif R.authority
            T.scheme = Base.scheme
        # endif R.scheme
        T.fragment = R.fragment

        return T

    def removeDotSegments(self,path):
        """Remove special "." and ".." from a path segment according to rules
        in RFC 3986, Section 5.2.4

        >>> u = BaseURLParser()
        >>> u.removeDotSegments("/a/b/c/./../../g")
        u'/a/g'
        >>> u.removeDotSegments("mid/content=5/../6")
        u'mid/6'
        """
        input = path.split('/')
        output = []
        print "1", output, input
        while len(input) > 0:
            # Rule 'A'
            if len(input) > 1 and input[0] in [u"..", u"."]:
                input = input[1:]
                print "2A", output, input
                continue
            # Rule 'B'
            elif len(input) > 1 and input[0:2] == [u'', u'.']:
                input = [u''] + input[2:]
                print "2B", output, input
                continue
            # Rule 'C'
            elif len(input) > 1 and input[0:2] == [u'', u'..']:
                input = [u''] + input[2:]
                output.pop()
                print "2C", output, input
                continue
            # Rule 'D'
            if input == [u".."] or input == [u"."]:
                input.pop()
                print "2D", output, input
                continue
            # Rule 'E' FIXME FIXME FIXME FIXME FIXME FIXME FIXME
            if input[0] == u'' and len(output) == 0 : # remove initial "/"
                output.append( input[0] )
                del input[0]
            if len(input):
                output.append( input[0] )
                del input[0]
            if len(input): # still got segments? then add the initial "/" back
                input.insert(0, u"")
            print "2E", output, input
            continue
        return u"/".join(output)





        raise NotImplementedError()

    def mergePath(self,base, ref):
        raise NotImplementedError()

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
                if port == u'80' or port == u'':
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
        """Reads a path component."""
        # FIXME remove_dot_segments
        path = self._readUntilDelimiter(u"?#")
        #FIXME relative->absolute (__add__) should deal with it
        #FIXME if not path:
        #FIXME    path = '/'
        return self.fixPercentEncoding(path)

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
    >>> [u.path for u in relative_urls] == list_of_relatives
    True
    >>> absolute = TestURLParser("http://a.com/index.html")
    >>> [absolute + u for u in relative_urls]
    []

    >>> TestURLParser("http://www.exemple.com") != TestURLParser("http://exemple.com")
    True

    # 6.2.4.  Protocol-Based Normalization
    >>> TestURLParser("http://www.exemple.com/sub/") != TestURLParser("http://www.exemple.com/sub")
    True

    >>> TestURLParser("http://www.exemple.com/?") != TestURLParser("http://www.exemple.com/")
    True

    >>> TestURLParser(u"http://a.com/%7e é %41") == TestURLParser(u"http://a.com/~%20%C3%A9%20A")
    True
    """
    pass

def _test():
    import doctest
    doctest.testmod()


if __name__ == '__main__':
    _test()




# vim:sts=4:ts=4:et:sw=4:ai:fo+=tcroq:fileencoding=utf-8:
