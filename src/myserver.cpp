// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "httpserver.hpp"
#include "config.h"


/******************************************************************************
				      Main
 ******************************************************************************/



int main(int argc, char* argv[])
{
	BaseHTTPServer server(SERVER_PORT);
	server.run();

//        int fd;
//        
//        if (-1 == (fd = setupServerSocket(SERVER_PORT)) ) {
//                std::cerr << "Could not setup server socket. Exiting" <<
//                        std::endl;
//                exit(EXIT_FAILURE);
//        }
//
//        while(1) {
//
//                int cli_fd;
//                struct sockaddr_in cli_addr;
//                socklen_t cli_addr_len = sizeof(struct sockaddr);
//
//                cli_fd = accept(fd, (struct sockaddr*) &cli_addr,&cli_addr_len);
//                if (-1 == cli_fd ){
//                        perror("Error in accept.");
//                        exit(EXIT_FAILURE);
//                }
//                HTTPClientHandler client(cli_fd);
//        }

}
