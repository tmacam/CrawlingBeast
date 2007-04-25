#ifndef __HTMLPARSER_TEST_H
#define __HTMLPARSER_TEST_H

#include "cxxtest/TestSuite.h"
#include "htmlparser.h"

#include <sstream>

/* **********************************************************************
 *				HELPER FUNCTIONS
 * ********************************************************************** */

std::ostream& operator<<(std::ostream& out, const filebuf& f)
{
	std::string s(f.current, f.len());

	out << s;
	return out;
}

std::ostream& operator<<(std::ostream& out,const BaseHTMLParser::attr_list_t& m)
{
	BaseHTMLParser::attr_list_t::const_iterator i;
	out << "{";
	for(i = m.begin(); i != m.end(); ++i){
		out << "'" << (*i).first << "'";
		out << ":";
		out << "'" << (*i).second << "'";
		out << ", ";
	}
	out << "}";

	return out;
}


/* **********************************************************************
 *				TEST-READY CLASS
 * ********************************************************************** */

// Stupid class just to test our methods
class TestHTMLParser : public SloppyHTMLParser {
	friend class BaseHTMLParserTest;
public:
	std::ostringstream items;
	
	TestHTMLParser(const filebuf& t):
		SloppyHTMLParser(t), items() {}

	void handleText(filebuf data){
		this->items << "TEXT[" <<  data << "] ";
	}

	void handleStartTag(const std::string& tag_name,
			attr_list_t& attrs, bool empty_element_tag=false){
		this->items << "TAG[" <<  tag_name << ", " <<  attrs << "] ";
	}

	void handleEndTag(const std::string& tag_name){
		this->items << "ENDTAG[" <<  tag_name << "] ";
	}

	void handleProcessingInstruction(const std::string& tag_name,
			attr_list_t& attrs)
	{
		this->items << "PI[" <<  tag_name << ", " <<  attrs << "] ";
	}

	void handleComment(const filebuf& comment){
		this->items << "COMMENT[" <<  comment << "] ";
	}
    	


//    def handleProcessingInstruction(self,name,attrs):
//        self.items.append(("PI",name,attrs))

};


/* **********************************************************************
 *				   UNIT TESTS
 * ********************************************************************** */

