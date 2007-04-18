import sys
# -*- coding: utf-8 -*-
"""A simple, almost stupid conversor of webdata to unicode.


Inspired in UnicodeDammit, from BeautifuSoup.
"""


import string
import codecs
from parser import SloppyHtmlParser

class CannotFindSuitableEncodingException(Exception):
    """The name says it all. We gave up."""

class CharsetDetectedException(Exception):
    """Not really an error, just used to stop parsing and notify that an
    indicative encoding was found."""
    pass

class FindEncParser(SloppyHtmlParser):
    def __init__(self,data):
        super(FindEncParser,self).__init__(data)
        self.enc = None

    def parse(self):
        try:
            super(FindEncParser,self).parse()
        except CharsetDetectedException:
            pass

    def handleProcessingInstruction(self,name,attrs):
        if name == 'xml' and 'encoding' in attrs:
            self.enc = attrs['encoding'].strip()
            raise CharsetDetectedException("Found in a Processing Instruction")

    def handleStartTag(self, tag_name,attrs, empty_element_tag):
        self.safeHandleStartTag(tag_name, attrs, empty_element_tag)
        if self.skipTagIfTroublesome(tag_name, empty_element_tag):
            self.handleEndTag(tag_name)

    def safeHandleStartTag(self,name,attrs,empty):
        if name == 'meta' and \
           attrs.get('http-equiv','').lower() =='content-type':
                content = attrs.get('content','').lower()
                content = content.lower()
                if 'charset' in content:
                    charset = content[content.find('charset'):]
                    charset = charset[charset.find('=') + 1:]
                    self.enc = charset.strip()
                    raise CharsetDetectedException("Found in a Meta Tag")


    def getEnc(self):
        return self.enc
            


# Used for BOM/XML charset recognition
XML_MARKS = (
    ('utf-32LE','\x3c\x00\x00\x00'),
    ('utf-32BE','\x00\x00\x00\x3c'),
    ('utf-16LE','\x3c\x00\x3f\x00'),
    ('utf-16BE','\x00\x3c\x00\x3f'),
)

BOM_MARKS = ( 
    ('utf-8',   '\xef\xbb\xbf'),
    ('utf-32LE','\xff\xfe\x00\x00'),
    ('utf-32BE','\x00\x00\xfe\xff'),
    ('utf-16LE','\xff\xfe'),
    ('utf-16BE','\xfe\xff'),
)

ALL_MARKS = XML_MARKS + BOM_MARKS




class UnicodeBugger(object):
    """Convers streams to UTF-8
    """
    # We read up to MAX_META_POS characters in data to find an indicative
    # character encoding (In XML header of in a HTML Meta Tag
    MAX_META_POS = 1024

    def __init__(self,data,suggested_encodings=[]):
        """
        @param suggested_encodings List of suggested encodings to use,
                                   in order of precedence.
        """
        self.data = data
        self.suggested_encodings = suggested_encodings
        self.encoding = None
        self.tried_encodings = set()

    def convert(self):
        u = None
        data = self.data
        if isinstance(data,unicode):
            # No need to do anything
            return data

        # Try the proposed encodings
        for e in self.suggested_encodings:
            u = self._convertFrom(e)
            if u:
                return u
        
        # Ok, try to find the BOM or XML header...
        u = self._detectEncoding()
        if u:
            return u

        # Damn! Does this thing has a XML header or HTML Meta tag w/ encoding?
        max_pos = self.MAX_META_POS
        if len(data) < max_pos:
            max_pos = len(data)
        # latin1 is a good guess anyway, and won't complain in most cases
        # about bad char. conversions
        e = FindEncParser(unicode(data[:max_pos],'latin1','ignore'))
        try:
            e.parse()
        except Exception:
            pass
        if e.getEnc():
            u = self._convertFrom(e.getEnc())
            if u:
                return u

        # we are out options here! Last options: utf-8 and latin1
        for e in ['utf-8', 'latin1']:
            u = self._convertFrom(e)
            if u:
                return u

        raise CannotFindSuitableEncodingException("Giving up")


    def _convertFrom(self,encoding):
        u = None
        # Why bother trying all over again?
        if encoding not in self.tried_encodings:
            try:
                self.tried_encodings.add(encoding)
                u = unicode(self.data,encoding)
                self.encoding = encoding
            except UnicodeDecodeError:
                pass
        return u

    def _detectEncoding(self):
        """Uses XML and Unicode BOM heuristics to find charset."""
        length = len(self.data)
        u = None
        for enc, mark in ALL_MARKS:
            if length > len(mark) and data.startswith(mark):
                u = self._convertFrom(enc)
                if u:
                    break
        return u

        


def _test():
    import doctest
    doctest.testmod()


if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1 and sys.argv[1] != '-v':
        for filename in sys.argv[1:] :
            data = open(filename,'r').read()
            u = UnicodeBugger(data)
            u.convert()
            print "The encoding of", filename, "is", u.encoding
    else:
        _test()
        print "Done."






# vim:sts=4:ts=4:et:sw=4:ai:fo+=tcroq:fileencoding=utf-8:
