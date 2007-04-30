#include "sauron.h"

#include <sstream>
#include <iomanip>

const int Sauron::SLEEP_TIME = 10;

void* Sauron::run()
{
	docid_t down = 0;
	docid_t found = 0;
	docid_t old_down = 0;
	docid_t old_found = 0;

	while (manager.isRunning()) {
		old_down = down;
		old_found = found;
		down = manager.getDownloadCount();
		found = manager.getLastDocId();
		std::cout << old_found << " " << found << " " <<
			old_down << " " << down << std::endl;
		log <<
			"Stats : " << 
			std::dec << std::setw(10) << std::setfill('0') <<
			(down - old_down) << " downloaded, " <<
			std::dec << std::setw(10) << std::setfill('0') <<
			(found - old_found) << " found" << std::endl;
		std::cout <<
			"Stats : " << 
			std::dec << std::setw(10) << std::setfill('0') <<
			(down - old_down) << " downloaded, " <<
			std::dec << std::setw(10) << std::setfill('0') <<
			(found - old_found) << " found" << std::endl;

		sleep(SLEEP_TIME);
	}
	return (void*)this;
}

Sauron::Sauron(DeepThought& manager, std::string logfilename)
: BaseThread(), manager(manager), log(logfilename.c_str())
{
	// Turn store exceptions on
	log.exceptions( std::ios_base::badbit|std::ios_base::failbit);
	log.rdbuf()->pubsetbuf(0,0);
}
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
