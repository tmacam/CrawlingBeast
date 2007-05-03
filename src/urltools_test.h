#ifndef __URLTOOLS_TEST_H
#define __URLTOOLS_TEST_H

#include "cxxtest/TestSuite.h"
#include "urltools.h"

#include <sstream>
#include <set>

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


class FriendBaseURLParser: public BaseURLParser {
public:
	friend class PathParsingTests;
	friend class MiscURLTestes;
	FriendBaseURLParser(const std::string url=""):
		BaseURLParser(url) {}
};


class PathParsingTests : public CxxTest::TestSuite {
public:


	/** Observe that http://www.exemple.com/ == http://www.exemple.com ,
	 * as inscructed by RFC 3986, Section 6.2.3.
	 */
	void test_TrailingSlashAfterAuthority()
	{
		BaseURLParser u = BaseURLParser("http://www.exemple.com/");
		BaseURLParser v = BaseURLParser("http://www.exemple.com");
		
		TS_ASSERT_EQUALS(u,v);

		TS_ASSERT_EQUALS(v.getPath(), "/");
	}

	void test_PercentEncodingSimple()
	{
		BaseURLParser i("http://abc.com:80/~smith/home.html");
		BaseURLParser j("http://ABC.com/%7Esmith/home.html");
		BaseURLParser k("http://ABC.com:/%7esmith/home.html");

		TS_ASSERT_EQUALS(i,j);
		TS_ASSERT_EQUALS(j,k);
		TS_ASSERT_EQUALS(i,k);
	}

	//! Verifies charToPE only returns strings with length equals to 3.
	void test_CharToPEAndBack()
	{
		int i;
		char c;
		std::string res;

		for(i = 0x00; i <= 0xff; ++i) {
			c = i;
			res = FriendBaseURLParser::charToPE(c);
			TS_ASSERT_EQUALS(res.size(),3);
		}
	}

	void testTrailingSlashInPath()
	{

		BaseURLParser u("http://www.exemple.com/foo");
		BaseURLParser v("http://www.exemple.com/foo/");
		
		TS_ASSERT_DIFFERS(u,v);
	}

	void testPathNormalizationOne()
	{

		TS_ASSERT_EQUALS(
			BaseURLParser("./..///a/b/../c"),
			BaseURLParser("/a/c"));
	}
	
	void testRemoveDotSegments()
	{
		BaseURLParser u("/a/b/c/./../../g");
		TS_ASSERT_EQUALS(u.getPath(), "/a/g");

		BaseURLParser v("mid/content=5/../6");
		TS_ASSERT_EQUALS(v.getPath(), "mid/6");

		BaseURLParser x("mid/content=5////../6");
		TS_ASSERT_EQUALS(x.getPath(), "mid/6");
	}

	void testdecodeAndFixPe()
	{
		FriendBaseURLParser u = FriendBaseURLParser();

		TS_ASSERT_EQUALS(u.decodeAndFixPE("%20") ,  "%20");
		TS_ASSERT_EQUALS(u.decodeAndFixPE("%24") ,  "%24");
		TS_ASSERT_EQUALS(u.decodeAndFixPE("%5F") , "_");
		TS_ASSERT_THROWS(u.decodeAndFixPE("%XX") , InvalidURLException);

	}

	void testFixPercentEncoding()
	{
		FriendBaseURLParser u = FriendBaseURLParser();
		TS_ASSERT_EQUALS(
			u.fixPercentEncoding("%41%42%43%61%62%63%25 é%e9%23%3F"),
        		"ABCabc%25%20%C3%A9%E9%23%3F"
			);
	}

};


class RelativeAbsoluteURLTests : public CxxTest::TestSuite {
	typedef std::vector<BaseURLParser> urls_t;
	typedef std::vector<std::string> paths_t;
	paths_t paths;
	urls_t urls;
public:

	void setUp()
	{
		paths.clear();
		urls.clear();

		paths_t::const_iterator p;

		paths.push_back("/p4");
		paths.push_back("p4/");
		paths.push_back("../blah/..");
		paths.push_back("./isso.html");
		paths.push_back("file.html");

		for(p = paths.begin(); p != paths.end(); ++p){
			urls.push_back(BaseURLParser(*p));
		}
		
	}

	void test_AllAreRelative()
	{
		bool all_are = true;
		urls_t::const_iterator i;

		for(i = urls.begin(); i != urls.end(); ++i){
			all_are &= (i->isRelative());
		}

		TS_ASSERT(all_are);
	}

	void testPathsExpected()
	{
		TS_ASSERT_EQUALS(paths.size(), urls.size());

		TS_ASSERT_EQUALS("/p4", urls[0].getPath());
		TS_ASSERT_EQUALS("p4/", urls[1].getPath());
		TS_ASSERT_EQUALS("", urls[2].getPath());
		TS_ASSERT_EQUALS("isso.html", urls[3].getPath());
		TS_ASSERT_EQUALS("file.html", urls[4].getPath());
	}

