#ifndef __UNICODEBUGGER_TEST_H
#define __UNICODEBUGGER_TEST_H

#include "unicodebugger.h"
#include "cxxtest/TestSuite.h"


class IconvWrapperTestSuit : public CxxTest::TestSuite {
public:
	void test_SuccessfulConvertion()
	{
		const char latin1[] = "\x61\xE7\xE3\x6F";
		const char utf8[] = "\x61\xC3\xA7\xC3\xA3\x6F";

		filebuf l(latin1, sizeof(latin1) -1);
		filebuf u;

		IconvWrapper i("latin1");
		TS_ASSERT_THROWS_NOTHING( u = i.convert(l) );
		TS_ASSERT_EQUALS( u.str(), utf8);

	}
};


class UnicodeBuggerTestSuit : public CxxTest::TestSuite {
public:

	void test_get_charset_from_content_type()
	{
		std::string content_type = "text/html;charset=ISO-8859-1";

		std::string cs =
		    FindEncParser::get_charset_from_content_type(content_type);

		TS_ASSERT_EQUALS(cs, "iso-8859-1");

	}

	void test_UnicodebuggerUTF16()
	{
		std::string filename = "../unicode_tests/note_encode_utf16_u.xml";
		MMapedFile file(filename);
		UnicodeBugger unicodeforgodsake(file.getBuf());
		AutoFilebuf data (unicodeforgodsake.convert());
		
		TS_ASSERT_EQUALS(unicodeforgodsake.getEncoding(), "UTF-16LE" );

	};

	void test_Unicodebugger1252()
	{
		std::string filename = "../unicode_tests/note_encode_1252_u.xml";
		MMapedFile file(filename);
		UnicodeBugger unicodeforgodsake(file.getBuf());
		AutoFilebuf data(unicodeforgodsake.convert());
		
		TS_ASSERT_EQUALS(unicodeforgodsake.getEncoding(), "WINDOWS-1252" );

	};

};

#endif // __UNICODEBUGGER_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
