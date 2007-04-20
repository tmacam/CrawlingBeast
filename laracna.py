#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
laracna

The beast that controls URL crawling.

Not only this monster manages the other spinders (crawlers), it also
manages the list of known and pending URLs.
"""

from threading import *

def synchronized(lock):
    """ Synchronization decorator.
    
    http://aspn.activestate.com/ASPN/Cookbook/Python/Recipe/465057
    """
    def wrap(f):
        def newFunction(*args, **kw):
            lock.acquire()
            try:
                return f(*args, **kw)
            finally:
                lock.release()
        return newFunction
    return wrap

########################################################################
#                                   LARACNA
########################################################################

class Laracna(object):
    """The beast the controls URL crawling."""

    DOCID_LOCK = RLock()
    DOMAIN_LOCK = RLock() # controls access to known_domains and domain_queue

    def __init__(self):
        # Map of known domains
        self.known_domains = {}
        # Controls the order of domain downloads
        self.domain_queue = []

        # docId counter
        self.docId = 0

    @synchronized(DOMAIN_LOCK) # domains may be updated while we read it...
    def addPages(self,urls):
        """Enqueues pages for download.

        If pages are already known, nothing is done.
        @param urls a list of BaseURLParser instances."""
        dom = None
        # Group pages by domain
        domains = {}
        for u in urls:
            domains.setdefault(u.domain,set()).add( str(u) )
        # Add pages per domain:
        for d, pages in domains.items():
            # New domain...
            if d not in self.known_domains:
                dom = self._addNewDomain(d, pages)
            else: 
                dom = self.known_domains[d]
                dom.addPages(pages)
                self._enqueueDomain(dom)

    @synchronized(DOMAIN_LOCK)
    def _addNewDomain(self,domain_name, pages):
        """Creates a new domain, adding a group of pages to it.

        @return the Domain object of the new domain.
        """
        dom = Domain(domain_name, pages, self)
        self.domains[domain_name] = dom
        self._enqueueDomain(dom)
        return dom

    @synchronized(DOMAIN_LOCK)
    def _enqueueDomain(self, dom):
        """Add a Domain instance to the download queue"""
        if not dom.empty() and not dom.in_queue:
            dom.in_queue = True
            self.domain_queue.append(dom)

    @synchronized(DOMAIN_LOCK)
    def popPage(self):
        """Get a page to download from the queue.

        From the first domain in queue, get the first page in it's queue.

        @return a url as string.
        """
        page = None
        if self.domain_queue:
            dom = self.domain_queue.pop(0)
            page = dom.popPage()

            # Deal with empty domains
            if dom.empty():
                dom.in_queue = False
            else:
                self.domain_queue.append(dom)

    @synchronized(DOCID_LOCK)
    def registerURL(self,url):
        """Register a new find URL and assigns a docID to it.
        
        Registration happens by adding this URL to a file.
        @returns the docId assigned to the URL.
        """
        #FIXME Registration happens by adding this URL to a file.
        new_id = self._getNewDocId()

    @synchronized(DOCID_LOCK)
    def _getNewDocId(self):
        self.docId += 1
        return self.docId



########################################################################
#                                    DOMAIN
########################################################################

class Domain(object):
    """Encapsulates our notion of something that about a domain.
    
    Domain objects control certain domain's URL lists:
     - known URL, already downloaded or not
     - pending URLs that must be downloaded.
    """
    PAGES_LOCK = RLock() # FIXME precisa de rlock mesmo?
    def __init__(self,name, pages=set(), manager):
        """
        @param name The name of this domain
        @param pages List known pages in this domain.
        """
        # Set of known pages/URLs
        self.name = name
        self.known_pages = set()
        self.pages_queue = []

        # used by Laracna to control whether or not this domain is in the queue
        # or not
        self.in_queue = False

        # Should this domain be enqueued?
        self.addPages(pages)

    @synchronized(PAGES_LOCK)
    def addPages(self, pages):
    """Add pages for this domain:
    @param pages A list of URLs (str)
    """
        for p in pages:
            if p not in self.known_pages:
                self.known_pages.add(p)
                id = self.manager.registerURL(p)
                self.pages_queue.append( Page(p,id) )

    def empty(self)
        return len(self.pages_queue) < 1

    @synchronized(PAGES_LOCK)
    def popPage(self):
        if self.pages_queue:
            return self.pages_queue.pop(0)
        else:
            return None

def Page(object):
    """A simple struct-like class just to store information about
    a page.
    """
    def __init__(self,url,docid):
        self.url = url
        self.docid = docid


def _test():
    import doctest
    (fail, tests) = doctest.testmod()

    return fail


if __name__ == '__main__':
    import sys
    if len(sys.argv) > 1 and sys.argv[1] != '-v':
        pass
    else:
        if not _test():
            print "All tests passed."






# vim:sts=4:ts=4:et:sw=4:ai:fo+=tcroq:fileencoding=utf-8:
