#!/usr/bin/python
# -*- coding: utf-8 -*-
"""
laracna

The beast that controls URL crawling.

Not only this monster manages the other spinders (crawlers), it also
manages the list of known and pending URLs.
"""

#FIXME we still handle more than plain text/html - FIXME

from threading import *
import os
from urltools import BaseURLParser, NotSupportedSchemeException
from getpage import PageDownloader

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

    ERRLOG_LOCK = Lock()
    DOCID_LOCK = RLock()
    #: controls access to known_domains and domain_queue
    DOMAIN_LOCK = Condition(RLock()) 

    def __init__(self, store_dir="/tmp/"):
        """
        @param store_dir The directory where we save our files
        """
        # DocID Store
        self.store_dir = store_dir
        self.store_filename = self.store_dir + '/docids'
        self.store = open(self.store_filename,"a",0) # unbuffered

        self.errlog = open(self.store_dir + "/err.txt",'a',0)

        # Map of known domains
        self.known_domains = {}
        # Controls the order of domain downloads
        self.domain_queue = []

        # docId counter
        self.docId = 0

        # let threads know when we are stopping
        self.running = True

    def unserialize(self):
        """Get the list of known docIds/URLs from docids file."""
        # open the document with the list of known URLs
        retrieved = []
        pending = []
        docids_file = open(self.store_filename)
        for line in docids_file:
            id_str, url = line.strip().split(None,1) # at most 2 separations
            id = int(id_str)
            if self.pageExists(id):
                retrieved.append(url)
            else:
                pending.append(url)
        self.addPages(retrieved,unserializing=True)
        self.addPages(pending,unserializing=False)

    @synchronized(DOMAIN_LOCK) # domains may be updated while we read it...
    def addPages(self, urls, unserializing=False):
        """Enqueues pages for download.

        If pages are already known, nothing is done.
        @param urls a list of url strings.
        @param unserializing Are we reading URLs back from the disk?
        """
        dom = None
        # Group pages by domain
        domains = {}
        parsed_urls = [BaseURLParser(u) for u in urls]
        for u in parsed_urls:
            domains.setdefault(u.host,set()).add( str(u) )
        # Add pages per domain:
        for d, pages in domains.items():
            # New domain...
            if d not in self.known_domains:
                dom = self._addNewDomain(d, pages, unserializing)
            else: 
                dom = self.known_domains[d]
                dom.addPages(pages, unserializing)
                self._enqueueDomain(dom)

    @synchronized(DOMAIN_LOCK)
    def _addNewDomain(self,domain_name, pages, unserializing):
        """Creates a new domain, adding a group of pages to it.

        @param unserializing Are we reading URLs back from the disk?
        @return the Domain object of the new domain.
        """
        dom = Domain(domain_name, pages, self, unserializing)
        self.known_domains[domain_name] = dom
        self._enqueueDomain(dom)
        return dom

    @synchronized(DOMAIN_LOCK)
    def _enqueueDomain(self, dom):
        """Add a Domain instance to the download queue"""
        was_empty = False
        if not self.domain_queue:
            was_empty = True
        # should this domain really be enqueued?
        if not dom.empty() and not dom.in_queue:
            dom.in_queue = True
            self.domain_queue.append(dom)
        # If the list of domains in queue was empty,  there may be threads
        # waiting to be notified
        if was_empty:
            self.DOMAIN_LOCK.notify()

    @synchronized(DOMAIN_LOCK)
    def popPage(self):
        """Get a page to download from the queue.

        From the first domain in queue, get the first page in it's queue.

        @return a url as string.
        """
        page = None
        print currentThread(), "popPage" # DEBUG
        while not self.domain_queue:
            self.DOMAIN_LOCK.wait()
        if self.domain_queue:
            dom = self.domain_queue.pop(0)
            page = dom.popPage()

            # Deal with empty domains
            if dom.empty():
                dom.in_queue = False
            else:
                # non-empty domains should be added back to the queue
                self.domain_queue.append(dom)
        return page


    @synchronized(DOCID_LOCK)
    def registerURL(self,url):
        """Register a new found URL and assigns a docID to it.
        
        Registration happens by adding this URL to a file.
        @returns the docId assigned to the URL.
        """
        new_id = self._getNewDocId()
        self.store.write("%i\t%s\n" % (new_id, url) )
        return new_id

    @synchronized(DOCID_LOCK)
    def _getNewDocId(self):
        self.docId += 1
        return self.docId

    def pageExists(self,docid):
        """Checks if a given page was already downloaded.
        
        This is done verifing of a file named 'data' exists inside
        the path corresponding to a DocIdPath.
        """
        try:
            os.stat( self.getDocIdPath(docid) + "/data")
            return True
        except OSError:
            return False

    def getDocIdPath(self, docid):
        id_hex = "%08X" % docid
        id_path = "/".join([id_hex[0:2], id_hex[2:4], id_hex[4:6], id_hex[6:8]])
        return self.store_dir + "/" + id_path + "/"

    @synchronized(ERRLOG_LOCK)
    def reportBadCrawling(self,id,url,msg):
        self.errlog.write("%i %s ERR %s\n" % (id, str(url),str(msg)))




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
    def __init__(self,name, pages, manager, unserializing):
        """
        @param name The name of this domain
        @param pages Initial set of known pages to this domain.
        @param manager Reference to a Laracna instance
        @param unserializing Are we reading URLs back from the disk?
        """
        # Set of known pages/URLs
        self.name = name
        self.known_pages = set()
        self.pages_queue = []

        # used by Laracna to control whether or not this domain is in the queue
        # or not
        self.in_queue = False

        self.manager = manager

        # Add initial set of known pages
        self.addPages(pages,unserializing)

    @synchronized(PAGES_LOCK)
    def addPages(self, pages, unserializing=False):
        """Add pages for this domain:
        @param pages A list of URLs (str)
        """
        for p in pages:
            if p not in self.known_pages:
                self.known_pages.add(p)
                if not unserializing and self.allowedByRobotsTxt(p):
                    id = self.manager.registerURL(p)
                    self.pages_queue.append( Page(p,id) )

    def empty(self):
        return len(self.pages_queue) < 1

    @synchronized(PAGES_LOCK)
    def popPage(self):
        if self.pages_queue:
            return self.pages_queue.pop(0)
        else:
            return None

    def allowedByRobotsTxt(self,urlstr):
        #FIXME This is not implemented yet
        return True
        

