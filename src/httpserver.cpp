// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "httpserver.hpp"

#include <sstream>

#include <arpa/inet.h>		// htons
#include <strings.h>		// bzero

#include "strmisc.h"
#include "urltools.h"

#include <sys/sendfile.h>	// For StaticFileHandler
#include "mmapedfile.h"		// For StaticFileHandler

#include <iostream>		// FIXME temporary


/******************************************************************************
				Misc. Functions
 ******************************************************************************/

std::string http::mk_response_header(std::string msg, int code,
			std::string type, std::string extra_hdrs)
{
	std::ostringstream buf;
	const std::string& CRLF = http::CRLF;

	buf <<	"HTTP/1.0 " << code <<  msg << CRLF <<
		"Server: ProcastinationBroadcaster/0.1" << CRLF <<
		"Connection: close" << CRLF << 
		"Content-Type: " << type << CRLF << 
		extra_hdrs <<  // They should contain line-ending CRLFs
		CRLF;
	
	return buf.str();

}


/******************************************************************************
				   Exceptions
 ******************************************************************************/


std::string HTTPException::make_error_page(std::string msg, int code)
{
	return make_error_page(msg,code,msg);
}

std::string HTTPException::make_error_page(std::string msg, int code,
						std::string text)
{
	std::ostringstream buf;
	const std::string& CRLF = http::CRLF;

	buf <<	http::mk_response_header(msg,code) <<
		"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">" << CRLF << 
		"<html><head>" << CRLF << 
		"<title>" << code << " " << msg << "</title>" << CRLF << 
		"</head><body>" << CRLF << 
		"<h1>" << text <<  "</h1>" << CRLF << 
		"</body></html>" << CRLF;
	
	return buf.str();

}



/******************************************************************************
			       HTTPClientHandler
 ******************************************************************************/


void HTTPClientHandler::go()
{
	try {
		parseRequest();
		handler->process(*this);
	} catch (HTTPException& e) {
		try {
			this->write(e.what());
			this->loseConnection();
		} catch (...){
			// Errors in write are ignored
		}
	}
}

void HTTPClientHandler::parseRequest()
{
	const int BUF_LEN = 1024;
	char buf[BUF_LEN];
	ssize_t n;
	std::string msg;

	// Read untill the end of the request
	while( 0 < (n = recv(fd,buf, BUF_LEN, 0)) ) {
		msg.append(buf, n);
		// Get out if found the request end
		if (msg.find(http::CRLF_CRLF)) break;
	}

	// parse request
	if (msg.empty()) {
		throw BadRequestHTTPException();
	}
	strvec_t fullreq_lines(split(msg,http::CRLF));
	strvec_t::iterator line = fullreq_lines.begin();
	strvec_t request_fields(split(*line," ",2));
	if (request_fields.size() != 3) {
		throw BadRequestHTTPException();
	}
	method = request_fields[0];
	uri = request_fields[1];
	proto_version = request_fields[2];
	// Parse request headers
	for(; line != fullreq_lines.end(); ++line){
		strvec_t header_fields(split(to_lower(*line),":",1));
		// Ignore empty headers, continuations and malformed whatever
		if (header_fields.size() < 2) {
			continue;
		}
		std::string key = strip(header_fields[0]);
		std::string val = strip(header_fields[1]);
		headers[key] = val;
	}
}

void HTTPClientHandler::write(const std::string data)
{
	const char*  buf = data.c_str();
	const size_t len = data.size();

	size_t total = 0;		// how many bytes we've sent
	size_t bytesleft = len;		// how many we have left to send
	int n;

	while(total < len) {
		n = send(fd, buf+total, bytesleft, 0);
		if (n == -1) {
			throw ErrnoSysException("Error in send()");
		}
		total += n;
		bytesleft -= n;
	}

}

void HTTPClientHandler::loseConnection()
{
	if(fd) {
		if( -1 == close(fd) ){
			throw ErrnoSysException("Error in close()");
		}
		fd = 0;
	}
}


