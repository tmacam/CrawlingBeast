#ifndef __STRMISC_TEST_H
#define __STRMISC_TEST_H

#include "cxxtest/TestSuite.h"
#include "strmisc.h"

#include <vector>
#include <string>
#include <deque>
#include <list>

class StringTransformsTestSuite : public CxxTest::TestSuite {
	std::string mixed_case;
	std::string lower_case;
	std::string upper_case;
public:

	void setUp()
	{
		mixed_case = "-AbCdeFg123";
		lower_case = "-abcdefg123";
		upper_case = "-ABCDEFG123";
	}

	void test_to_lower(void) {
		std::string tmp = mixed_case;
		TS_ASSERT_EQUALS(to_lower(tmp), lower_case);
		TS_ASSERT_DIFFERS(mixed_case, lower_case);
	}

	void test_to_upper()
	{
		std::string tmp = mixed_case;

		TS_ASSERT_EQUALS(to_upper(tmp), upper_case);
		TS_ASSERT_DIFFERS(mixed_case, upper_case);
	}

	void test_is_in(void)
	{
		TS_ASSERT( is_in('a',LETTERS) );
		TS_ASSERT( !is_in('1',LETTERS) );
		TS_ASSERT( is_in('1',DIGITS) );
		TS_ASSERT( is_in(' ',WHITESPACE) );
		TS_ASSERT( !is_in('a',WHITESPACE) );
	}

	void testStrip()
	{
		std::string original = "   123			\n\n";

		std::string res = strip(original);

		TS_ASSERT_EQUALS(res, "123");
	}

	void testeStartswithFilebuf()
	{
		const char teste[] = "This is a test string";
		const char start[] = "This i";
		const char mismatch[] = "akjdak";

		filebuf data(teste, sizeof(teste));

		TS_ASSERT( startswith(data, start));
		TS_ASSERT( not startswith(data, mismatch));
	}

	void testeStartswithString()
	{
		std::string data = "This is a test string";
		std::string start = "This i";
		std::string mismatch = "akjdak";

		TS_ASSERT( startswith(data, start));
		TS_ASSERT( not startswith(data, mismatch));
		TS_ASSERT( startswith("/teste/file.html", "/teste"));
		TS_ASSERT( not startswith("/teste/file.html", "/tmp"));
	}

	void testeStartswithEmptyString()
	{
		TS_ASSERT( startswith("/teste/file.html", ""));
		TS_ASSERT( startswith("/teste/file.html", ""));
	}

	void test_endswith()
	{
		std::string domainbr("www.uol.com.br");
		std::string domainorg("www.slashdot.org");

		TS_ASSERT( endswith(domainbr, ".br"));
		TS_ASSERT( not endswith(domainorg, ".br"));
	}

	void test_unichr()
	{
		TS_ASSERT_EQUALS( unichr(0), "");
		TS_ASSERT_EQUALS( unichr(38), "&");
		TS_ASSERT_EQUALS( unichr(97), "a");
		TS_ASSERT_EQUALS( unichr(193), "\xc3\x81"); // Aacute
		TS_ASSERT_EQUALS( unichr(9824), "\xe2\x99\xa0"); // spades
	}
};



class SplitTestSuite : public CxxTest::TestSuite {
public:

	void test_EmptyVector(void)
	{
		std::vector< std::string > res;

		res = split("","/");
		TS_ASSERT_EQUALS(res.size(), 1);
		TS_ASSERT_EQUALS(res[0], "");
	}

	void test_EmptyDeque(void)
	{
		std::deque<std::string> res;

		res = split< std::deque<std::string> >("","/");
		TS_ASSERT_EQUALS(res.size(), 1);
		TS_ASSERT_EQUALS(res[0], "");
	}

	void test_SingleSep(void)
	{
		std::vector< std::string > res;

		res = split("/","/");
		TS_ASSERT_EQUALS(res.size(), 2);
		TS_ASSERT_EQUALS(res[0], "");
		TS_ASSERT_EQUALS(res[1], "");
	}

	void test_Test1Vector(void)
	{
		std::vector< std::string > res;

		res = split("/A/B/C/D/","/");

		TS_ASSERT_EQUALS(res.size(), 6);
		TS_ASSERT_EQUALS(res[0], "")
		TS_ASSERT_EQUALS(res[1], "A");
		TS_ASSERT_EQUALS(res[2], "B");
		TS_ASSERT_EQUALS(res[3], "C");
		TS_ASSERT_EQUALS(res[4], "D");
		TS_ASSERT_EQUALS(res[5], "");

	}

	void test_Test1Deque(void)
	{
		std::deque< std::string > res;

		res = split< std::deque< std::string > >("/A/B/C/D/","/");

		TS_ASSERT_EQUALS(res.size(), 6);
		TS_ASSERT_EQUALS(res[0], "")
		TS_ASSERT_EQUALS(res[1], "A");
		TS_ASSERT_EQUALS(res[2], "B");
		TS_ASSERT_EQUALS(res[3], "C");
		TS_ASSERT_EQUALS(res[4], "D");
		TS_ASSERT_EQUALS(res[5], "");

	}

