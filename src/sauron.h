#ifndef __SAURON_H
#define __SAURON_H
/**@file sauron.h
 * @brief The eye without lids, lord of the evil and statistics
 *
 * Statistics class.
 */

#include "deepthought.h"
#include "threadingutils.h"

/**@brief The eye without lids, lord of the evil and statistics.
 *
 * This is our "statistics gathering and reporting" class.
 */
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
