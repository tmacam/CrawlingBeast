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
# FIXME missing: <!DOCBOOK, <![CDATA ]]>


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

class ParserEOFError(ParsingError):
    """We unexpectedly found the end of the parsing data."""

class AbstractParser(object):
    """Abstract definition of our parser"""
    # Callbacks - Must be implemented
    def handleText(self, text):
        pass

    def handleStartTag(self, tag_name, attrs={}, empty_element_tag=False):
        pass

    def handleEndTag(self,tag_name):
        pass

    def handleProcessingInstruction(self, text):
        pass

    def handleComment(self,comment):
        pass


class BaseParser(AbstractParser):
    """A simple, almost stupid non-validating (x)HTML push parser.

    Handling Validation Errors
    ==========================

    Errors found during tag processing "promote" that thought-to-be-tag
    content into text content.
    """
    
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
        return self



    # Private Functions
    # #########################################################################

    # This-ought-to-have-a-tokenizer aux. Functions ###########################

    def _consumeToken(self,token):
        """Verifies that the current reading is valid and equal to token.

        After callig this function the reading position is located one
        character after last demilimiter mark character.
        
        EOF-ness is not verified at the end of the function.

        @param token The string you expect to be present in the current
                     reading pos.
        """
        for c in token:
            self._checkForEOF()
            if c != self._text[ self._start ]:
                raise InvalidCharError("Expected '%s' but found '%s'" % \
                                       (c, self._text[ self._start ]))
            self._start += 1

    # FIXME Do we really need this function as it is today? Does it really
    # FIXME need to call _checkForEOF at it's end?
    def _advanceReadingPosition(self):
        """Advance current reading position in text by one character.

        This function should be used when you must advance the reading position
        and read content immediately after. This situation usually arises
        during rule processing, where a given token is expected and consumed
        and immediately after you should validate the existence of/check for
        some other token.
        
        @warning May raise ParserEOFError if we go past the end of the buffer.
        """
        self._start += 1
        self._checkForEOF()

    def _checkForEOF(self,msg=""):
        if self._start > self._end:
            raise ParserEOFError(msg)


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
        """Reads a 'S*' rule, as close as possible to the XML specification.
        
        Returns True if any space was read/consumed. After calling this
        function the current reading position should be on a non-space
        character.

        @warning This function calls _checkForEOF"""
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
        self._checkForEOF()

        return found

    def _readUntilDelimiter(self, delimiters):
        """Returns whatever exists until the one of the delimiters is found.

        After callig this function the reading position is advanced to the
        position of the first delimiter found.
        
        @param delimiters A list with all the delimiters. It can be a single
                          string (each character is a delimiter) or a a list
                          with single character strings, each being a delimiter.
        """
        i = self._start
        while i <= self._end and (self._text[i] not in delimiters):
            i += 1

        data = self._text[self._start: i]
        # Parsing restart after the end of this rule
        self._start = i

        return data
    
    def _readUntilDelimiterMark(self,mark):
        """Returns whatever exists until the delimiter mark is found.

        After callig this function the reading position is located one
        character AFTER last demilimiter mark character.
        
        @param delimiter A string.

        @returns the text found until (but not cotaining) the delimiter
        """
        text_end = self._start
        where = self._text.find(mark, self._start)
        if where == -1:
            # Not found?
            raise ParserEOFError("While looking for delimiter mark '%s'" % mark)
        else:
            text_end = where + len(mark)
            
        data = self._text[self._start: where]
        # Parsing restart after the mark
        self._start = text_end

        return data

    def _readUntilEndTag(self,tag_name):
        """Returns whatever exists until a end-tag w/ name tag_name is found.

        After callig this function the reading position is located one
        character AFTER the EndTag construct.
        
        Notice:
            * Tag matching is case insensitive.
            * You will lose an EndTag event for this tag.
        """
        # [42] ETag ::= '</' Name S? '>'
        found = False
        while not found:
            self._readUntilDelimiterMark("</")
            i = self._start 
            self._start += len(tag_name)
            self._checkForEOF("While looking for the EndTag for " + tag_name)
            if self._text[i:i+len(tag_name)].lower() != tag_name.lower():
                # This is not the tag we are looking for...
                # Keep looking...
                continue
            self._readSpace() #_checkForEOF was called for us already
            if self._text[self._start] != u'>':
                raise ParserEOFError( "While looking for the EndTag for " +\
                                      tag_name )
            self._consumeToken('>')
            # Yeah! The EndTag was found. We are done
            found = True
            

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
            self._consumeToken('=') # Get past the '='
            self._readSpace()
            if self._text[self._start] == u"'":
                token = u"'"
                self._consumeToken(token)  # Get past the first '
                value =  self._readUntilDelimiter(token)
                self._consumeToken(token)    # Get past the end '
            elif self._text[self._start] == u'"':
                token = u'"'
                self._consumeToken(token)    # Get past the first "
                value =  self._readUntilDelimiter(token)
                self._consumeToken(token)    # Get past the end "
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

        Errors during tag-like-content decoding "promote" the tag-like to 
        text content.
        """
        previous_start = self._start
        next_pos = self._start + 1
        next = None

        # Is this really  a tag-like content?
        if not self._tagFollows():
            # not a tag, actually, just text... 
            self.handleText( self._text[ self._start : self._start +1])
            self._start += 1
            return

        # Bellow here things start to get weird. We should do as a normal
        # grammar-based parser would...
        # _tagFollows garantees this is a valid reading position
        next = self._text[next_pos]
        
        try:
            if next.isalpha():
                # Start-Tag, section 3.1
                self._readStartTag()
            elif next == u'/':
                # End-Tag, section 3.1
                self._readEndTag()
            elif next == u'?':
                # Processing Instruction, section 2.6
                self._readProcessingInstructions()
            elif next == u'!':
                # let's hope is a Comment
                if self._text[next_pos:].startswith(u"!--"):
                    self._readComment()
                else:
                    # FIXME
                    self._readGenericTagConstruction()
            else:
                self._readGenericTagConstruction()
        except ParserEOFError:
            # EOF found before we could finish parsing this tag-like content?
            # Just turn everything we had from previous_start to _end into
            # text content.
            self.handleText( self._text[ previous_start : self._end +1])
        except InvalidCharError:
            # InvalidCharError found?
            # Just turn everything we had from previous_start to _end into
            # text content.
            self.handleText( self._text[ previous_start : self._end +1])
            # Go past the offending character
            self._start += 1
        

    def _readStartTag(self):
        """Reads a Start-Tag construction.
        
        Parsing restarts after the tag's closing ">"."""
        # See Section 3.1 from XML Specification
        # [40] STag ::= '<' Name (S  Attribute)* S? '>'
        # [44] EmptyElemTag ::= '<' Name (S  Attribute)* S? '/>'
        name = None
        attrs = {}
        empty_element_tag = False

        self._consumeToken('<') # go past the '<'
        name = self._readName()
        attrs = self._readAttributeList()
        self._readSpace()

        # Is this an empty element tag?
        i = self._start # _readSpace already did a _checkForEOF for us...
        if self._text[i] == '/':
            empty_element_tag = True
            self._consumeToken('/')  # Go past the '/'
        self._consumeToken('>')  # Go past the >

        self.handleStartTag(name, attrs, empty_element_tag)
        if empty_element_tag:
            self.handleEndTag(name)

        # we are already past the '>', so there is no need to update _start
        # parsing should restart after the >

    
    def _readEndTag(self):
        """Reads an End-Tag construction.
        
        Parsing restarts after the tag's closing ">"."""
        # [42] ETag ::= '</' Name  S? '>'
        self._consumeToken('</') # go past the '</'
        name = self._readName()
        self._readSpace()
        # Mozilla just ignores whatever comes after the 'Name S?' sequence
        # but before a '>' Let's just do the same!
        self._readUntilDelimiter(">")
        self._consumeToken('>')  # go past the '>'
        
        self.handleEndTag(name)
        # we are already past the '>', so there is no need to update _start
        # parsing should restart after the >

    def _readProcessingInstructions(self):
        """Reads a Processing Instruction construction.
        
        Parsing restarts after the tag's closing ">"."""
        # [16] PI ::=  '<?' PITarget (S (Char* - (Char* '?>' Char*)))? '?>'
        content = None
        self._consumeToken('<?') # go past the '<?'
        content = self._readUntilDelimiterMark("?>")
        # we are already past the '?>' mark. No need to update _start
        self.handleProcessingInstruction(content)

    def _readComment(self):
        """Reads a Comment.
        
        Parsing restarts after the tag's closing ">"."""
        # [15] Comment ::= '<!--' ((Char - '-') | ('-' (Char - '-')))* '-->'
        content = None
        self._consumeToken('<!--') # go past the '<'
        content = self._readUntilDelimiterMark("-->")
        # we are already past the '-->' mark. No need to update _start
        self.handleComment(content)


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

    >>> TestParser("<b>").parse().items
    [('TAG', u'b', {})]

    >>> TestParser('#<a duplas="x y"'+" simples='what is that' html=antigo attrhtml />#").parse().items == [('TEXT', u'#'), ('TAG', u'a', {u'duplas': u'x y', u'simples': u'what is that', u'html': u'antigo', u'attrhtml': None }), ('ENDTAG', u'a'), ('TEXT', u'#')]
    True

    >>> TestParser("a<b style=").parse().items
    [('TEXT', u'a'), ('TEXT', u'<b style=')]
    
    >>> TestParser("a<b style=b ").parse().items
    [('TEXT', u'a'), ('TEXT', u'<b style=b ')]

    # EndTag 
    #
    # Mozilla just ignores whatever comes after the 'Name S?' sequence
    # but before a '>' Let's just do the same!
    >>> TestParser("a<b style='>'></b 1 asd asd asd>").parse().items
    [('TEXT', u'a'), ('TAG', u'b', {u'style': u'>'}), ('ENDTAG', u'b')]

    
    >>> TestParser("a <? xml blah ?><!-- <b> --> c").parse().items
    [('TEXT', u'a '), ('PI', u' xml blah '), ('COMMENT', u' <b> '), ('TEXT', u' c')]

    >>> TestParser("a<b style=b 1").parse().items
    [('TEXT', u'a'), ('TEXT', u'<b style=b 1')]
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

    def handleStartTag(self, tag_name, attrs={}, empty_element_tag=False):
        self.items.append(("TAG",tag_name,attrs))

    def handleEndTag(self,tag_name):
        self.items.append(("ENDTAG",tag_name))

    def handleProcessingInstruction(self,text):
        self.items.append(("PI",text))

    def handleComment(self,comment):
        self.items.append(("COMMENT",comment))


