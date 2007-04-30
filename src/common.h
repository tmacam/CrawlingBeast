#ifndef __COMMOM_H
#define __COMMOM_H
/**@file commom.h
 * @brief Commom definitions, typedefs and forward declarations.
 */

#include <sys/types.h>
#include <set>
#include <utility>

typedef u_int64_t docid_t;

//!A simple struct-like class just to store information about a page.
typedef std::pair<std::string, docid_t> PageRef;


/**It has been said that (s)he is the one that controlls the crawlers.
 *
 * @see DeepThought
 */
struct AbstractHyperDimentionalCrawlerDeity{
	virtual docid_t registerURL(std::string new_url) = 0;

	virtual ~AbstractHyperDimentionalCrawlerDeity(){}

	virtual PageRef popPage() = 0;
	virtual bool isRunning() = 0;
};


//#include <iostream>
//
//#define BEEN_HERE do{ std::cout << "BEEN HERE\n" }while(0)

#endif // __COMMOM_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
