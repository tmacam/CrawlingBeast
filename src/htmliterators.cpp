// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "htmliterators.hpp"


/***********************************************************************
			      HTMLContentRetriever
 ***********************************************************************/

void HTMLContentRetriever::handleStartTag(const std::string& tag_name,
	attr_list_t& attrs, bool empty_element_tag)
{
	this->safeHandleStartTag(tag_name, attrs, empty_element_tag);
	if (this->skipTagIfTroublesome(tag_name,empty_element_tag)) {
		this->handleEndTag(tag_name);
	}
}

void HTMLContentRetriever::safeHandleStartTag(const std::string& tag_name,
		attr_list_t& attrs, bool empty_element_tag)
{
	//FIXME we should retrieve title, alt and other kinds of
	//      attributes here.
}

void HTMLContentRetriever::handleText(filebuf text)
{
	if ( ! lstrip(text).eof()) {
		text_contents.push_back(text);
	}
}

void HTMLContentRetriever::parse()
{
	char c = 0;

	if (not this->eof()){
            c = *text;
            if (c == '<'){
                this->readTagLike();
            } else {
                this->readText();
	    }
	}
}

filebuf HTMLContentRetriever::pull()
{
	filebuf res;

	// Popule the text_content list
	while( not text.eof() && text_contents.empty() ) {
		parse();
	}

	if ( ! text_contents.empty() ){
		res = text_contents.front();
		text_contents.pop_front();
	}

	return res;
}


// EOF