	void testAbsoluteResolution()
	{
		BaseURLParser base = BaseURLParser("http://a.com/base/index.html");

		urls_t::iterator i;

		for(i = urls.begin(); i != urls.end(); ++i){
			*i = (base + *i);
		}

		TS_ASSERT_EQUALS( urls[0], BaseURLParser("http://a.com/p4"));
		TS_ASSERT_EQUALS( urls[1], BaseURLParser("http://a.com/base/p4/"));
		TS_ASSERT_EQUALS( urls[2], BaseURLParser("http://a.com/base/index.html"));
		TS_ASSERT_EQUALS( urls[3], BaseURLParser("http://a.com/base/isso.html"));
		TS_ASSERT_EQUALS( urls[4], BaseURLParser("http://a.com/base/file.html"));
	}

	void testAbsolutePlusAbsolute()
	{
		BaseURLParser u("http://a.com/base/index.html");
		BaseURLParser v("http://b.net/a/very/long/path/almost/");

		BaseURLParser x = u + v;
		TS_ASSERT_EQUALS(x,v);
	}
};




class MiscURLTests : public CxxTest::TestSuite {
public:

	/**Scheme-Based Normalization.
	 * RFC 3986 section 6.2.3.  
	 */
	void testSchemeBasedNormalization()
	{
		TS_ASSERT_EQUALS(
				BaseURLParser("http://www.Exemple.com.:80/"),
				BaseURLParser("HTTP://WWW.exemple.com")
				);

		BaseURLParser m("http://example.com");
		BaseURLParser n("http://example.com/");
		BaseURLParser o("http://example.com:/");
		BaseURLParser p("http://example.com:80/");

		TS_ASSERT_EQUALS(m,n);
		TS_ASSERT_EQUALS(n,o);
		TS_ASSERT_EQUALS(o,p);
		TS_ASSERT_EQUALS(p,m);
	}

	//!We are not handling sub-domain normalization.
	void testSubDomainNormalization()
	{
    		TS_ASSERT_DIFFERS( BaseURLParser("http://www.exemple.com"),
				BaseURLParser("http://exemple.com"));
	}

   	/** Protocol-Based Normalization.
	 * As Defined in RC 3986, section 6.2.4.
	 *
	 * @note This is the Trailing Slash in Path normalization procedure!
	 */
	void test_Protocol_Based_Normalization()
	{
		TS_ASSERT_DIFFERS( BaseURLParser("http://www.exemple.com/sub/"),
				   BaseURLParser("http://www.exemple.com/sub"));
	}


	void testDotSegmentAndPercentEncoding_One()
	{
		TS_ASSERT_EQUALS( BaseURLParser("http://a.com/%7e é %41"),
				  BaseURLParser("http://a.com/~%20%C3%A9%20A") );
	}

	void testDotSegmentAndPercentEncoding_Two()
	{
    		TS_ASSERT_EQUALS( BaseURLParser("http://AeXAMPLE/a/./b/../b/%63/%7bfoo%7d"),
			BaseURLParser("http://aexample://a/b/c/%7Bfoo%7D"));
	}

	// FIXME we don't include URLs with query and fragment components
	// in the list bellow.
	void testToStringConversion()
	{
		typedef std::vector<std::string> list_of_ursl_t;
		list_of_ursl_t normalized_urls;

		normalized_urls.push_back("http://www.slashdot.org/");
		normalized_urls.push_back("http://www.ubuntu.com/index.html");
		normalized_urls.push_back("index.html");
		normalized_urls.push_back("http://user:pass@domain.com:8080/a/");

		list_of_ursl_t::const_iterator i;

		for(i=normalized_urls.begin();i != normalized_urls.end(); ++i){
			TS_ASSERT_EQUALS(BaseURLParser(*i).str(), *i);
		}
	}

	void testAddingToSet()
	{
		std::set<BaseURLParser> links;

		links.insert(BaseURLParser("http://www.a.net"));
		links.insert(BaseURLParser("http://www.b.net/index.html"));
		links.insert(BaseURLParser("http://www.c.com"));
		links.insert(BaseURLParser("http://www.d.org"));

		TS_ASSERT_EQUALS(links.size(), 4);

	}

	void testStrip()
	{
		BaseURLParser u("http:// www.tanttron@tanttron.com.br/?query=none#frag");
		
		TS_ASSERT_EQUALS(u.strip().str(), "http://tanttron.com.br/");
	}


};




/* **********************************************************************

class BaseURLParser(BaseURLParser):
    """Simple Test class for the parser.

    The aim of this class is to be simple fixure upon which unittests can be
    easly build. For now, doctests are being used.

    >>> u = BaseURLParser("http://user:pass@Exemple.com.:80/f/file?query#frag")
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


    >>> BaseURLParser("http://www.exemple.com/?") != BaseURLParser("http://www.exemple.com/")
    True

        """
    pass

 * ********************************************************************** */


#endif // __URLTOOLS_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
