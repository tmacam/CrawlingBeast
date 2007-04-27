#ifndef __URLTOOLS_TEST_H
#define __URLTOOLS_TEST_H

#include "cxxtest/TestSuite.h"
#include "urltools.h"

#include <sstream>

class URLTests : public CxxTest::TestSuite {
public:
	void testBlah()
	{
		TS_ASSERT_EQUALS(1,1);
	}

	void testEmptyEquals()
	{
		TS_ASSERT_EQUALS(BaseURLParser(), BaseURLParser(""));
	}
	void testMixedCaseDomain()
	{
		TS_ASSERT_EQUALS(BaseURLParser("HTTP://WWW.EXaMPLE.TLD"),
			BaseURLParser("http://www.example.tld"));
	}

	void testSimpleUnsupportedExcetionsInHost()
	{
		TS_ASSERT_THROWS(BaseURLParser("FTP://WWW.EXEMPLE.TLD"),
				NotSupportedSchemeException);

		TS_ASSERT_THROWS(BaseURLParser("javascript: alert();"),
				NotSupportedSchemeException);
	}

	/* Scheme tests */
	
	/**Verifies if an empty string is accepted.
	 *
	 * It should not be, as per section RFC 3986
	 * Section 3.1 .
	 *
	 * @see BaseURLParser::readScheme()
	 */
	void testEmptyScheme()
	{
		BaseURLParser u = BaseURLParser("://www.exemple.tld/");
		TS_ASSERT_EQUALS(u.getScheme(), "");
	}
};

class AuthorityParsingTests : public CxxTest::TestSuite {
public:

	void test_SpacesInHostname()
	{
		TS_ASSERT_THROWS(
			BaseURLParser("http:// invalid.url:80/"),
			InvalidURLException);
	}

	void test_InvalidCharsInHostname()
	{
		TS_ASSERT_THROWS(
			BaseURLParser("http://#urlsite#/estilo.css"),
			InvalidURLException);
	}

	void test_NonAsciiInHostname()
	{
		TS_ASSERT_THROWS(
			BaseURLParser("http://www.ficções.net/biblioteca_conto/"),
			InvalidURLException);
	}

	void test_EmptyHostname()
	{
		TS_ASSERT_THROWS(
			BaseURLParser("http://a.very.long.username:8080@:NOTPORT/biblioteca_conto/"),
			InvalidURLException);
	}

	void test_EmptyPortOk()
	{
		TS_ASSERT_THROWS_NOTHING(
			BaseURLParser("http://example.tld:/"));
	}

	void test_InvalidPort()
	{
		TS_ASSERT_THROWS(
			BaseURLParser("http://example.tld: notvalid/"),
			InvalidURLException);

		TS_ASSERT_THROWS(
			BaseURLParser("http://example.tld:80 /"),
			InvalidURLException);

		TS_ASSERT_THROWS(
			BaseURLParser("http://example.tld:http/"),
			InvalidURLException);
	}


};



/* **********************************************************************

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
    >>> [u.path for u in relative_urls]
    [u'/p4', u'p4/', u'', u'isso.html', u'file.html']

    >>> absolute = TestURLParser("http://a.com/base/index.html")
    >>> [str(absolute + u) for u in relative_urls]
    ['http://a.com/p4', 'http://a.com/base/p4/', 'http://a.com/base/index.html', 'http://a.com/base/isso.html', 'http://a.com/base/file.html']

    >>> TestURLParser("http://www.exemple.com") != TestURLParser("http://exemple.com")
    True

    # 6.2.4.  Protocol-Based Normalization
    >>> TestURLParser("http://www.exemple.com/sub/") != TestURLParser("http://www.exemple.com/sub")
    True

    >>> TestURLParser("http://www.exemple.com/?") != TestURLParser("http://www.exemple.com/")
    True

    >>> TestURLParser(u"http://a.com/%7e é %41") == TestURLParser(u"http://a.com/~%20%C3%A9%20A")
    True

    >>> u = BaseURLParser("http://AeXAMPLE/a/./b/../b/%63/%7bfoo%7d")
    >>> v = BaseURLParser("http://aexample://a/b/c/%7Bfoo%7D")
    >>> u == v
    True
    """
    pass

 * ********************************************************************** */


#endif // __URLTOOLS_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
