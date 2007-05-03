#ifndef __PARSER_TEST_H
#define __PARSER_TEST_H

#include "cxxtest/TestSuite.h"
#include "parser.h"


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
};

// Stupid class just to test our methods
struct TestFriendBaseParser : public BaseParser {
	friend class ParserTest;
	std::string delims;
	filebuf res;

	TestFriendBaseParser(filebuf& t): BaseParser(t){}

	void parse() { }
};


class ParserTest : public CxxTest::TestSuite {

public:

	void testConsumeTokenOK() {
		char parse_data[] = "word 123: word";
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		TS_ASSERT_THROWS_NOTHING(p.consumeToken("word"));
	}

	void testConsumeTokenNotOK() {
		char parse_data[] = "word 123: word";
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		TS_ASSERT_THROWS(p.consumeToken("notthere"), InvalidCharError);
	}

	void testConsumeTokenOKChar() {
		char parse_data[] = "word 123: word";
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		TS_ASSERT_THROWS_NOTHING(p.consumeToken('w'));
		TS_ASSERT_THROWS_NOTHING(p.consumeToken('o'));
		TS_ASSERT_THROWS_NOTHING(p.consumeToken('r'));
		TS_ASSERT_THROWS_NOTHING(p.consumeToken('d'));
	}
	void testConsumeTokenNotOKChar() {
		char parse_data[] = "word 123: word";
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		TS_ASSERT_THROWS(p.consumeToken('='), InvalidCharError);
	}


	void testConsumeTokenEof() {
		char parse_data[] = {'w','o'};
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		TS_ASSERT_THROWS(p.consumeToken("word"), ParserEOFError);
	}


	void testReadUntilDelimiter(){
		char parse_data[] = "word 123: word";
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		filebuf res;
		std::string tmp;

		res = p.readUntilDelimiter(std::string(" "));
		tmp = std::string(res.current,res.end);


		TS_ASSERT_EQUALS(tmp,"word");
	}

	// Testing implicit conversion of char* to std::string
	void testReadUntilDelimiterCharPointer(){
		char parse_data[] = "word 123: word";
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		filebuf res;
		std::string tmp;

		res = p.readUntilDelimiter("2");
		tmp = std::string(res.current,res.end);


		TS_ASSERT_EQUALS(tmp,"word 1");
	}

	void testReadUntilDelimiterEOF(){
		char parse_data[] = "word 123: word";
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		// This will make parsing reach the EOF but it won't raise an
		// error
		p.readUntilDelimiter(std::string("#"));
		// But this will
		TS_ASSERT_THROWS(p.checkForEOF() , 
			ParserEOFError);
	}

	void testReadUntilDelimiterMark(){
		char parse_data[] = "word 123: word";
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		filebuf res;
		std::string tmp;

		res = p.readUntilDelimiterMark(std::string(": "));
		tmp = std::string(res.current,res.end);

		TS_ASSERT_EQUALS(tmp,"word 123");
	}

	// Testing with implicit conversion from char* to std::string
	void testReadUntilDelimiterMarkCharPointer(){
		char parse_data[] = "word 123: word";
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		filebuf res;
		std::string tmp;

		res = p.readUntilDelimiterMark(": ");
		tmp = std::string(res.current,res.end);

		TS_ASSERT_EQUALS(tmp,"word 123");
	}


	void testReadUntilDelimiterMarkEOF(){
		char parse_data[] = "word 123: word";
		filebuf content = filebuf(parse_data, sizeof parse_data);
		TestFriendBaseParser p = TestFriendBaseParser(content);

		TS_ASSERT_THROWS(
			p.readUntilDelimiterMark(std::string("#NOTFOUND#")),
			ParserEOFError);
	}



};

#endif // __PARSER_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
