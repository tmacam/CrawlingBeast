#ifndef __SAURON_H
#define __SAURON_H
/**@file sauron.h
 * @brief O cara do olhão
 *
 * Statistics class.
 */

#include "deepthought.h"
#include "threadingutils.h"

class Sauron : public BaseThread {
	
	DeepThought& manager;
	std::ofstream log;
public:
	static const int SLEEP_TIME;
	Sauron(DeepThought& manager, std::string logfilename);
	void* run();
};



#endif // __SAURON_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
