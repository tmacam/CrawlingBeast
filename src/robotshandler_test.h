#ifndef __ROBOTSHANDLER_TEST_H
#define __ROBOTSHANDLER_TEST_H

#include "robotshandler.h"
#include "cxxtest/TestSuite.h"

static char robots_test1[] =
	"\n\n"
	"#Single comment line\n"
	"User-agent: blah\n"
	"Allow: /ERROR\n"
	"\n"
	"User-agent: blah\n"
	"User-agent: * #comment right in the middle\n"
	"User-agent: yada\n"
	"AlloW: /test1\n"
	"Disallow: /test2\n"
	"ALLOW: error\n"
	"\n"
	"";

static char robots_ufmg[] = {0x55,0x73,0x65,0x72,0x2d,0x61,0x67,0x65,0x6e,0x74,0x3a,0x20,0x2a,0x0d,0x0a,0x44,0x69,0x73,0x61,0x6c,0x6c,0x6f,0x77,0x3a,0x20,0x2f,0x6f,0x6e,0x6c,0x69,0x6e,0x65,0x2f,0x76,0x65,0x73,0x74,0x69,0x62,0x75,0x6c,0x61,0x72,0x32,0x30,0x30,0x36,0x2f};


class RobotsHandlerTestSuit : public CxxTest::TestSuite {
public:
	void test_test1()
	{
		robots_rules_t rules;
		filebuf  file(robots_test1, sizeof(robots_test1));
		RobotsParser r(file);
		r.parse();
		rules = r.getRules();

		TS_ASSERT_EQUALS(rules.size() , 2);
		TS_ASSERT_EQUALS(rules.front().first , "/test1");
		TS_ASSERT_EQUALS(rules.front().second , true);
		rules.pop_front();
		TS_ASSERT_EQUALS(rules.front().first , "/test2");
		TS_ASSERT_EQUALS(rules.front().second , false);
	}

	void test_UFMG_CRLF()
	{
		robots_rules_t rules;
		filebuf  file(robots_ufmg, sizeof(robots_ufmg));
		RobotsParser r(file);
		r.parse();
		rules = r.getRules();

		TS_ASSERT_EQUALS(rules.size(), 1);
		TS_ASSERT_EQUALS(rules.front().first ,"/online/vestibular2006/");
		TS_ASSERT_EQUALS(rules.front().second , false);
	}
};

#endif // __ROBOTSHANDLER_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