class BaseHTMLParserTest : public CxxTest::TestSuite {
protected:

std::string getParsedString(const std::string text )
{
	filebuf f(text.c_str(), text.size());
	TestHTMLParser p(f);
	p.parse();

	return p.items.str();
}

public:

void testSimpleText(void) {
	TS_ASSERT_EQUALS( getParsedString("a c "), "TEXT[a c ] ");
}

void testNullTag(void) {
	TS_ASSERT_EQUALS( getParsedString("a <> c "), "TEXT[a <> c ] ");
}

void testSimpleTag(){
	TS_ASSERT_EQUALS( getParsedString("<b>"), "TAG[b, {}] ");
}

void testSimpleStartTagEndTag(){
	TS_ASSERT_EQUALS( getParsedString("<b></b>"), "TAG[b, {}] ENDTAG[b] ");
}

void testSimpleTagAndText(){
	TS_ASSERT_EQUALS( getParsedString("a<b>c"),
		"TEXT[a] TAG[b, {}] TEXT[c] ");
}

void testSimpleTagWithHTMLAtribute(){
	TS_ASSERT_EQUALS( getParsedString("<a href=http://www.uol.com.br/>"),
	"TAG[a, {'href':'http://www.uol.com.br/', }] ");
}

void testTagWithCloseAndAllKindOfAttributeDeclaration()
{
	TS_ASSERT_EQUALS( getParsedString("#<a duplas=\"x y\" simples='what is that' html=antigo attrhtml />#"),
	"TEXT[#] TAG[a, {'attrhtml':'', 'duplas':'x y', 'html':'antigo', 'simples':'what is that', }] ENDTAG[a] TEXT[#] ");
}

void testInterruptedTag(){
	TS_ASSERT_EQUALS( getParsedString("a<b"),
		"TEXT[a] TEXT[<b] ");
}


void testInterruptedAttributeValue(){
	TS_ASSERT_EQUALS( getParsedString("a<b style="),
		"TEXT[a] TEXT[<b style=] ");
}

void testInterruptedAttributeList(){
	TS_ASSERT_EQUALS( getParsedString("a<b style=b "),
		"TEXT[a] TEXT[<b style=b ] ");
}

// Mozilla just ignores whatever comes after the 'Name S?' sequence
// but before a '>' Let's just do the same!
void testNoiseInEndTag(){
	TS_ASSERT_EQUALS( getParsedString("a<b style='>'></b 1 asd asd asd>"),
    		"TEXT[a] TAG[b, {'style':'>', }] ENDTAG[b] ");
}

void testPIAndComment(){
	TS_ASSERT_EQUALS( getParsedString("a <? xml blah ?><!-- <b> --> c"),
		"TEXT[a ] PI[xml, {'blah':'', }] COMMENT[ <b> ] TEXT[ c] ");
}

void testInvalidAttrNameInInterruptedTagList(){
	TS_ASSERT_EQUALS( getParsedString("a<b style=b 1"),
		"TEXT[a] TEXT[<b style=b 1] ");
}

// It is debatable if this test is correct or not...
//void testinvalidTagName(){
//        std::cout << getParsedString("#<a&whatever duplas=\"x\" simples='what' html=antigo attrhtml />#");
//        TS_ASSERT_EQUALS( getParsedString("#<a&whatever duplas=\"x\" simples='what' html=antigo attrhtml />#"),
//            "TEXT[#], TAG[a&whatever, {'attrhtml':'', 'duplas':'x', 'html':'antigo', 'simples':'what', }], TEXT[#]")
//        ;
//}


void testReadUntilEndTag(){

	const char teste[] = "ERROR<a>ERROR<b>ERROR</b><a>ERROR</a>OK";
	filebuf f(teste,sizeof(teste));
	TestHTMLParser p(f);

	filebuf tmp;
	std::string res;

	p.readUntilEndTag("a");
	TS_ASSERT_THROWS(p.readUntilEndTag("b"), ParserEOFError);
}

}; // class  BaseHTMLParserTest


/* **********************************************************************
 *			    FORGUIVEFUL HTML PARSERS
 * ********************************************************************** */


class SloppyTestHTMLParser: public TestHTMLParser {
public:
	friend class SloppyHTMLParserTest;

	SloppyTestHTMLParser(const filebuf& t): TestHTMLParser(t)  {}

	void handleStartTag(const std::string& tag_name,
			attr_list_t& attrs, bool empty_element_tag=false)
	{
		this->items << "TAG[" <<  tag_name << ", " <<  attrs << "] ";
		if (this->skipTagIfTroublesome(tag_name,empty_element_tag)) {
			this->handleEndTag(tag_name);
		}
	}

};

class SloppyHTMLParserTest : public CxxTest::TestSuite {
public:
	std::string getParsedString(const std::string text )
	{
		filebuf f(text.c_str(), text.size());
		SloppyTestHTMLParser p(f);
		p.parse();

		return p.items.str();
	}

	void testSimpleScript()
	{
	    TS_ASSERT_EQUALS( getParsedString(
		"a<b><script><c><d></script><e>"),
		"TEXT[a] TAG[b, {}] TAG[script, {}] ENDTAG[script] TAG[e, {}] ");
	}

	void testSimpleScripEmptyClose()
	{
	    TS_ASSERT_EQUALS( getParsedString(
		"a<b><script/><c><d></script><e>"),
		"TEXT[a] TAG[b, {}] TAG[script, {}] ENDTAG[script] TAG[c, {}] TAG[d, {}] ENDTAG[script] TAG[e, {}] ");
	}
};

#endif // __HTMLPARSER_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