/******************************************************************************
				 BaseHTTPServer
 ******************************************************************************/

void BaseHTTPServer::process(HTTPClientHandler& req)
{

	std::cout << "Method: " << req.method << " URI: " <<
		req.uri << " version:" << req.proto_version << std::endl;
	StrStrMap& headers = req.headers;
	StrStrMap::const_iterator hdr;
	for (hdr = headers.begin(); hdr != headers.end(); ++hdr){
		std::cout << "\t" << hdr->first << " == " <<
			hdr->second << std::endl;
	}

	// XXX HACK
	// Just to force dot segment normalization in the request path.
	// and avoid "path hacks" like "../../../../etc/passwd"
	BaseURLParser uri("http://localhost/" + req.uri);

	// Get the first path segment
	std::vector<std::string> path_segs = split(uri.path,"/");
	assert(path_segs.size() > 1);
	
	std::cout << "Requested path " << uri.path << " selector segment: "<<
		path_segs[1] << std::endl; // FIXME

	TReqHandlerMap::iterator han = handlers.find(path_segs[1]);
	if (han == this->handlers.end()) {
		throw NotFoundHTTPException();
	} else {
		han->second->process(req);
	}



	req.loseConnection();
}


int BaseHTTPServer::setupServerSocket(uint16_t server_port, int backlog)
{
	int fd = -1;
	int res = 0;
	int yes = 1;
	struct sockaddr_in addr;


	/* Socket creation */
	fd = socket(AF_INET, SOCK_STREAM,0);
        if (fd < 0) {
                throw ErrnoSysException("Error creating listening socket.");
        }

        /* lose the pesky "Address already in use" error message */
        if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
                throw ErrnoSysException("Error configuring socket: SO_REUSEADDR can't be set!");
        }

	/* Bind to listening port */
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(server_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	res = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)); 
	if (res){
		throw ErrnoSysException("Error binding to listening port.");
	}

	/* Listen for connections on socket */
	if (listen(fd,backlog) == -1) {
		throw ErrnoSysException("Error listening");
	}

        return fd;
}

void BaseHTTPServer::run()
{
	while(1) {

		int cli_fd;
		struct sockaddr_in cli_addr;
		socklen_t cli_addr_len = sizeof(struct sockaddr);

		cli_fd = accept(server_fd, (struct sockaddr*) &cli_addr,&cli_addr_len);
		if (-1 == cli_fd ){
			throw ErrnoSysException("Error in accept.");
		}

		handleNewClient(cli_fd, cli_addr);
	}
}

void BaseHTTPServer::handleNewClient(int cli_fd, struct sockaddr_in cli_addr)
{
	std::cout << "GOT A NEW CLIENT"<< std::endl;
	try {
		HTTPClientHandler client(this, cli_fd);
		client.go();
	} catch (std::runtime_error& msg) {
		std::cerr << "Oops! error creating http client request handler" << msg.what();
	}
}

void BaseHTTPServer::putChild(std::string path, AbstractRequestHandler* handler)
{
	TReqHandlerMap::iterator h = this->handlers.find(path);
	// Remove previouos handler for this path, if any.
	if ( h != this->handlers.end()) {
		delete h->second;
	}
	this->handlers[path] = handler;
	std::cout << "Registered " << path << " w " << handler << std::endl; // FIXME
}


/******************************************************************************
			     Some Request Handlers
 ******************************************************************************/


void StaticFileHandler::process(HTTPClientHandler& req)
{
	using http::mk_response_header;

	std::string response;

	try {
		response = mk_response_header("OK", 200, ctype);
		ManagedFilePtr file(name.c_str());
		req.write(response);
		sendfile( req.fd, file.getFileno(),
			  NULL, file.filesize());
	} catch (ErrnoSysException& e) {
		throw InternalServerErrorHTTPException(e.what());
	}
}


// EOF