class Page(object):
    """A simple struct-like class just to store information about
    a page.
    """
    def __init__(self,url,docid):
        self.url = url
        self.docid = int(docid)

class OfficeBoy(Thread):
    def __init__(self,manager):
        self.manager = manager
        super(OfficeBoy,self).__init__()

    def run(self):
        manager = self.manager
        while manager.running:
            # Homenagem ao DJ ATB: Don't Stop, 'till i come
            page = manager.popPage()
            if page:    # cuz we can return None...
                try:
                    d = PageDownloader(page.url)
                    d.get()
                    if d.follow :
                        manager.addPages(d.links)
                    # FIXME we are ignoring index/noindex
                    doc_path = manager.getDocIdPath(page.docid)
                    try:
                        os.makedirs(doc_path)
                    except OSError:
                        pass
                    meta = open(doc_path + "/meta", 'w',0)
                    d.writeMeta(meta)
                    meta.close()
                    data = open(doc_path + "/data",'w',0)
                    data.write(d.contents)
                    data.close()
                    print "DOWN", currentThread(), page.url
                except NotSupportedSchemeException:
                    # Seems like we got redirected to a not-supported URL
                    manager.reportBadCrawling(page.docid, page.url,
                                "BAD REDIRECT " +str(d.errestr))
                except Exception, e:
                    manager.reportBadCrawling(page.docid, page.url,e)
                
def _test():
    import doctest
    (fail, tests) = doctest.testmod()

    if not fail:
        print "All tests passed."

def main():
    N_OF_WORKERS = 10
    boss = Laracna('/tmp/down/')
    # Load data from previous invocations
    boss.unserialize()
    boss.addPages(['http://www.uol.com.br'])

    workers = [OfficeBoy(boss) for i in range(N_OF_WORKERS)]

    for w in workers:
        w.start()
    
    if raw_input():
        boss.running = False

    for w in workers:
        w.join()


if __name__ == '__main__':
#    import sys
#    if len(sys.argv) > 1 and sys.argv[1] != '-v':
#        pass
#    else:
#        if not _test():
    main()





# vim:sts=4:ts=4:et:sw=4:ai:fo+=tcroq:fileencoding=utf-8:
