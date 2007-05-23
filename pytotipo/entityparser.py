import sys
# -*- coding: utf-8 -*-

import string
from parser import  BaseParser, ParsingError, ParserEOFError, InvalidCharError,\
                    LETTERS, DIGITS

HEXDIGITS = unicode(string.hexdigits)

HTML_ENTITIES_UTF8 = {
	"nbsp" : "\xc2\xa0",
	"iexcl" : "\xc2\xa1",
	"cent" : "\xc2\xa2",
	"pound" : "\xc2\xa3",
	"curren" : "\xc2\xa4",
	"yen" : "\xc2\xa5",
	"brvbar" : "\xc2\xa6",
	"sect" : "\xc2\xa7",
	"uml" : "\xc2\xa8",
	"copy" : "\xc2\xa9",
	"ordf" : "\xc2\xaa",
	"laquo" : "\xc2\xab",
	"not" : "\xc2\xac",
	"shy" : "\xc2\xad",
	"reg" : "\xc2\xae",
	"macr" : "\xc2\xaf",
	"deg" : "\xc2\xb0",
	"plusmn" : "\xc2\xb1",
	"sup2" : "\xc2\xb2",
	"sup3" : "\xc2\xb3",
	"acute" : "\xc2\xb4",
	"micro" : "\xc2\xb5",
	"para" : "\xc2\xb6",
	"middot" : "\xc2\xb7",
	"cedil" : "\xc2\xb8",
	"sup1" : "\xc2\xb9",
	"ordm" : "\xc2\xba",
	"raquo" : "\xc2\xbb",
	"frac14" : "\xc2\xbc",
	"frac12" : "\xc2\xbd",
	"frac34" : "\xc2\xbe",
	"iquest" : "\xc2\xbf",
	"Agrave" : "\xc3\x80",
	"Aacute" : "\xc3\x81",
	"Acirc" : "\xc3\x82",
	"Atilde" : "\xc3\x83",
	"Auml" : "\xc3\x84",
	"Aring" : "\xc3\x85",
	"AElig" : "\xc3\x86",
	"Ccedil" : "\xc3\x87",
	"Egrave" : "\xc3\x88",
	"Eacute" : "\xc3\x89",
	"Ecirc" : "\xc3\x8a",
	"Euml" : "\xc3\x8b",
	"Igrave" : "\xc3\x8c",
	"Iacute" : "\xc3\x8d",
	"Icirc" : "\xc3\x8e",
	"Iuml" : "\xc3\x8f",
	"ETH" : "\xc3\x90",
	"Ntilde" : "\xc3\x91",
	"Ograve" : "\xc3\x92",
	"Oacute" : "\xc3\x93",
	"Ocirc" : "\xc3\x94",
	"Otilde" : "\xc3\x95",
	"Ouml" : "\xc3\x96",
	"times" : "\xc3\x97",
	"Oslash" : "\xc3\x98",
	"Ugrave" : "\xc3\x99",
	"Uacute" : "\xc3\x9a",
	"Ucirc" : "\xc3\x9b",
	"Uuml" : "\xc3\x9c",
	"Yacute" : "\xc3\x9d",
	"THORN" : "\xc3\x9e",
	"szlig" : "\xc3\x9f",
	"agrave" : "\xc3\xa0",
	"aacute" : "\xc3\xa1",
	"acirc" : "\xc3\xa2",
	"atilde" : "\xc3\xa3",
	"auml" : "\xc3\xa4",
	"aring" : "\xc3\xa5",
	"aelig" : "\xc3\xa6",
	"ccedil" : "\xc3\xa7",
	"egrave" : "\xc3\xa8",
	"eacute" : "\xc3\xa9",
	"ecirc" : "\xc3\xaa",
	"euml" : "\xc3\xab",
	"igrave" : "\xc3\xac",
	"iacute" : "\xc3\xad",
	"icirc" : "\xc3\xae",
	"iuml" : "\xc3\xaf",
	"eth" : "\xc3\xb0",
	"ntilde" : "\xc3\xb1",
	"ograve" : "\xc3\xb2",
	"oacute" : "\xc3\xb3",
	"ocirc" : "\xc3\xb4",
	"otilde" : "\xc3\xb5",
	"ouml" : "\xc3\xb6",
	"divide" : "\xc3\xb7",
	"oslash" : "\xc3\xb8",
	"ugrave" : "\xc3\xb9",
	"uacute" : "\xc3\xba",
	"ucirc" : "\xc3\xbb",
	"uuml" : "\xc3\xbc",
	"yacute" : "\xc3\xbd",
	"thorn" : "\xc3\xbe",
	"yuml" : "\xc3\xbf",
	"fnof" : "\xc6\x92",
	"Alpha" : "\xce\x91",
	"Beta" : "\xce\x92",
	"Gamma" : "\xce\x93",
	"Delta" : "\xce\x94",
	"Epsilon" : "\xce\x95",
	"Zeta" : "\xce\x96",
	"Eta" : "\xce\x97",
	"Theta" : "\xce\x98",
	"Iota" : "\xce\x99",
	"Kappa" : "\xce\x9a",
	"Lambda" : "\xce\x9b",
	"Mu" : "\xce\x9c",
	"Nu" : "\xce\x9d",
	"Xi" : "\xce\x9e",
	"Omicron" : "\xce\x9f",
	"Pi" : "\xce\xa0",
	"Rho" : "\xce\xa1",
	"Sigma" : "\xce\xa3",
	"Tau" : "\xce\xa4",
	"Upsilon" : "\xce\xa5",
	"Phi" : "\xce\xa6",
	"Chi" : "\xce\xa7",
	"Psi" : "\xce\xa8",
	"Omega" : "\xce\xa9",
	"alpha" : "\xce\xb1",
	"beta" : "\xce\xb2",
	"gamma" : "\xce\xb3",
	"delta" : "\xce\xb4",
	"epsilon" : "\xce\xb5",
	"zeta" : "\xce\xb6",
	"eta" : "\xce\xb7",
	"theta" : "\xce\xb8",
	"iota" : "\xce\xb9",
	"kappa" : "\xce\xba",
	"lambda" : "\xce\xbb",
	"mu" : "\xce\xbc",
	"nu" : "\xce\xbd",
	"xi" : "\xce\xbe",
	"omicron" : "\xce\xbf",
	"pi" : "\xcf\x80",
	"rho" : "\xcf\x81",
	"sigmaf" : "\xcf\x82",
	"sigma" : "\xcf\x83",
	"tau" : "\xcf\x84",
	"upsilon" : "\xcf\x85",
	"phi" : "\xcf\x86",
	"chi" : "\xcf\x87",
	"psi" : "\xcf\x88",
	"omega" : "\xcf\x89",
	"thetasym" : "\xcf\x91",
	"upsih" : "\xcf\x92",
	"piv" : "\xcf\x96",
	"bull" : "\xe2\x80\xa2",
	"hellip" : "\xe2\x80\xa6",
	"prime" : "\xe2\x80\xb2",
	"Prime" : "\xe2\x80\xb3",
	"oline" : "\xe2\x80\xbe",
	"frasl" : "\xe2\x81\x84",
	"weierp" : "\xe2\x84\x98",
	"image" : "\xe2\x84\x91",
	"real" : "\xe2\x84\x9c",
	"trade" : "\xe2\x84\xa2",
	"alefsym" : "\xe2\x84\xb5",
	"larr" : "\xe2\x86\x90",
	"uarr" : "\xe2\x86\x91",
	"rarr" : "\xe2\x86\x92",
	"darr" : "\xe2\x86\x93",
	"harr" : "\xe2\x86\x94",
	"crarr" : "\xe2\x86\xb5",
	"lArr" : "\xe2\x87\x90",
	"uArr" : "\xe2\x87\x91",
	"rArr" : "\xe2\x87\x92",
	"dArr" : "\xe2\x87\x93",
	"hArr" : "\xe2\x87\x94",
	"forall" : "\xe2\x88\x80",
	"part" : "\xe2\x88\x82",
	"exist" : "\xe2\x88\x83",
	"empty" : "\xe2\x88\x85",
	"nabla" : "\xe2\x88\x87",
	"isin" : "\xe2\x88\x88",
	"notin" : "\xe2\x88\x89",
	"ni" : "\xe2\x88\x8b",
	"prod" : "\xe2\x88\x8f",
	"sum" : "\xe2\x88\x91",
	"minus" : "\xe2\x88\x92",
	"lowast" : "\xe2\x88\x97",
	"radic" : "\xe2\x88\x9a",
	"prop" : "\xe2\x88\x9d",
	"infin" : "\xe2\x88\x9e",
	"ang" : "\xe2\x88\xa0",
	"and" : "\xe2\x88\xa7",
	"or" : "\xe2\x88\xa8",
	"cap" : "\xe2\x88\xa9",
	"cup" : "\xe2\x88\xaa",
	"int" : "\xe2\x88\xab",
	"there4" : "\xe2\x88\xb4",
	"sim" : "\xe2\x88\xbc",
	"cong" : "\xe2\x89\x85",
	"asymp" : "\xe2\x89\x88",
	"ne" : "\xe2\x89\xa0",
	"equiv" : "\xe2\x89\xa1",
	"le" : "\xe2\x89\xa4",
	"ge" : "\xe2\x89\xa5",
	"sub" : "\xe2\x8a\x82",
	"sup" : "\xe2\x8a\x83",
	"nsub" : "\xe2\x8a\x84",
	"sube" : "\xe2\x8a\x86",
	"supe" : "\xe2\x8a\x87",
	"oplus" : "\xe2\x8a\x95",
	"otimes" : "\xe2\x8a\x97",
	"perp" : "\xe2\x8a\xa5",
	"sdot" : "\xe2\x8b\x85",
	"lceil" : "\xe2\x8c\x88",
	"rceil" : "\xe2\x8c\x89",
	"lfloor" : "\xe2\x8c\x8a",
	"rfloor" : "\xe2\x8c\x8b",
	"lang" : "\xe2\x8c\xa9",
	"rang" : "\xe2\x8c\xaa",
	"loz" : "\xe2\x97\x8a",
	"spades" : "\xe2\x99\xa0",
	"clubs" : "\xe2\x99\xa3",
	"hearts" : "\xe2\x99\xa5",
	"diams" : "\xe2\x99\xa6",
	"quot" : "\x22",
	"amp" : "\x26",
	"lt" : "\x3c",
	"gt" : "\x3e",
	"OElig" : "\xc5\x92",
	"oelig" : "\xc5\x93",
	"Scaron" : "\xc5\xa0",
	"scaron" : "\xc5\xa1",
	"Yuml" : "\xc5\xb8",
	"circ" : "\xcb\x86",
	"tilde" : "\xcb\x9c",
	"ensp" : "\xe2\x80\x82",
	"emsp" : "\xe2\x80\x83",
	"thinsp" : "\xe2\x80\x89",
	"zwnj" : "\xe2\x80\x8c",
	"zwj" : "\xe2\x80\x8d",
	"lrm" : "\xe2\x80\x8e",
	"rlm" : "\xe2\x80\x8f",
	"ndash" : "\xe2\x80\x93",
	"mdash" : "\xe2\x80\x94",
	"lsquo" : "\xe2\x80\x98",
	"rsquo" : "\xe2\x80\x99",
	"sbquo" : "\xe2\x80\x9a",
	"ldquo" : "\xe2\x80\x9c",
	"rdquo" : "\xe2\x80\x9d",
	"bdquo" : "\xe2\x80\x9e",
	"dagger" : "\xe2\x80\xa0",
	"Dagger" : "\xe2\x80\xa1",
	"permil" : "\xe2\x80\xb0",
	"lsaquo" : "\xe2\x80\xb9",
	"rsaquo" : "\xe2\x80\xba",
	"euro" : "\xe2\x82\xac"
}

