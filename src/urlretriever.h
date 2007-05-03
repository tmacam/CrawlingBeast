#ifndef __URLRETRIEVER_H
#define __URLRETRIEVER_H
/**@file urlretriever.h
 * @brief It get's URLs, get it?
 */

#include <stdexcept>
#include <curl/curl.h>
#include <map>

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
	typedef std::map<std::string, std::string> _headers_t;

	CURL* _handle;
	std::string original_url;
	MemoryStruct mem;
	_headers_t headers;
	int statuscode;
	std::string content_type;

	char curlerrbuf[CURL_ERROR_SIZE];
	struct curl_slist* extra_headers;

	bool only_html;

public:
	typedef _headers_t headers_t;
	static const int STATUS_OK = 200;

	static const std::string USER_AGENT;

	/**Constructor.
	 *
	 * @param url The URL to be fetched.
	 * @param only_html Should we include a Accept header
	 * 		    to limit responces to HTML/XML?
	 */
	URLRetriever(std::string url, bool only_html=true);

	~URLRetriever();

	size_t writeCallback(void* ptr, size_t realsize);
	size_t headerCallback(void* data, size_t realsize);

	//! Perform page download.
	void go();


	headers_t& getHeaders() {return this->headers; }
	std::string getContentType() {return this->content_type; }
	int getStatusCode() {return this->statuscode; }

	filebuf getData() {return filebuf(this->mem.memory, this->mem.size); }

	/**Get the final URL of this request.
	 *
	 * It may be the same as the request URL or something different if
	 * there was any redirection
	 */
	std::string getLocation();

};




#endif // __URLRETRIEVER_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
