#include <iostream>
#include "urlretriever.h"

int main()
{
	// Just plain HTTP, please.
	curl_global_init(CURL_GLOBAL_NOTHING);

	URLRetriever ret("http://www.dcc.ufmg.br");
	ret.go();

	std::cout << ret.getContentType() << " " << ret.getStatusCode() << std::endl;
}


// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
