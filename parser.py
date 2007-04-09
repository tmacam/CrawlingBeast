import sys
# -*- coding: utf-8 -*-

# Assumptions
#
# * KISS
# * Text is read in a single pass.
# FIXME tag-end is not being handled properly


class BaseParser(object):
    
    def __init__(self, text=''):
        self._text = unicode(text)
        self._start = 0
        self._end = len(text) - 1

    def parse(self):
        while self._start <= self._end:
            character = self._text[ self._start ]
            if character == '<':
                self._readTag()
            else:
                self._readText()


    # Callbacks - Must be implemented
    def handleText(self, text):
        pass

    def handleStartTag(self, tag_name, attrs):
        pass

    # private functions
    
    def _readText(self):
        """Reads text until the start of something that *seems* like a tag."""
        # XXX   in the future, this should handle PCDATA, IIRC, it means it
        #       should handle entities and such. Ignore it for BaseParser
        i = self._start
        while i <= self._end and self._text[i] != '<':
            i = i + 1
        # We may have reached the end or found a possible tag start
        # in any case text[start:i] has all that matter for us -- nothing
        # more, nothing less.
        self.handleText( self._text[self._start : i] )
         
        # Parsing should re-start at current position
        self._start = i

    def _tagFollows(self):
        """Is the next thing to be read a tag?"""
        start = self._start
        next = start + 1
        end = self._end
        data = self._text
        # After the < there MUST be a character that is not a white-space,
        # not a & and not a >
        if next >= end or data[start] != u'<' or data[next].isspace() or \
           data[next] in (u'&', u'>'):
                # not a tag, actually
                return False
        return True


    def _readTag(self):
        """Read and parse a tag-like content.
        
        If it starts with a <, and is not followed by space or & characters
        then it is assuped to be a tag-like content.
        """
        i = self._start
        next_pos = i + 1
        if not self._tagFollows():
            # not a tag, actually, just text...
            self.handleText( self._text[i:i +1])
            self._start += 1
            return

        # Bellow here things start to get weird. We should do as a normal
        # grammar-based parser would...
        i += 1 # go past the '>'
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
    [('TEXT', u'a '), ('TEXT', u'<'), ('TEXT', u'> c ')]

    >>> TestParser("a<b>c").parse().items
    [('TEXT', u'a'), ('TAG', u'b', {}), ('TEXT', u'c')]

    >>> TestParser('#<a&whatever duplas="x"'+" simples='what' html=antigo attrhtml />#").parse().items
    [('TEXT', u'#'), ('TAG', u'a&whatever', {u'duplas': u'x', u'simples': u'what', u'html': u'antigo', u'attrhtml': None }), ('TEXT', u'#')]
    """
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
