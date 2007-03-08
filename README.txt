
=============
CrawlingBeast
=============


Back in 2007, in the eary years of my PhD I was enrolled in a
Information Retrieval course. Its main assignment, which was divided
into 3 "stages" was to build a full-blow search engine: from crawling,
to indexing and finally to ranking and answering search queries. 

The assignment had the following restrictions:

  * The only external library we were allowed to use was libCURL,
    (http://curl.haxx.se/libcurl/), a HTTP/networking library.
    Everything else had to be coded by ourselves.

  * It had to be able to crawl at least 100K in a single day (or
    something of this magnitude IIRC).

This is the result of this "assignment". Its main code is in C++
although there is a python prototype for the early stages of the code
(HTML parsing, URL normalization, crawling).


What pieces of reusable code you will find here
===============================================

  * A lenient HTML push parser.

    It will try its best to parse any malformed HTML as good as a
    browser would. It was greatly inspired in Python's BeatufilSoup
    (http://www.crummy.com/software/BeautifulSoup/) and in open source
    browsers' parsing code.

    It correctly handles HTML character entities, ignores javascript
    code, STYLE and SCRIPT tags, finds text in "alt" attributes etc.

  * Character set detection and conversion utilities.

    Web is a jungle of Latin-1, Unicode, ISO-8859-X and whatnot. We try
    to detect whatever we are dealing with and convert it to UTF-8
    before processing.

  * URL normalization code.

    Follows RFCs as much as possible.

And that should be it. The rest of the code is not that reusable, but
"your mileage may vary".

What about style? Y U no KISS?
------------------------------

First, have in mind that I wrote python prototype before going to the
C++ version but that the majority of this code was being written while I
was having a blast reading Stroustrup's "The C++ Programming Language".
So you will find that not only the code style changes as the code
progresses from crawling to serving, but the constructions change as
well -- not necessarily for the better. 

For instance, you will find an HTML parser wrapped inside a STL
iterator. Why? "Why not, it will be fun and elegant" -- or so I though.
A silly idea wrapped into a dreadful interface but it made perfect sense
to abort the KISS principle at that time and do the things "the
(supposed) C++ way".


What about tests?
-----------------

Some of this code has companion unit tests. In particular the HTML
parsing and URL normalization code have. You can take a look at the it
to notice what sort of crazy stuff this code handles -- or fails to.
