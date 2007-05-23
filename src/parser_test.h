#ifndef __PARSER_TEST_H
#define __PARSER_TEST_H

#include "cxxtest/TestSuite.h"
#include "parser.h"


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