	void test_Test1List(void)
	{
		std::list<std::string> res;

		res = split<  std::list<std::string> >("/A/B/C/D/","/");

		TS_ASSERT_EQUALS(res.size(), 6);
	}

	void test_Test2Vector(void)
	{
		std::vector< std::string > res;

		res = split("a::b:c","::");

		TS_ASSERT_EQUALS(res.size(), 2);
		TS_ASSERT_EQUALS(res[0], "a")
		TS_ASSERT_EQUALS(res[1], "b:c");
	}

	void test_MaxSplit1Vector(void)
	{
		std::vector< std::string > res;

		res = split("/A/B/C/D/","/",1);

		TS_ASSERT_EQUALS(res.size(), 2);
		TS_ASSERT_EQUALS(res[0], "")
		TS_ASSERT_EQUALS(res[1], "A/B/C/D/");

	}

	void test_MaxSplit1Deque(void)
	{
		std::deque< std::string > res;

		res = split< std::deque< std::string > >("/A/B/C/D/","/",1);

		TS_ASSERT_EQUALS(res.size(), 2);
		TS_ASSERT_EQUALS(res[0], "")
		TS_ASSERT_EQUALS(res[1], "A/B/C/D/");

	}

	void test_MaxSplit2Vector(void)
	{
		std::vector< std::string > res;

		res = split("/A/B/C/D/","/",2);

		TS_ASSERT_EQUALS(res.size(), 3);
		TS_ASSERT_EQUALS(res[0], "")
		TS_ASSERT_EQUALS(res[1], "A");
		TS_ASSERT_EQUALS(res[2], "B/C/D/");

	}

	void test_MaxSplit2Deque(void)
	{
		std::deque< std::string > res;

		res = split< std::deque< std::string > >("/A/B/C/D/","/",2);

		TS_ASSERT_EQUALS(res.size(), 3);
		TS_ASSERT_EQUALS(res[0], "")
		TS_ASSERT_EQUALS(res[1], "A");
		TS_ASSERT_EQUALS(res[2], "B/C/D/");

	}

};


class JoinTestSuite : public CxxTest::TestSuite {
	std::string splitNjoin(const std::string what,
			const std::string sep)
	{
		std::vector< std::string > res;
		res = split(what,sep);

		return join(sep,res);
	}
public:

	void test_EmptyVector(void)
	{
		std::vector< std::string > res;
		res = split("","/");

		TS_ASSERT_EQUALS(join("/",res), "");
	}

	void test_SingleVector(void)
	{
		std::string sample = "a/b";
		TS_ASSERT_EQUALS(splitNjoin(sample,"/"), sample);
	}

	void test_SampleVectorOne(void)
	{
		std::string sample = "/a/b/";
		TS_ASSERT_EQUALS(splitNjoin(sample,"/"), sample);
	}

	void test_LotsOfSeparators(void)
	{
		std::string sample = "/a/b////////";
		TS_ASSERT_EQUALS(splitNjoin(sample,"/"), sample);
	}

	void test_MultiCharSeparators(void)
	{
		std::string sample = "a::b::c:::";
		TS_ASSERT_EQUALS(splitNjoin(sample,"::"), sample);
	}

	void test_emptyAgain()
	{
		std::vector< std::string > res;
		TS_ASSERT_EQUALS(join("/",res), "");
	}
};


class LStripTestSuite : public CxxTest::TestSuite {
public:
	void testEmpty()
	{
		filebuf empty;

		TS_ASSERT(empty.eof());
		TS_ASSERT_EQUALS(lstrip(empty).len(), 0);
		TS_ASSERT(empty.eof());
	}

	void testEofAfter()
	{
		const char t[] = "   \v\t\n\r\0   ";
		filebuf f(t,sizeof(t));

		TS_ASSERT_EQUALS( lstrip(f).len(), 0);
	}

	void testSimple()
	{
		const char t[] = "   \v\t\n\r\0   a b c ";
		filebuf f(t,sizeof(t));

		const char _expected[] = "a b c ";
		filebuf expected(_expected, sizeof(_expected));

		TS_ASSERT_EQUALS( lstrip(f).str(), expected.str());
	}
};


class WideCharConverterTestSuite : public CxxTest::TestSuite {
public:
	void testNullStr()
	{
		std::string in;
		std::wstring out;

		WideCharConverter wconv;
		
		out = wconv.mbs_to_wcs(in);
		TS_ASSERT_EQUALS(out, std::wstring());

		in = wconv.wcs_to_mbs(out);
		TS_ASSERT_EQUALS(in, std::string());

	}

	void testSimpleExample()
	{
		std::string utf8("aÇão weißbier 1ªcolocada palavra«perdida»©");


		WideCharConverter wconv;

		std::wstring tmp = wconv.mbs_to_wcs(utf8);

		TS_ASSERT_EQUALS(utf8 ,wconv.wcs_to_mbs(tmp));
	}
};

#endif // __STRMISC_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
