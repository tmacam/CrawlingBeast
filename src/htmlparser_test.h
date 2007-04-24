#ifndef __HTMLPARSER_TEST_H
#define __HTMLPARSER_TEST_H

#include "cxxtest/TestSuite.h"
#include "htmlparser.h"

#include <sstream>

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

// Stupid class just to test our methods
class TestHTMLParser : public BaseHTMLParser {
public:
	std::ostringstream items;
	
	TestHTMLParser(const filebuf& t):
		BaseHTMLParser(t), items() {}

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

	void handleProcessingInstruction(const std::string& name,
			attr_list_t& attrs){}

	void handleComment(const filebuf& comment){
		this->items << "COMMENT[" <<  comment << "] ";
	}
    	


//    def handleProcessingInstruction(self,name,attrs):
//        self.items.append(("PI",name,attrs))

};


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

}; // class  BaseHTMLParserTest

/*
########################################################################
#                     UNIT/DOC TESTS AND DEBUGING CLASSES
########################################################################


class TestParser(BaseHTMLParser):
    """Simple Test class for the parser.

    The aim of this class is to be simple fixure upon which unittests can be
    easly build. For now, doctests are being used.


    >>> TestParser("a<b style=").parse().items
    [('TEXT', u'a'), ('TEXT', u'<b style=')]
    
    >>> TestParser("a<b style=b ").parse().items
    [('TEXT', u'a'), ('TEXT', u'<b style=b ')]

    # EndTag 
    #
    # Mozilla just ignores whatever comes after the 'Name S?' sequence
    # but before a '>' Let's just do the same!
    >>> TestParser("a<b style='>'></b 1 asd asd asd>").parse().items
    [('TEXT', u'a'), ('TAG', u'b', {u'style': u'>'}), ('ENDTAG', u'b')]

    
    >>> TestParser("a <? xml blah ?><!-- <b> --> c").parse().items
    [('TEXT', u'a '), ('PI', u'xml', {u'blah': None}), ('COMMENT', u' <b> '), ('TEXT', u' c')]

    >>> TestParser("a<b style=b 1").parse().items
    [('TEXT', u'a'), ('TEXT', u'<b style=b 1')]
    """

    #>>> TestParser('#<a&whatever duplas="x"'+" simples='what' html=antigo attrhtml />#").parse().items
    #[('TEXT', u'#'), ('TAG', u'a&whatever', {u'duplas': u'x', u'simples': u'what', u'html': u'antigo', u'attrhtml': None }), ('TEXT', u'#')]
*/

#endif // __HTMLPARSER_TEST_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
