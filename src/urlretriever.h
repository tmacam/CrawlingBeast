#ifndef __URLRETRIEVER_H
#define __URLRETRIEVER_H
/**@file urlretriever.h
 * @brief It get's URLs, get it?
 */

#include <stdexcept>
#include <curl/curl.h>
#include <list>

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
	CURL* _handle;
	std::string original_url;
	MemoryStruct mem;
	std::list<std::string> headers;
	int statuscode;
	std::string content_type;

public:
	static const std::string USER_AGENT;

	URLRetriever(std::string url);
	~URLRetriever();

	size_t writeCallback(void* ptr, size_t realsize);
	size_t headerCallback(void* data, size_t realsize);

	//! Perform page download.
	void go();


	std::list<std::string>& getHeaders() {return this->headers; }
	std::string getContentType() {return this->content_type; }
	int getStatusCode() {return this->statuscode; }

	filebuf getData() {return filebuf(this->mem.memory, this->mem.size); }

};




#endif // __URLRETRIEVER_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq: