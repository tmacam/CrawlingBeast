#ifndef __ENTITYPARSER_TEST_H
#define __ENTITYPARSER_TEST_H

#include "entityparser.h"
#include "cxxtest/TestSuite.h"

/**
 * @todo better tests.
 * */
class EntityParserTestSuit : public CxxTest::TestSuite {
public:
	void test_EmptyText()
	{
		TS_ASSERT_EQUALS( parseHTMLText(""), "" );
	}

	void test_SimpleEntityCleanText()
	{
		TS_ASSERT_EQUALS( parseHTMLText("this is an example text"),
				 "this is an example text" );
	}

	void test_SimpleSingleEntity()
	{
		TS_ASSERT_EQUALS( parseHTMLText("&lt;"), "<" );
	}

	void test_TextMixedWithEntities()
	{
		TS_ASSERT_EQUALS( parseHTMLText("d&lt;j&gt; vu"), "d<j> vu" );
	}

	void test_aacute()
	{
		TS_ASSERT_EQUALS( parseHTMLText("á"), "\xc3\xa1" );
		TS_ASSERT_EQUALS( parseHTMLText("&aacute;"), "\xc3\xa1" );
		TS_ASSERT_EQUALS( parseHTMLText("&#xE1;"), "\xc3\xa1" );
		TS_ASSERT_EQUALS( parseHTMLText("&#225;"), "\xc3\xa1" );
	}

	/* mozilla tests
	 *
	 * Mozilla is very liberal in its entities parsing:
	 *
	 * - Missing ";" is accepted
	 *
	 */
	void test_MissingEntityTerminator()
	{
		TS_ASSERT_EQUALS( parseHTMLText("&aacute&aacute;") ,
				"\xc3\xa1\xc3\xa1");
	}

	/* - Entities can be terminated by whitespace */
	void test_EntityEndsWithWhitespace()
	{
		TS_ASSERT_EQUALS( parseHTMLText("&lt &lt &lt"), "< < <" );
	}

	/* - Entities can be closed by EOF */

	void test_EntityEndsWithEOF()
	{
		TS_ASSERT_EQUALS( parseHTMLText("&aacute") ,  "\xc3\xa1");
	}

	/* - Numeric entities can be terminated by non-numeric content */
	void test_NumericEntityEndsWithNonNumeric()
	{
		TS_ASSERT_EQUALS( parseHTMLText("&#225Zbade") ,  "\xc3\xa1Zbade" );
	}


	void test_PrematurelyClosedEntity()
	{
		TS_ASSERT_EQUALS( parseHTMLText("&"), "&" );
	}


	/* Some invalid entities that must be interpreted as text */

	void test_InvalidEntityIdentifier()
	{
		TS_ASSERT_EQUALS( parseHTMLText("&invalidentity;"),
				"&invalidentity;" );
	}
};


#endif // __ENTITYPARSER_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