class SloppyHtmlParser(BaseParser):
    """Our first try into a HTML parser that treats style and script
    correctly.
    """
    TROUBLESOME_TAGS = [u'script', u'style']

    def skipTagIfTroublesome(self, tag_name, empty_element_tag):
        """Skip content inside a tag if it is a troublesome one.

        @warning You should probably call handleEndTag if the the
                 tag was indeed skiped.
        
        @return content inside the Tag, None if the Tag is not a
                troublesome one.
        """
        content = None
        if tag_name.lower() in self.TROUBLESOME_TAGS  and not empty_element_tag:
            # A troublesome tag with troublesome content. Skipt it.
            content = self._readUntilEndTag(tag_name)
        return content

class LogParser(SloppyHtmlParser):

    def handleText(self,text):
        print self._start, "TEXT",text

    def handleStartTag(self, tag_name, attrs={}, empty_element_tag=False):
        print self._start, "TAG",tag_name,attrs

        if self.skipTagIfTroublesome(tag_name, empty_element_tag):
            self.handleEndTag(tag_name)

    def handleEndTag(self,tag_name):
        print self._start, "ENDTAG", tag_name

    def handleProcessingInstruction(self,text):
        print self._start, "PI",text

    def handleComment(self,comment):
        print self._start, "COMMENT",comment

