// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __HTTPSERVER_H__
#define __HTTPSERVER_H__
/**@file httpserver.hpp
 * @brief Simple and naïve HTTP server.
 */

#include <vector>
#include <string>
#include <ext/hash_map>
#include <tr1/functional>
#include <stdexcept>

#include <sys/types.h>
#include <sys/socket.h>

/**For ErrnoSysException.
 *
 * Although it is not used here, including it should ease the life of
 * developers - i.e., my life
 */
#include "mmapedfile.h"	

/******************************************************************************
				    TypeDefs
 ******************************************************************************/

typedef __gnu_cxx::hash_map < std::string, std::string,
                        std::tr1::hash<std::string> > StrStrMap;


/******************************************************************************
				   Exceptions
 ******************************************************************************/

//!@defgroup httpexceptions
/**@name HTTP Exceptions
 *
 * The base class for HTTP exceptions (@c HTTPException) and some commom
 * errors.
 */
//!@{

/**Base class for all HTTP-related classes.
 *
 * This class @c what()  will return an HTTP error responce text code
 * corresponding to the @c msg and @c code passed to the constructor.
 */
class HTTPException : public std::runtime_error {
public:
	/**Builds a simple HTTP response message.
	 *
	 * @param msg Error message
	 * @param code Error code. You should probably use a 5xx value here.
	 *
	 *
	 */
	static std::string make_error_page(std::string msg, int code);

	HTTPException(std::string msg, int code)
	: std::runtime_error( make_error_page(msg,code))
	{}
};

class BadRequestHTTPException :  public HTTPException {
public:
	BadRequestHTTPException()
	: HTTPException("Bad request", 400)
	{}
};

class NotFoundHTTPException :  public HTTPException {
public:
	NotFoundHTTPException()
	: HTTPException("Not Found", 404)
	{}
};

/**@todo we could include the original exceptio what() in the content
 * of this error.
 */
class InternalServerErrorHTTPException :  public HTTPException {
public:
	InternalServerErrorHTTPException()
	: HTTPException("Internal Server Error", 500)
	{}
};

//!@}

/******************************************************************************
			     AbstractRequestHandler
 ******************************************************************************/


class HTTPClientHandler; // Forward declaration

/**This "thing" knows how to handle a HTTP request from a client.
 *
 * Besides BaseHTTPServer, any other object implementing this
 * interface can handle HTTP requests. 
 *
 * Errors during a request processing should be signaled by throwing an
 * HTTPException or one of it's descendents.
 */
struct AbstractRequestHandler{
	virtual void process(HTTPClientHandler& req) = 0;

	virtual ~AbstractRequestHandler(){};
};

/******************************************************************************
			       HTTPClientHandler
 ******************************************************************************/



/**Simple class to parse a HTTP request and handle client connection.
 *
 * This class is losely based on Twisted's Transport. All it does is parse a
 * HTTP request and provide methods for writing a response back. It also
 * control's the use and lifetime of the client socket.
 *
 * This class it can be used in a threading enviroment - it has no assumption
 * on threading model whatsoever. This happens because the real processing of
 * the request is delegated to a @c AbstractRequestHandler instance, passed to
 * @c HTTPClientHandler constructor.
 *
 * Also observe that error ocurred in the @p handler should be signaled with
 * @c HTTPException. See @c AbstractRequestHandler for more information on
 * error signaling.
 *
 * @see BaseHTTPServer
 * @see AbstractRequestHandler
 */
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

	std::string method;	  //!< The HTTP method used by this client.
	std::string uri;	  //!< The URI used in this client's request.
	std::string proto_version;/**< HTTP protocol version used in 
				   *   the request.
				   */
	
	StrStrMap headers;	  //!< HTTP request headers

	AbstractRequestHandler* handler; /**< The request handler that will be
					  *   used to handle this client
					  *   request
					  */

	/**Constructor.
	 *
	 * @param handler The request handler for this client.
	 * @param cli_fd  The client's connection socket.
	 *
	 */
	HTTPClientHandler(AbstractRequestHandler* handler, int cli_fd)
	: fd(cli_fd) , handler(handler) 
	{}

	~HTTPClientHandler()
	{
		loseConnection();
	}

	/**Handle this client.
	 *
	 * This method instructs the instance to parse and handle
	 * the requests made by the client.
	 *
	 * Error handling and reporting is done here as well.
	 */
	void go();
	
	/**Parses the HTTP request.
	 *
	 * Read the HTTP method, URI, HTTP version and request headers
	 * sent by the client. Invalid request will raise an 
	 * @c BadRequestHTTPException()
	 *
	 */
	void parseRequest();

	/**Send data to the client.
	 *
	 * All data content's is sent - or an ErrnoSysException will be thrown.
	 *
	 * If desired - in should be in most cases, handlers should convert
	 * this error into a 5xx, i.e. Internal Error, HTTP Exception.
	 *
	 */
	void write(const std::string data);
	

	/**Close the connection.
	 *
	 * Multiple calls to this functin @e should be handled gracefuly.
	 *
	 * This is implictly called by this class destructor.
	 */
	void loseConnection();

};

/******************************************************************************
				 BaseHTTPServer
 ******************************************************************************/

/**Simple and naïve HTTP server and HTTP request handler.
 *
 * This class deals with to problems: managing a listening socket for a HTTP
 * server and being the default request handler for the its clients.
 *
 * This class constructor will only setup the listening socket. You must
 * call its @c run() method to make it do what it is supposed to do.
 *
 *
 */
struct BaseHTTPServer : public AbstractRequestHandler {

	int server_fd; //!< Server socket file-descriptor.

	BaseHTTPServer(uint16_t server_port, int backlog = 10)
	: server_fd(setupServerSocket(server_port, backlog))
	{}

	virtual ~BaseHTTPServer(){}

	static int setupServerSocket(uint16_t server_port, int backlog = 10);

	/**Default request handler.
	 *
	 * You should redefine it!
	 *
	 */
	virtual void process(HTTPClientHandler& req);

	/**Accept connections from and deal with new clients.
	 *
	 * Can throw ErrnoSysException if an error happens while calling
	 * @c accept().
	 */
	void run();

	/**Handle a new client.
	 *
	 * You can re-define this class if you want to make it multithreading.
	 *
	 */
	virtual void handleNewClient(int cli_fd, struct sockaddr_in cli_addr);
};

#endif // __HTTPSERVER_H__

//EOF
