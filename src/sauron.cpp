#include "sauron.h"

#include <sstream>
#include <iomanip>

const int Sauron::SLEEP_TIME = 10;

void* Sauron::run()
{
	int queue_len = 0;
	time_t now = 0;
	time_t time_left=0;
	crawl_stat_t stats;
	crawl_stat_t old_stats;

	while (manager.isRunning()) {
		now = time(NULL);

		old_stats = stats;
		stats = manager.getCrawlingStats();

		if (stats.next_ts < now) {
			time_left = 0;
		} else {
			time_left = (stats.next_ts - now);
		}

		queue_len = stats.n_active + stats.n_idle;
		log << now << 
			" s " << stats.seen - old_stats.seen << 
				" / " << stats.seen <<
			" c " << stats.crawled - old_stats.crawled <<
				" / " << stats.crawled <<
			" d " << stats.downloaded - old_stats.downloaded <<
				" / " << stats.downloaded <<
			" q " <<  queue_len << " / " << stats.n_domains <<
			" t " << time_left <<
			" i " << stats.n_idle <<
			" a " << stats.n_active <<
			std::endl;
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