class LinkExtractor (SloppyHtmlParser):
    LINK_TAGS = { u'a'      : u'href',
                  u'link'   : u'href'}

    def __init__(self,text):
        # setup base class
        super(LinkExtractor,self).__init__(text)
        self.links = set()

    def handleStartTag(self, tag_name, attrs={}, empty_element_tag=False):
        # Extract Links
        name = tag_name.lower()
        if name  in self.LINK_TAGS:
            self.links.add( attrs[ self.LINK_TAGS[name] ])
        # Deal with troublesome tags
        if self.skipTagIfTroublesome(tag_name, empty_element_tag):
            self.handleEndTag(tag_name)


def _test():
    import doctest
    doctest.testmod()


if __name__ == '__main__':
    if len(sys.argv) > 1:
        data=unicode(open(sys.argv[1],'r').read(),'latin1')
        p = LinkExtractor(data)
        p.parse()
        for i in p.links:
            print i
    else:

        _test()

        data = u"asdajsdh\tAsasd.ASdasd < ajk>dajkaXXX<dh title=''>XXX \x00\xa4 "
        parser = TestParser(data)
        parser.parse()
        print "\n\nOriginal stream:", data
        for content in parser.items:
            print content

#        data=unicode(open('/tmp/uol.html','r').read(),'latin1')
#        p=LogParser(data)
#        p.parse()



# vim:sts=4:ts=4:et:sw=4:ai:fo+=tcroq:fileencoding=utf-8:
