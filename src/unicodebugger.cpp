#include "unicodebugger.h"
#include "explode.h"

//!@return Empty string is no encoding value is found inside content
std::string FindEncParser::get_charset_from_content_type(std::string content)
{
	std::string charset;
	std::string::size_type i; 
	std::string tmp;
	std::vector<std::string> parts;

	to_lower(content);

	if ( (i = content.find("charset")) != content.npos) {
		parts = split(content, "charset");
		tmp = parts[1];
		if ( (i = tmp.find("=")) != tmp.npos ) {
			parts = split(tmp,"=");
			tmp = parts[1];
		}
		charset = strip(tmp);
	}
	return charset;
}


void FindEncParser::parse()
{
	try {
		SloppyHTMLParser::parse();
	} catch(CharsetDetectedException) {
		// Okey dokey, we found the encoding :-)
	}
}

void FindEncParser::handleProcessingInstruction(const std::string& name,
		attr_list_t& attrs)
{
	if ( name == "xml" and attrs.find("encoding") != attrs.end()) {
		this->enc = attrs["encoding"].str();
		this->enc = strip(this->enc);
		throw CharsetDetectedException("Found in a Processing Instruction");
	}

}

void FindEncParser::handleStartTag(const std::string& tag_name,
	attr_list_t& attrs, bool empty_element_tag)
{
	this->safeHandleStartTag(tag_name, attrs, empty_element_tag);
	if (this->skipTagIfTroublesome(tag_name,empty_element_tag)) {
		this->handleEndTag(tag_name);
	}
}

void FindEncParser::safeHandleStartTag(const std::string& name,
	attr_list_t& attrs, bool empty_element_tag)
{
	std::string val;

	if ( name == "meta" and attrs.find("http-equiv") != attrs.end() ) {
		val = attrs["http-equiv"].str();
		to_lower(val);
		if (val == "content-type" and attrs.find("content") != attrs.end() ) {
			val = attrs["content"].str();
			to_lower(val);
			std::string charset = get_charset_from_content_type(val);
			if (not charset.empty()) {
				this->enc = strip(charset);
				throw CharsetDetectedException("Found in a Meta Tag");
			}
		}
	}
}


// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
