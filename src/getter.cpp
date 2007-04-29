#include <iostream>
#include "urlretriever.h"

int main()
{
	// Just plain HTTP, please.
	curl_global_init(CURL_GLOBAL_NOTHING);
	URLRetriever::headers_t::const_iterator h;

	URLRetriever ret("http://www.dcc.ufmg.br");
	ret.go();

	std::cout << ret.getContentType() << " " << ret.getStatusCode() << std::endl;

	
	URLRetriever::headers_t& headers = ret.getHeaders();

	for(h = headers.begin(); h != headers.end(); ++h) {
		std::cout << h->first << " : " << h->second << std::endl;
	}

	
}


// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
