#include <iostream>
#include "urlretriever.h"
#include "pagedownloader.h"

void test_URLRetriever()
{
	URLRetriever::headers_t::const_iterator h;
	URLRetriever ret("http://www.dcc.ufmg.br/~tmacam");
	ret.go();
	std::cout << ret.getContentType() << " " << ret.getStatusCode() << std::endl;

	URLRetriever::headers_t& headers = ret.getHeaders();
	for(h = headers.begin(); h != headers.end(); ++h) {
		std::cout << h->first << " : " << h->second << std::endl;
	}

	filebuf data =  ret.getData();


}

void test_PageDownloader(std::string url)
{

	PageDownloader::url_set_t::const_iterator li;

	PageDownloader p(url);
	p.get();

	std::cout << "Page: " << url << std::endl;
	std::cout << "    base: " << p.base.str() << std::endl;
	std::cout << "    encoding: " << p.encoding << std::endl;
	std::cout << "    links: " <<  std::endl;
	for(li = p.links.begin(); li != p.links.end(); ++li)
	{
		std::cout<< "\t"<< li->str() << std::endl;
	}
	
}

int main(int argc, char* argv[])
{
	// Just plain HTTP, please.
	curl_global_init(CURL_GLOBAL_NOTHING);
	std::string url = "http://www.dcc.ufmg.br/~tmacam";
	
//	test_URLRetriever();
	for(int i = 1; i < argc; ++i) {
		test_PageDownloader(std::string(argv[i]));
	}

	std::string enter;
	std::cout << "Done\n";
	std::cin >> enter;

	
}


// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
