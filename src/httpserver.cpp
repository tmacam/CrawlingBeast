#include <iostream>
#include <vector>
#include <string>
#include <ext/hash_map>
#include <tr1/functional>
#include <stdexcept>


#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>		// htons
#include <strings.h>		// bzero

#include <stdlib.h>		// exit

// FOR perror
#include <stdio.h>
#include <errno.h>

#include "strmisc.h"

typedef __gnu_cxx::hash_map < std::string, std::string,
                        std::tr1::hash<std::string> > StrStrMap;

const uint16_t SERVER_PORT = 8090;



int setupServerSocket(uint16_t server_port, int backlog = 10)
{
	int fd = -1;
	int res = 0;
	int yes = 1;
	struct sockaddr_in addr;


	/* Socket creation */
	fd = socket(AF_INET, SOCK_STREAM,0);
        if (fd < 0) {
                perror("Error creating listening socket.");
                return -1;
        }

        /* lose the pesky "Address already in use" error message */
        if (setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) {
                perror("Error configuring socket: SO_REUSEADDR can't be set!");
                return -1;
        }

	/* Bind to listening port */
	bzero(&addr, sizeof(struct sockaddr_in));
	addr.sin_family = AF_INET;
	addr.sin_port = ntohs(server_port);
	addr.sin_addr.s_addr = INADDR_ANY;
	res = bind(fd, (struct sockaddr *)&addr, sizeof(struct sockaddr)); 
	if (res){
		perror("Error binding to listening port.");
		return -1;
	}

	/* Listen for connections on socket */
	if (listen(fd,backlog) == -1) {
		perror("Error listening");
		return -1;
	}

        return fd;

}

/******************************************************************************
			       HTTPClientHandler
 ******************************************************************************/

class HTTPClientHandler{
	
	//!This class is non-copyable
        HTTPClientHandler(const HTTPClientHandler&);
        //!This class is non-copyable
        HTTPClientHandler& operator=(const HTTPClientHandler&);

public:
	typedef std::vector<std::string> strvec_t;

	static const std::string CRLF;
	static const std::string CRLF_CRLF;

	int fd;		//!< Client socket file descriptor

	std::string method;
	std::string uri;
	std::string proto_version;
	StrStrMap headers;

	HTTPClientHandler(int cli_fd)
	: fd(cli_fd) 
	{
		std::cout << "GOT A NEW CLIENT"<< std::endl;
		handleRequest();

		std::cout << "Method: " << method << " URI: " << uri << " version:" << proto_version << std::endl;
		StrStrMap::const_iterator hdr;
		for (hdr = headers.begin(); hdr != headers.end(); ++hdr){
			std::cout << "\t" << hdr->first << " == " <<
				hdr->second << std::endl;
		}
	}

	~HTTPClientHandler()
	{
		close(fd);
		std::cout << "Client disconected" << std::endl;
	}

	void handleRequest()
	{
		const int BUF_LEN = 1024;
		char buf[BUF_LEN];
		ssize_t n;
		std::string msg;

		std::string CRLF_CRLF(CRLF);
		CRLF_CRLF += CRLF;


		while( 0 < (n = recv(fd,buf, BUF_LEN, 0)) ) {
			msg.append(buf, n);
			std::cout << "\tMSG LEN " << n << std::endl;

			// Get out if found the request end
			if (msg.find(CRLF_CRLF)) break;
		}

		// parse request
		if (msg.empty()) {
			throw std::runtime_error("Invalid request");
		}
		strvec_t fullreq_lines(split(msg,CRLF));
		strvec_t::iterator line = fullreq_lines.begin();
		strvec_t request_fields(split(*line," ",2));
		if (request_fields.size() != 3) {
			throw std::runtime_error("Invalid request line");
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

};

const std::string HTTPClientHandler::CRLF =  "\x0d\x0a";
const std::string HTTPClientHandler::CRLF_CRLF =  "\x0d\x0a\x0d\x0a";



int main(int argc, char* argv[])
{
	int fd;
	
	if (-1 == (fd = setupServerSocket(SERVER_PORT)) ) {
		std::cerr << "Could not setup server socket. Exiting" <<
			std::endl;
		exit(EXIT_FAILURE);
	}

	while(1) {

		int cli_fd;
		struct sockaddr_in cli_addr;
		socklen_t cli_addr_len = sizeof(struct sockaddr);

		cli_fd = accept(fd, (struct sockaddr*) &cli_addr,&cli_addr_len);
		if (-1 == cli_fd ){
			perror("Error in accept.");
			exit(EXIT_FAILURE);
		}
		HTTPClientHandler client(cli_fd);
	}

}
