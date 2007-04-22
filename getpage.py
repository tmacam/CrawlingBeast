#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
get-page

No documentation yet.
"""

# FIXME DocID not being used,
# FIXME page not being saved to disc
# FIXME dynamic pages should NOT be index or retrieved, but normalized
# FIXME PageDownloader::parse -> this is where dynamic and fragments
#       should be removed

import string
from unicodebugger import UnicodeBugger, get_charset_from_content_type
from parser import LinkExtractor
from urltools import BaseURLParser as URL, NotSupportedSchemeException, \
                InvalidURLException
import urllib2

class PageDownloader(object):
    """Simple downloader for a page/URL"""

    DEFAULT_ENCODING = 'utf-8'

    def __init__(self, url):
        # All url normalization and sanitization is dealt in
        # PageDownloader::parse, but it doesn't hurt trying to do it again...
        self.url = self.sanitizeURL(url)

        # Page contents (raw)
        self.contents = None

        # Redirects and META BASE tags may difficult the task of
        # rebuilding URLS from relative URLs. The base address
        # is thus an address where this document was really obtained
        self.base = self.url

        # This page may be accessible by other addresses:
        self.aliases = set()

        # shoult it be followed and indexed?
        self.follow = True
        self.index = True

        # To what pages/URLs it points to
        self.links = set()

        # what's it's encoding? Defaults to utf-8, since it's stricter and
        # almos anything is valid latin1
        self.encoding = self.DEFAULT_ENCODING

        # Was it previously retrieved? Was is succcessful?
        self.downloaded = False
        self.err = False
        self.errstr = ""
        #self.errcode = 0

    def get(self):
        """Retrieve and process this page."""
        try:
            self.download()
            self.parse()
        except urllib2.URLError, e:
            self.err = True
            self.errstr = unicode(e)
            raise
        except NotSupportedSchemeException:
            self.errstr = "Redirected to a unsuported protocol scheme [%s]" % \
                    self.base
            raise
        return self

    def download(self):
        original_url = self.url
        redirected_url = None
        page = urllib2.urlopen(self.url)
        # Obtain information from HTTP headers
        try:
            redirected_url = self.sanitizeURL( str(URL(page.geturl())) )
        except NotSupportedSchemeException:
            self.base = page.geturl()
            raise
        if  original_url != redirected_url:
            self.aliases.add(redirected_url)
            self.base = redirected_url
        if 'content-type' in page.info():
            ct = page.info()['content-type']
            self.encoding = get_charset_from_content_type(ct)
            if not self.encoding: self.encoding = self.DEFAULT_ENCODING
        self.contents = page.read()

        return self

    def sanitizeURL(self,url):
        """Does URL normalization and sanitization.

        Remove query and fragments from a URL.
        """
        # FIXME should be part of urltools
        u = URL(url)
        u.query = None
        u.fragment = None
        return str(u)

    def parse(self):
        """Parses page content.
        
        Unicod'ification and metadata and link extraction happens here.
        """
        # Converting to unicode
        unicoder = UnicodeBugger(self.contents, [self.encoding])
        data = unicoder.convert()
        self.encoding = unicoder.encoding
        
        # extracting link and meta information
        parser = LinkExtractor(data)
        parser.parse()
        self.parser = parser
        if parser.base:
            self.base = self.sanitizeURL(parser.base)
        self.follow = parser.follow
        self.index = parser.index

        # Prepare to get all the links from the page
        base_url = URL(self.base)
        for u in parser.links:
            try:
                l = URL(u)
                # FIXME this is where dynamic and fragments should be removed
                l.fragment = None
                l.query = None
                if l.isRelative():
                    self.links.add(str(base_url + l))
                else:
                    self.links.add(str(l))
            except NotSupportedSchemeException, InvalidURLException:
                # We just blindly ignore unsupported and invalid URLs
                pass

        return self

    def writeMeta(self,fh):
        """Writes metadata about this page to file-like object fh."""
        index = "noindex"
        if self.index: index = "index"

        follow = "nofollow"
        if self.follow: follow = "follow"

        fh.write("encoding: %s\nrobots: %s,%s\n" % (self.encoding,follow,index))

def get_page(url):
    p = PageDownloader(url)
    p.get()
    return (p.base, p.parser.links, p.links, p.encoding)

def _test():
    import doctest
    doctest.testmod()


if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1 and sys.argv[1] != '-v':
        for url in sys.argv[1:] :
            (base, links, norm_links, encoding) = get_page(url)
            print "URL:",url
            print "\tBASE:",base
            print "\tencoding:", encoding
            print "\tlinks (não normalizados):"
            for l in links: print "\t    ", l
            print "\tlinks normalizados:"
            for l in norm_links: print "\t    ", l
            print "\n\n"
    else:
        _test()
        print "Done."






# vim:sts=4:ts=4:et:sw=4:ai:fo+=tcroq:fileencoding=utf-8:
