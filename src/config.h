// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __CONFIG_H__
#define __CONFIG_H__

#include <sys/types.h>
#include <string>


/**@file config.h
 * @brief Defines, configuration constants and such.
 *
 * @TODO Mover constantes do crawler, indexador e merger para cá.
 */

//! Port used the the HTTP server.
const int SERVER_PORT =  8090;

/**Amount of entries to reserve during a docid reading
 *
 * @see read_docid_list
 */
const size_t DOCIDLIST_RESERVE = 1<<20;


//! Number of crawler's working threads 
const int N_OF_WORKERS = 100;

const std::string CRAWLER_STORE_DIR = "/ri/tmacam/down/";

//!Number of bytes to pre-allocate in decompress() zfilebuf.
const size_t DECOMPRESS_RESERVE = 100*1024;



#endif //__CONFIG_H__


//EOF
