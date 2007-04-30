#ifndef __PARANOIDANDROID_H
#define __PARANOIDANDROID_H
/**@file paranoidandroid.h
 * @brief Our not so humble and not so enthusiastic crawling unit.
 *
 *
 * "Where's my mind?" - Pixies
 */

#include "common.h"
#include "threadingutils.h"
#include "deepthought.h"
#include "pagedownloader.h"

/**Our depressed crawling unit.
 *
 * "Life? Don't talk about life!" - Marvin, our memorable Para. Andr.
 */
class ParanoidAndroid: public BaseThread{
	/**We are supposed to worship this dude - and report
	 * our activities back to it.
	 */
	DeepThought& manager;

	//!Turn on exceptions and disables buffering
	void setupOfstream(std::ostream& stream);
	void safePageAndMetadata(docid_t docid, PageDownloader& d);

public:

	ParanoidAndroid(DeepThought& manager):
	BaseThread(), manager(manager) {}

	void* run();
};

#endif // __PARANOIDANDROID_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