HTML_ENTITIES_UNICODE = {}
for k,v in HTML_ENTITIES_UTF8.items():
	HTML_ENTITIES_UNICODE[k] = unicode(v,"utf-8")

class UnknownEntityReferenceError(ParsingError):
    pass

class BaseEntityParser(BaseParser):
    """Base class for entity parsers.
    
    This entity parser tries to mimic the bahaviour of browsers like firefox
    and konkeror WRT the handling of xml-invalid entities.

    This base class provides a simple interface upon with more elaborated
    parsers can be built.
    """
    def __init__(self, text=u""):
        super(BaseEntityParser,self).__init__(unicode(text))

    def parse(self):
        while self._start <= self._end:
            character = self._text[ self._start ]
            if character == '&':
                self._readEntity()
            else:
                self._readPureText()
        return self

    def _readPureText(self):
        text = self._readUntilDelimiter("&")
        self.handlePureText(text)

    def _readEntity(self):
        previous_start = self._start
        try:
            self._consumeToken("&")
            self._checkForEOF() # there must be one character left
            c = self._text[self._start]

            if c == '#':
                self._readNumericEntity()
            elif c.isalpha():
                self._readEntityIdentifier()
            else:
                raise InvalidCharError(
                    "Invalid character '%s' in entity decoding" % c )

            # Consume trailing ';' if it is there...
            if self._start <= self._end and self._text[self._start] == u';':
                self._consumeToken(u';')

        except (InvalidCharError, UnknownEntityReferenceError):
            # Invalid or unexpected character found? Unknown entity?
            # Just turn everything we had from previous_start to _end into
            # text content.
            self.handlePureText( self._text[ previous_start : self._start])
        except ParserEOFError:
            self.handlePureText( self._text[ previous_start : self._end +1])

    def _readEntityIdentifier(self):
        # & was already read. Get the itentifier...
        i = self._start
        while i <= self._end and self._text[i] in (LETTERS + DIGITS):
            i += 1

        identifier = self._text[self._start : i]
        self._start = i

        if HTML_ENTITIES_UNICODE.has_key(identifier):
            self.handleEntity(HTML_ENTITIES_UNICODE[identifier])
        else:
            raise UnknownEntityReferenceError("Unknown entity identifier '%s'" %
                 identifier)


    def _readNumericEntity(self):
        # & was already read,
        # '#' is in the current position 
        self._consumeToken('#')
        # There MUST be something after this token
        self._checkForEOF()
        c = self._text[self._start]

        if c == 'x':
            # hexa number
            self._consumeToken('x')
            uni_val = self._readNumber(16)
        else:
            # commom number
            uni_val = self._readNumber(10)

        # Convert number into unicode
        self.handleEntity(unichr(uni_val))

    def _readNumber(self, base=10):
        """base = 10 or 16"""
        # Base setup
        if base == 10:
            ALLOWED_CHARS = DIGITS
        elif base == 16:
            ALLOWED_CHARS = HEXDIGITS
        else:
            raise Exception("Invalid base value for _readNumber")
        
        i = self._start
        while i <= self._end and self._text[i] in ALLOWED_CHARS:
            i = i + 1
        res = self._text[self._start: i]

        # Empty number?
        if len(res) == 0:
            raise InvalidCharError("Error while reading a number")
        self._start = i
        return int(res,base)

    def handlePureText(self, text):
        """Called when some simple text was found"""
        pass

    def handleEntity(self, entity_text):
        """Called when an entity is found and decoded."""
        pass


