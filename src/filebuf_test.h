#ifndef __FILEBUF_TEST_H
#define __FILEBUF_TEST_H

#include "filebuf.h"
#include "cxxtest/TestSuite.h"

static const char empty_str[] = "";

class FilebufTestSuit : public CxxTest::TestSuite {
public:

	void test_DefaultConstructor(void) {
		filebuf f;

		TS_ASSERT_EQUALS(f.len(),0);
		TS_ASSERT(f.eof());
		TS_ASSERT_THROWS(*f, std::out_of_range);
	}

	void test_EmptyString(void) {
		filebuf g(empty_str, sizeof empty_str);

		TS_ASSERT_EQUALS(g.len(), 1);
		TS_ASSERT_EQUALS(*g, 0);
		TS_ASSERT_THROWS_NOTHING(++g);
		TS_ASSERT(g.eof());
		TS_ASSERT_THROWS(*g, std::out_of_range);
	}

	void test_OperatorDeref() {
		char msg[] = "12345";
		filebuf f(msg,5);

		TS_ASSERT_EQUALS(f.len(), 5);
		TS_ASSERT_EQUALS(*f, '1');
		TS_ASSERT_THROWS_NOTHING(++f);
		TS_ASSERT_EQUALS(*f, '2');
		TS_ASSERT_THROWS_NOTHING(f.read(1));
		TS_ASSERT_EQUALS(*f, '3');
		TS_ASSERT_EQUALS(f.read(1), &msg[2]);
		TS_ASSERT_THROWS_NOTHING(f += 1);
		TS_ASSERT(!f.eof());
		TS_ASSERT_EQUALS(*f, '5');
		// Ok, the next read will reach the EOF - but is allowed,
		// since it doesn't go beyond end
		TS_ASSERT_THROWS_NOTHING(f.read(1));
		// We reached EOF in the line above. Cannot derref anymore!
		TS_ASSERT_THROWS(*f, std::out_of_range);
	}

	void test_OperatorEqual()
	{
		char a[] = "apples";
		char b[] = "oranges and apples";

		filebuf i(a,sizeof(a));
		filebuf j(b,sizeof(b));

		TS_ASSERT_DIFFERS(j.len()  , i.len());
		TS_ASSERT_DIFFERS(j.start  , i.start);
		TS_ASSERT_DIFFERS(j.current, i.current);
		TS_ASSERT_DIFFERS(j.end    , i.end);
		TS_ASSERT_DIFFERS(*j       , *i);

		i += 4;
		j += 2;
		j=i;

		TS_ASSERT_EQUALS(j.len()  , i.len());
		TS_ASSERT_EQUALS(j.start  , i.start);
		TS_ASSERT_EQUALS(j.current, i.current);
		TS_ASSERT_EQUALS(j.end    , i.end);
		TS_ASSERT_EQUALS(*j       , *i);

	}

	void test_Str()
	{
		char msg[] = "12345";
		filebuf f(msg,5);

		TS_ASSERT_EQUALS(f.str(), std::string(msg));
	}

};

#endif // __FILEBUF_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
