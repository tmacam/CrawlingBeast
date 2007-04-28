#include "unicodebugger.h"
#include "explode.h"

/* ********************************************************************** *
 *				   CONSTANTS
 * ********************************************************************** */

typedef UnicodeBugger::enc_pair_t enc_pair_t;

static const int __XML_MARKS_LEN = 4;
static const UnicodeBugger::enc_pair_t __XML_MARKS[__XML_MARKS_LEN] = {
    enc_pair_t("UTF-32LE","\x3c\x00\x00\x00"),
    enc_pair_t("UTF-32BE","\x00\x00\x00\x3c"),
    enc_pair_t("UTF-16LE","\x3c\x00\x3f\x00"),
    enc_pair_t("UTF-16BE","\x00\x3c\x00\x3f"),
};

static const int __BOM_MARKS_LEN = 5;
static const UnicodeBugger::enc_pair_t __BOM_MARKS[__BOM_MARKS_LEN] = { 
    enc_pair_t("UTF-8"   ,"\xef\xbb\xbf"),
    enc_pair_t("UTF-32LE","\xff\xfe\x00\x00"),
    enc_pair_t("UTF-32BE","\x00\x00\xfe\xff"),
    enc_pair_t("UTF-16LE","\xff\xfe"),
    enc_pair_t("UTF-16BE","\xfe\xff"),
};

static const int __ALL_MARKS_LEN = __XML_MARKS_LEN + __BOM_MARKS_LEN;
static const UnicodeBugger::enc_pair_t __ALL_MARKS[__ALL_MARKS_LEN] = { 
    //XML
    enc_pair_t("UTF-32LE","\x3c\x00\x00\x00"),
    enc_pair_t("UTF-32BE","\x00\x00\x00\x3c"),
    enc_pair_t("UTF-16LE","\x3c\x00\x3f\x00"),
    enc_pair_t("UTF-16BE","\x00\x3c\x00\x3f"),
    //BOM
    enc_pair_t("UTF-8"   ,"\xef\xbb\xbf"),
    enc_pair_t("UTF-32LE","\xff\xfe\x00\x00"),
    enc_pair_t("UTF-32BE","\x00\x00\xfe\xff"),
    enc_pair_t("UTF-16LE","\xff\xfe"),
    enc_pair_t("UTF-16BE","\xfe\xff"),
};


const UnicodeBugger::enc_list_t UnicodeBugger::XML_MARKS = 
	enc_list_t(__XML_MARKS, __XML_MARKS + __XML_MARKS_LEN);

const UnicodeBugger::enc_list_t UnicodeBugger::BOM_MARKS = 
	enc_list_t(__BOM_MARKS, __BOM_MARKS + __BOM_MARKS_LEN);

const UnicodeBugger::enc_list_t  UnicodeBugger::ALL_MARKS = 
	enc_list_t(__ALL_MARKS, __ALL_MARKS + __ALL_MARKS_LEN);
	




/* ********************************************************************** *
			     Methods Implementation
 * ********************************************************************** */



/**@return Empty string if no encoding is found or the encoding name,
 * if found inside content
 */
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


filebuf IconvWrapper::convert(const filebuf& input)
{
	size_t ret = 0;

	char* BUFFER;
	size_t BUFFSIZE;

	size_t inbytesleft = 0;
	size_t outbytesleft = 0;
	char* input_ptr = 0;
	char* output_ptr = 0;

	char* output = 0; // The final result will go here...
	size_t output_size = 0;

	// How much data should be put avaiable to iconv?
	// How about twice as much as input's length?
	inbytesleft = input.len();
	BUFFSIZE = 2 * inbytesleft;
	outbytesleft = BUFFSIZE;

	// Get memory to hold our converted data.
	BUFFER = new char[ BUFFSIZE ];

	input_ptr = (char*)input.current;
	output_ptr = BUFFER;

	ret = iconv(	cd,
			&input_ptr, &inbytesleft, 
			&output_ptr, &outbytesleft);
	if (ret == size_t(-1)) {
		delete[] BUFFER;
		throw IconvError("While converting...");
	} 

	// Got here? So everything went fine.

	// Copy data to a more memory-efficient location
	output_size = BUFFSIZE - outbytesleft;
	output = new char[output_size];
	memcpy(output, BUFFER, output_size);
	delete[] BUFFER;

	return filebuf(output, output_size);
}

bool UnicodeBugger::convertFrom(std::string encoding)
{
	bool was_successful = false;
	filebuf u;

	to_upper(encoding);

	// Why bother trying all over again?
	if (tried_encodings.count(encoding) == 0){
		try {
			tried_encodings.insert(encoding);
			IconvWrapper icw(encoding);
			this->converted_data = icw.convert(this->data);
			this->encoding = encoding;
			was_successful = true;
		} catch (IconvError) {
			// Just igore...
			was_successful = false;
		}
	}
	return was_successful;
}




bool UnicodeBugger::detectUTFEncoding()
{
	bool was_successful = false;
        size_t length = data.len();
	enc_list_t::const_iterator i;

	for(i = ALL_MARKS.begin(); i != ALL_MARKS.end(); ++i){

		const std::string& enc = i->first;
		const std::string& mark = i->second;

		if (length > mark.size() and startswith(data,mark) )
		{
			if ( convertFrom(enc) ) {
				was_successful = true;
				break;
			}
		}
	}
	
        return was_successful;
}


bool UnicodeBugger::findEncodingByParsing()
{
	bool was_successful = false;

        filebuf data_cpy = this->data;
	filebuf u;
	std::string enc;

	unsigned int max_pos = MAX_META_POS;
        if ( data.len() < max_pos ) {
            max_pos = data.len();
	}

        // latin1 is a good guess anyway, and won"t complain in most cases
        // about bad char. conversions
	try {
		// Convert the begining of data to latin1
		IconvWrapper icw("latin1");
		filebuf tmp = data_cpy.readf(max_pos);
		u = icw.convert(tmp); // XXX relese me later!!!
		// Read the document, tring to find it's advetised encoding
		FindEncParser e(u);
		e.parse();
		enc = e.getEnc();
		// Try the advertised encoding, if any.
		if (not enc.empty()) {
			was_successful = convertFrom(enc);
		}
	} catch (...){
		// ignore any error!
	}

	// If u was used, clear it.
	if (u.start){
		delete[] u.start;
	}

	return was_successful;
}

filebuf UnicodeBugger::convert()
{
	filebuf& u = this->converted_data;

        // Try the proposed encodings
	std::list<std::string>::iterator si;
	for(	si = suggested_encodings.begin();
		si != suggested_encodings.end();
		++si)
	{
		if ( convertFrom(*si) ){
			return u;
		}
	}
        
        // Ok, try to find the UTF BOM or XML header...
        if ( detectUTFEncoding() ){
            return u;
	}

        // Damn! Does this thing has a XML header or HTML Meta tag w/ encoding?
	if ( findEncodingByParsing()) {
            return u;
	}

        // we are out options here! Last options: utf-8 and latin1
	if ( convertFrom("uf-8") || convertFrom("LATIN1") ) {
		return u;
	}

	// Nothing found? What a bummer!
        throw CannotFindSuitableEncodingException("Giving up");
}






// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
