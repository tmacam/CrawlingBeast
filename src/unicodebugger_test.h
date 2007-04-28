#ifndef __UNICODEBUGGER_TEST_H
#define __UNICODEBUGGER_TEST_H

#include "unicodebugger.h"
#include "cxxtest/TestSuite.h"

class UnicodeBuggerTestSuit : public CxxTest::TestSuite {
public:

	void test_get_charset_from_content_type()
	{
		std::string content_type = "text/html;charset=ISO-8859-1";

		std::string cs =
		    FindEncParser::get_charset_from_content_type(content_type);

		TS_ASSERT_EQUALS(cs, "iso-8859-1");

	}
};

#endif // __UNICODEBUGGER_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
