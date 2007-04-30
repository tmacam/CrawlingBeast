#ifndef __COMMOM_H
#define __COMMOM_H
/**@file commom.h
 * @brief Commom definitions, typedefs and forward declarations.
 */

#include <sys/types.h>
#include <set>

typedef u_int64_t docid_t;

struct AbstractHyperDimentionalCrawlerDeity{
	virtual docid_t registerURL(std::string new_url) = 0;

	virtual ~AbstractHyperDimentionalCrawlerDeity(){}
};


//#include <iostream>
//
//#define BEEN_HERE do{ std::cout << "BEEN HERE\n" }while(0)

#endif // __COMMOM_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
