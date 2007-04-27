#ifndef __EXPLODE_TEST_H
#define __EXPLODE_TEST_H

#include "cxxtest/TestSuite.h"
#include "explode.h"

#include <vector>
#include <string>
#include <deque>
#include <list>


class ExplodeTestSuite : public CxxTest::TestSuite {
	std::string mixed_case;
	std::string lower_case;
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
#endif // __EXPLODE_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
