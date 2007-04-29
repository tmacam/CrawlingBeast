#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <iostream>

#include <list>
#include <stdexcept>

#include "filebuf.h"

/* ********************************************************************** *
			LIBCURL MEMORY HANDLING ROUTINES
				 AND STRUCTURES
 * ********************************************************************** */

struct MemoryStruct {
  char *memory;
  size_t size;

  MemoryStruct(): memory(0), size(0){}
};

void *myrealloc(void *ptr, size_t size)
{
  /* There might be a realloc() out there that doesn't like reallocing
     NULL pointers, so we take care of it here */
  if(ptr)
    return realloc(ptr, size);
  else
    return malloc(size);
}


/* ********************************************************************** *
				 URLRetriever
 * ********************************************************************** */

size_t URLRetriever_static_writecallback(void *ptr, size_t size, size_t nmemb, void *data);
size_t URLRetriever_static_headercallback( void *ptr, size_t size, size_t nmemb, void *stream);

//!Something ist kaputt and das ist nicht mein problem
class UndeterminedURLRetrieverException: public std::runtime_error {
public:
	UndeterminedURLRetrieverException(std::string msg=""):
		std::runtime_error(msg) {}
};

class URLRetriever {
	CURL* _handle;
	std::string original_url;
	MemoryStruct mem;
	std::list<std::string> headers;
	int statuscode;
	std::string content_type;




public:
	static const std::string USER_AGENT;

	URLRetriever(std::string url):
	 _handle(0), original_url(url), mem(), headers(),
	 statuscode(400), content_type()
	{
		if(( _handle = curl_easy_init()) == NULL) {
			throw UndeterminedURLRetrieverException("init");
		}

		curl_easy_setopt(_handle, CURLOPT_URL, original_url.c_str());

		/* Setup DATA and HEADER callbacks */
		curl_easy_setopt(_handle, CURLOPT_WRITEFUNCTION, URLRetriever_static_writecallback);
		curl_easy_setopt(_handle, CURLOPT_WRITEDATA, (void *)this);
		curl_easy_setopt(_handle, CURLOPT_HEADERFUNCTION, URLRetriever_static_headercallback);
		curl_easy_setopt(_handle, CURLOPT_HEADERDATA, (void *)this);

		/* Redirections 
		 * rfc 2616, section 10.3 -> recomended 5, vamos com 10
		 */
		curl_easy_setopt (_handle, CURLOPT_FOLLOWLOCATION, 1);
		curl_easy_setopt (_handle, CURLOPT_MAXREDIRS, 10 );

		/* some servers don't like requests that are made without
		 * a user-agent field, so we provide one */
		curl_easy_setopt(_handle, CURLOPT_USERAGENT,USER_AGENT.c_str());

		// Thread-safety
		curl_easy_setopt(_handle, CURLOPT_DNS_USE_GLOBAL_CACHE, 0);
		curl_easy_setopt(_handle, CURLOPT_NOSIGNAL, 1);

		// Misc
		curl_easy_setopt(_handle, CURLOPT_TCP_NODELAY, 1);

	}

	~URLRetriever()
	{
		if (_handle){curl_easy_cleanup(_handle);}
		if (mem.memory){ free(mem.memory); }
	}

	size_t writeCallback(void* ptr, size_t realsize)
	{
		mem.memory = (char *)myrealloc(mem.memory, mem.size + realsize + 1);
		if (mem.memory) {
			memcpy(&(mem.memory[mem.size]), ptr, realsize);
			mem.size += realsize;
			mem.memory[mem.size] = 0;
		} else {
			// memory error... 
			throw UndeterminedURLRetrieverException("Memory");
		}
		return realsize;
	}

	size_t headerCallback(void* data, size_t realsize)
	{
		headers.push_back(std::string((const char*)data,realsize));
		return realsize;
	}

	void go()
	{
		char* _ct;
		long _code;

		if ( CURLE_OK != curl_easy_perform(_handle) ) {
			throw UndeterminedURLRetrieverException("perform");
		}

		if ( CURLE_OK != curl_easy_getinfo( _handle,
			CURLINFO_CONTENT_TYPE, &_ct) )
		{
			throw UndeterminedURLRetrieverException("content-type");
		}
		this->content_type = _ct;

		if ( CURLE_OK != curl_easy_getinfo(_handle,
			CURLINFO_RESPONSE_CODE, &_code) )
		{
			throw UndeterminedURLRetrieverException("code");
		}
		this->statuscode = _code;
	}

	std::list<std::string>& getHeaders() {return this->headers; }
	std::string getContentType() {return this->content_type; }
	int getStatusCode() {return this->statuscode; }

	filebuf getData() {return filebuf(this->mem.memory, this->mem.size); }

};



size_t URLRetriever_static_writecallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  URLRetriever& retriever = *((URLRetriever*)data);

  return retriever.writeCallback(ptr, realsize);
}

size_t URLRetriever_static_headercallback( void *ptr, size_t size, size_t nmemb, void *stream)
{
  size_t realsize = size * nmemb;
  URLRetriever& retriever = *((URLRetriever*)stream);

  return retriever.headerCallback(ptr, realsize);

}



const std::string URLRetriever::USER_AGENT = "DepressedAndParanoidMarvin/0.9";

int main()
{
	// Just plain HTTP, please.
	curl_global_init(CURL_GLOBAL_NOTHING);

	URLRetriever ret("http://www.dcc.ufmg.br");
	ret.go();

	std::cout << ret.getContentType() << " " << ret.getStatusCode() << std::endl;
}


// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
