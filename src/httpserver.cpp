// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#include "httpserver.hpp"

#include <sstream>

#include <arpa/inet.h>		// htons
#include <strings.h>		// bzero

#include "strmisc.h"

#include <iostream>		// FIXME temporary

/******************************************************************************
				   Exceptions
 ******************************************************************************/

std::string HTTPException::make_error_page(std::string msg, int code)
{
	std::ostringstream buf;
	const std::string CRLF = "\x0d\x0a";

	buf <<	"HTTP/1.0 " << code <<  msg << CRLF <<
		"Server: ProcastinationBroadcaster/0.1" << CRLF <<
		"Connection: close" << CRLF << 
		"Content-Type: text/html; charset=iso-8859-1" << CRLF << 
		CRLF << 
		"<!DOCTYPE HTML PUBLIC \"-//IETF//DTD HTML 2.0//EN\">" << CRLF << 
		"<html><head>" << CRLF << 
		"<title>" << code << " " << msg << "</title>" << CRLF << 
		"</head><body>" << CRLF << 
		"<h1>" << msg <<  "</h1>" << CRLF << 
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
		if (msg.find(CRLF_CRLF)) break;
	}

	// parse request
	if (msg.empty()) {
		throw BadRequestHTTPException();
	}
	strvec_t fullreq_lines(split(msg,CRLF));
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

const std::string HTTPClientHandler::CRLF =  "\x0d\x0a";
const std::string HTTPClientHandler::CRLF_CRLF =  "\x0d\x0a\x0d\x0a";


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

	throw NotFoundHTTPException();

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



// EOF

