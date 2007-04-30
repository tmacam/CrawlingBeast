#include "urlretriever.h"

#include <curl/types.h>
#include <curl/easy.h>

#include "explode.h"
#include "parser.h"


/* ********************************************************************** *
			LIBCURL MEMORY HANDLING ROUTINES
				 AND STRUCTURES
 * ********************************************************************** */

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


const std::string URLRetriever::USER_AGENT = "DepressedAndParanoidMarvin/0.9";


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


URLRetriever::URLRetriever(std::string url):
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
	curl_easy_setopt(_handle, CURLOPT_TIMEOUT, 60);

}

URLRetriever::~URLRetriever()
{
	if (_handle){curl_easy_cleanup(_handle);}
	if (mem.memory){ free(mem.memory); }
}

size_t URLRetriever::writeCallback(void* ptr, size_t realsize)
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

size_t URLRetriever::headerCallback(void* data, size_t realsize)
{
	
	std::string header_line = std::string((const char*)data,realsize);
	std::vector<std::string> res = split(header_line , ":", 1);

	if (res.size() == 2) {
		std::string key = strip(res[0]);
		headers[ to_lower(key) ] = strip(res[1]);
	} else {
		headers[ strip(res[0]) ] = strip(header_line);
	}

	return realsize;
}

void URLRetriever::go()
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

	if( this->statuscode != STATUS_OK){
		// We can't couple we errors...
		std::ostringstream out;
		out << "Invalid error code ";
		out << statuscode;
		throw UndeterminedURLRetrieverException( out.str() );
	}
}

std::string URLRetriever::getLocation()
{
	if (headers.count("location")) {
		// There was a redirection
		return headers["location"];
	} else {
		// No redirection
		return original_url;
	}
}




// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