class EntityParser(BaseEntityParser):
    def __init__(self,text=u""):
        super(EntityParser,self).__init__(text)
        self.parsed_text = u""
        self.parse()

    def handlePureText(self,text):
        self.parsed_text += text

    def handleEntity(self,entity_text):
        self.parsed_text += entity_text

    def __str__(self):
        return self.parsed_text.encode('utf-8')

    def __unicode__(self):
        return self.parsed_text


def parseText(text):
    return unicode(EntityParser(text))


class TestURLParser(EntityParser):
    """Simple Test class for the entity parser.

    The aim of this class is to be simple fixure upon which unittests can be
    easly build. For now, doctests are being used.

    >>> parseText(u"")
    u''

    Simple text must pass as-is

    >>> parseText(u'this is an example text')
    u'this is an example text'

    Simple Entities too:

    >>> parseText(u'&lt;')
    u'<'

    Text with entities should pass as expected

    >>> parseText(u'd&lt;j&gt; vu')
    u'd<j> vu'

    Some of the possible ways to represent an á

    >>> parseText(u"\xe1") == u'\xe1'
    True
    >>> parseText(u"&aacute;") == u'\xe1'
    True
    >>> parseText(u"&#xe1;") == u'\xe1'
    True
    >>> parseText(u"&#225;") == u'\xe1'
    True


    Mozilla is very liberal in its entities parsing:

    - Missing ";" is accepted

    >>> parseText(u"&aacute&aacute;") == u'\xe1\xe1'
    True

    - Entities can be terminated by whitespace
    >>> parseText(u'&lt &lt &lt')
    u'< < <'

    - Entities can be closed by EOF

    >>> parseText(u"&aacute") == u'\xe1'
    True

    - Numeric entities can be terminated by non-numeric content
    >>> parseText(u'&#225bade') == u'\xe1bade'
    True

    Some invalid entities that must be interpreted as text

    >>> parseText(u'&')
    u'&'
    """
    pass

def _test():
    import doctest
    (fail, tests) = doctest.testmod()
    if not fail:
        print "All tests passed.", tests, "tests executed."


if __name__ == '__main__':
    _test()
    print "Done."





# vim:sts=4:ts=4:et:sw=4:ai:fo+=tcroq:fileencoding=utf-8:
