import sys
# -*- coding: utf-8 -*-

# Assumptions
#
# * KISS
# * Text is read in a single pass.
# * We will try to be as i18n and unicode-aware as possible, but we know that
#   our aim is latin1 and that most of our content is in west-european
#   languages.
# * For every rule-parsing function: after it's execution, the parser state
#   should be ready do process the NEXT rule. So, _start should be updated
#   accordingly
#
# FIXME tag-end is not being handled properly


import string

# This constants have the same meaning as the corresponding rules of the
# XML Specification, Section 2.3 Common Syntatic Constructs
#

LETTERS = unicode(string.ascii_letters)
DIGITS = unicode(string.digits)
WHITESPACE = unicode(string.whitespace)

NAME_START_CHARS = LETTERS + u'_:'
NAME_CHARS = NAME_START_CHARS + u'.-' + DIGITS

# Things that should not be in the start of a tag-like construct
TAGLIKE_START_INVALID_CHARS = u"<&>"


class ParsingError (Exception):
    """Something unexpected happened while parsing the text content.
    
    This is the root of all parsing errors"""

class InvalidCharError (ParsingError):
    """An error ocurred while parsing a non-terminal rule.
    
    Users should not really see this error as any well-formness errors and
    "unexpected char" errors that ocurr in a tag-like element promote this
    tag-like into a text data element."""


class BaseParser(object):
    
    def __init__(self, text=''):
        self._text = unicode(text)
        self._start = 0
        self._end = len(text) - 1

    def parse(self):
        while self._start <= self._end:
            character = self._text[ self._start ]
            if character == '<':
                self._readTagLike()
            else:
                self._readText()


    # Callbacks - Must be implemented
    def handleText(self, text):
        pass

    def handleStartTag(self, tag_name, attrs={}):
        pass

    def handleGenericTag(self):
        pass

    # Private Functions
    # #########################################################################

    # Commom Syntatic Constructs (S 2.3) Rules ################################
    
    def _readName(self):
        """Reads a 'name', almost according to the XML specification.

        See XML Specification, Section 2.3 Common Syntatic Constructs.
        """
        i = self._start
        if not (self._text[i] in NAME_START_CHARS or self._text[i].isalpha()):
            raise InvalidCharError("Error parsing 'Name' rule.")
        i += 1 # go to next char in name
        # Find the end of this name
        while i <= self._end and \
            (self._text[i] in NAME_CHARS or self._text[i].isalpha()):
                i += 1
        name = self._text[ self._start : i ]
        # Parsing restart after the end of this rule
        self._start = i

        return name

    def _readSpace(self,optional=True):
        """Reads a 'S' rule, as close as possible to the XML specification.
        
        Returns True if any space was read/consumed."""
        i = self._start
        found = False
        if (not optional) and (not self._text[i].isspace()):
            raise InvalidCharError("Error parsing 'Name' rule.")
        # Find the end of this space
        while i <= self._end and self._text[i].isspace():
                i += 1
                found = True
        # Parsing restart after the end of this rule
        # in this case, if may be in the same place it started...
        self._start = i

        return found

    def _readUntilDelimiter(self, delimiters):
        """Returns whatever exists until the one of the delimiters is found.
        
        @param delimiters A list with all the delimiters. It can be a single
                          string (each character is a delimiter) or a a list
                          with strings, each string being a delimiter.
        """
        i = self._start
        while i <= self._end and (self._text[i] not in delimiters):
            i += 1

        data = self._text[self._start: i]
        # Parsing restart after the end of this rule
        self._start = i

        return data

    def _readAttValue(self):
        """Reads a AttValue rule and a possible preceding Eq rule.
        
        Observe that we deal here with HTML-styled attributes, so """
        # [25] Eq	    ::= S? '=' S?
        # [10] AttValue	::= '"' ([^<&"] | Reference)* '"'
		#	             |  "'" ([^<&'] | Reference)* "'"
        previous_start = self._start # Just in case we need to go back
        value = None
        self._readSpace()
        if self._text[self._start] != u'=':
            # plain HTML attribute with no value
            self._start = previous_start # Get back
            value = None
        else:
            self._start += 1    # Get past the '='
            self._readSpace()
            if self._text[self._start] == u"'":
                self._start += 1    # Get past the first '
                value =  self._readUntilDelimiter(u"'")
                self._start += 1    # Get past the end '
            elif self._text[self._start] == u'"':
                self._start += 1    # Get past the first "
                value =  self._readUntilDelimiter(u'"')
                self._start += 1    # Get past the end "
            else:
                # Old HTML-style attribute
                value =  self._readUntilDelimiter(WHITESPACE + u">")
        return value

    def _readAttributeList(self):
        """Reas a (S Attribute)* rule"""
        attrs = {}
        while self._readSpace() and self._text[self._start] not in u'/>':
            name = self._readName()
            val = self._readAttValue()
            attrs[name] = val
        return attrs


    # Element (Tags, PI, Data) Constructs (S 2.4+) Rules #######################
    
    def _readText(self):
        """Reads text until the start of something that *seems* like a tag."""
        # XXX   in the future, this should handle PCDATA, IIRC, it means it
        #       should handle entities and such. Ignore it for BaseParser
        i = self._start
        while i <= self._end and \
            (self._text[i] != '<' or not self._tagFollows(i)):
                i = i + 1
        # We may have reached the end or found a possible tag start
        # in any case text[start:i] has all that matter for us -- nothing
        # more, nothing less.
        self.handleText( self._text[self._start : i] )
         
        # Parsing should re-start at current position
        self._start = i

    def _tagFollows(self,start=None):
        """Is the next thing to be read a tag?
        
        @param start Position in text where we should test if there is a
                     tag-like content starting.
        """
        if not start:
            start = self._start
        next_pos = start + 1
        end = self._end
        data = self._text
        # After the < there MUST be a character that is not a white-space,
        # not a &, not another < and not a >
        # Besides, there must be at least a third character to form the
        # smallest tag possible
        if next_pos > end or data[next_pos].isspace() or \
           data[next_pos] in TAGLIKE_START_INVALID_CHARS:
                # not a tag, actually
                return False
        return True

    def _readTagLike(self):
        """Read and parse a tag-like content.
        
        If it starts with a <, and is not followed by space or & characters
        then it is assuped to be a tag-like content.
        """
        i = self._start
        next_pos = i + 1
        next = None

        # Is this really  a tag-like content?
        if not self._tagFollows():
            # not a tag, actually, just text...
            self.handleText( self._text[i:i +1])
            self._start += 1
            return

        # Bellow here things start to get weird. We should do as a normal
        # grammar-based parser would...
        next = self._text[next_pos]
        
        if next.isalpha():
            # Start-Tag, section 3.1
            self._readStartTag()
        else:
            self._readGenericTagConstruction()
        

    def _readStartTag(self):
        """Reads a Start-Tag construction.
        
        Parsing restarts after the tag's closing ">"."""
        # See Section 3.1 from XML Specification
        # [40] STag ::= '<' Name (S  Attribute)* S? '>'
        name = None
        attrs = {}

        self._start += 1 # go past the '<'
        name = self._readName()
        attrs = self._readAttributeList()
        self._readSpace()

        i = self._start
        while i <= self._end and self._text[i] != '>':
            i = i + 1

        # just get the text between < and >
        #self.handleStartTag(name, {self._text[ self._start + 1: i] : None})
        self.handleStartTag(name, attrs)

        # parsing should restart after the >
        self._start = i + 1

    def _readGenericTagConstruction(self):
        """Reads a generic construction.
        
        Parsing restarts after the tag's closing ">"."""
        # See Section 3.1 from XML Specification
        # [40] STag ::= '<' Name (S  Attribute)* S? '>'
        i = self._start + 1 # go past the '>'
        while i <= self._end and self._text[i] != '>':
            i = i + 1

        # just get the text between < and >
        self.handleStartTag( self._text[ self._start + 1: i])

        # parsing should restart after the >
        self._start = i + 1




class TestParser(BaseParser):
    """Simple Test class for the parser.

    The aim of this class is to be simple fixure upon which unittests can be
    easly build. For now, doctests are being used.

    >>> TestParser("a c ").parse().items
    [('TEXT', u'a c ')]

    >>> TestParser("a <> c ").parse().items
    [('TEXT', u'a <> c ')]

    >>> TestParser("a<b>c").parse().items
    [('TEXT', u'a'), ('TAG', u'b', {}), ('TEXT', u'c')]

    >>> TestParser('#<a duplas="x"'+" simples='what' html=antigo attrhtml />#").parse().items == [('TEXT', u'#'), ('TAG', u'a', {u'duplas': u'x', u'simples': u'what', u'html': u'antigo', u'attrhtml': None }), ('TEXT', u'#')]
    True

    >>> TestParser("a<b style=").parse().items
    [('TEXT', u'a <b style=')]
    """

    #>>> TestParser('#<a&whatever duplas="x"'+" simples='what' html=antigo attrhtml />#").parse().items
    #[('TEXT', u'#'), ('TAG', u'a&whatever', {u'duplas': u'x', u'simples': u'what', u'html': u'antigo', u'attrhtml': None }), ('TEXT', u'#')]
    def __init__(self,text):
        # setup base class
        super(TestParser,self).__init__(text)
        self.items = []

    def parse(self):
        super(TestParser,self).parse()
        return self
        
    def handleText(self,text):
        self.items.append(("TEXT",text))

    def handleStartTag(self, tag_name, attrs={}):
        self.items.append(("TAG",tag_name,attrs))



def read_tag(data,start,length):
    end = start
    next = start + 1
    # After the < there MUST be a character that is not a white-space, not a &
    # and not a >
    if (start + 1) >= length or data[next].isspace() or data[next] == u'&' :
        # not a tag, actually
        got_text(data[start:end + 1])
        return end + 1
    while end < length and data[end] != '>':
        end = end + 1

    got_tag(data[start:end +1])

    return end # includes tag closing '>'

def read_text(data,start,length):
    end = start
    while end < length and data[end] != '<':
        end = end + 1

    end = end - 1 # we don't include '<'

    got_text(data[start:end])

    return end


def parse(data):
    i = 0
    size =  len(data)
    end = i
    while i < size:
        character = data[i]
        if character == '<':
            end = read_tag(data,i,size)
        else:
            end = read_text(data,i,size)

        i = end +1


def process(filename):
    data = open(filename,'r').read()
    parse(data)

def _test():
    import doctest
    doctest.testmod()


if __name__ == '__main__':
    if len(sys.argv) > 1:
        process(sys.argv[1])
    else:

        _test()

        data = u"asdajsdh\tAsasd.ASdasd < ajk>dajkaXXX<dh title=''>XXX \x00\xa4 "
        parser = TestParser(data)
        parser.parse()
        print "Original stream:", data
        for content in parser.items:
            print content





# vim:sts=4:ts=4:et:sw=4:ai:fo+=tcroq:fileencoding=utf-8:
