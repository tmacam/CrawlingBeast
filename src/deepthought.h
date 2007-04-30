#ifndef __DEEPTHOUGHT_H
#define __DEEPTHOUGHT_H
/**@file deepthought.h
 * @brief Our all-mighty crawler
 *
 * It was previously known as laracna, the big beast that 
 * manages the other spinders (crawlers), it also
 * manages the list of known and pending URLs.
 *
 * FIXME we still handle more than plain text/html - FIXME
 * FIXME we still handle more than plain text/html - FIXME
 * FIXME we still handle more than plain text/html - FIXME
 * FIXME we still handle more than plain text/html - FIXME
 * FIXME we still handle more than plain text/html - FIXME
 * FIXME we still handle more than plain text/html - FIXME
 */

#include "common.h"
#include "threadingutils.h"
#include "domains.h"

#include <time.h>

#include <fstream>
#include <deque>
#include <ext/hash_set>
#include <ext/hash_map>
#include <queue>



/* ********************************************************************** *
				    TYPEDEFS
 * ********************************************************************** */

struct domptr_hash
{
  __gnu_cxx::hash<const char*> H;
  size_t operator()(const Domain* dom) const
  {
    return H(dom->name.c_str());
  }
};

struct eqdomptr
{
  bool operator()(const Domain* s1, const Domain* s2) const
  {
    return s1->name == s2->name;
  }
};


typedef __gnu_cxx::hash_map<std::string, Domain*,str_hash,eqstr> DomainMap;
typedef std::priority_queue<Domain*,std::vector<Domain*>,DomainPtrSmallest> DomainQueue;
typedef __gnu_cxx::hash_map<std::string, URLSet, str_hash, eqstr> DomainURLSetMap;




/* ********************************************************************** *
				  DEEP THOUGHT

			  D O N ' T    P A N I C ! ! !
 * ********************************************************************** */

/**The beast the controls URL crawling.
 */
class DeepThought : public AbstractHyperDimentionalCrawlerDeity {
	//!@name locks paranoia!
	//@{
	//!Error log access lock.
	CatholicShameMutex ERRLOG_LOCK;
	//!DocID generator lock
	CatholicShameMutex DOCID_LOCK;
	//! controls access to known_domains and domain_queue
	BigBangBabyConditional DOMAIN_LOCK; 
	//! Statistics variables' lock
	CatholicShameMutex STATS_LOCK;
	//@}
	
	/**@name Document Store variables.
	 * You know, where we store our documents
	 */
	//@{
	std::string store_dir;
	std::string store_filename;
	std::ofstream store;
	//@}
	
	//!Error log.
	std::string errlog_filename;
	std::ofstream errlog;

	//!@name Domain control
	//@{
	DomainMap known_domains;
	DomainQueue domain_queue;
	//@}
	
	docid_t last_docid;


	docid_t download_counter;


public:
	bool running;
	
	static const int MINIMUM_INTERVAL;

	/**Constructor.
	 *
         * @param store_dir The directory where we save our files
	 */
	DeepThought(std::string store_dir="/tmp/")
	: AbstractHyperDimentionalCrawlerDeity(),
	  store_dir(store_dir),
	  store_filename(store_dir + "/docids.dat"),
	  store(store_filename.c_str(), std::ios::app),
	  errlog_filename(store_dir + "/err.txt"),
	  errlog(errlog_filename.c_str(), std::ios::app),
	  known_domains(),
	  domain_queue(),
	  last_docid(0),
	  download_counter(0),
	  running(true)
	{
		// Turn store exceptions on
		store.exceptions( std::ios_base::badbit|std::ios_base::failbit);
		// FIXME set store unbuffered
		errlog.rdbuf()->pubsetbuf(0,0);
	}

	/**Get the list of known docIds/URLs from docids file.
	 *
	 * @warning You must be sure that you are the sole user of this
	 * concrete AbstractHyperDimentionalCrawlerDeity before calling
	 * this method.
	 */
	void unserialize();

	/**Enqueues pages for download.
	 *
	 * If pages are already known, nothing is done.
	 *
	 * @param urls a list of url strings.
	 * @param unserializing Are we reading URLs back from the disk?
	 *
	 * @synchronized(DOMAIN_LOCK) // domains may be updated while we read it...
	 *
	 * @note .br only domains is implemented here!
	 */
	void addPages(const URLSet& urls, bool unserializing=false);

	/**Creates a new domain, adding a group of pages to it.
	 *
	 * @param unserializing Are we reading URLs back from the disk?
	 * @return the Domain object of the new domain.
	 *
	 * @synchronized(DOMAIN_LOCK)
	 */
	Domain* addNewDomain(const std::string& domain_name,
			const URLSet& pages, bool unserializing);

	/**Checks if a given page was already downloaded.
	 *
	 * This is done verifing of a file named 'data' exists inside
	 * the path corresponding to a DocIdPath.
	 */
	bool pageExists(docid_t docid);


	/**Verifies if a given path exists
	 *
	 * It doesn't matter if it is a directory or a file.
	 */
	bool pathExists(const std::string& filename);

	std::string getDocIdPath(docid_t docid);


	/**Add a Domain instance to the download queue.
	 *
	 * @synchronized(DOMAIN_LOCK)
	 */
	void enqueueDomain(Domain* dom);

	/**Get a new docID.
	 *
	 * @synchronized(DOCID_LOCK)
	 */
	docid_t getNewDocId();


	//@synchronized(ERRLOG_LOCK)
	void reportBadCrawling(docid_t id, const std::string& url,
				const std::string& msg);


	//@synchronized(STATS_LOCK)
	void incDownloaded();

	inline time_t now() {return time(NULL); }

	inline time_t makeNextValidTimestamp() { return now() + MINIMUM_INTERVAL; }

	/**Get a page to download from the queue.
	 *
	 * From the first domain in queue, get the first page in it's queue.
	 *
	 * @return a PageRef object - it can be a Null-one, but
	 * 	   I sincerly doubt this will ever happen
	 *
	 * @synchronized(DOMAIN_LOCK)
	 */
	PageRef popPage();

	/**If needed, sleep untill it's safe to download the first domain
	 * in queue.
	 */
	void waitUntillSafeToDownload();


	/**Register a new found URL and assigns a docID to it.
	 *
	 * Registration happens by adding this URL to a file.
	 *
	 * @returns the docId assigned to the URL.
	 *
	 * @synchronized(DOCID_LOCK)
	 */
	docid_t registerURL(std::string new_url);

	/**Super-mkdir.
	 *
	 * create a leaf directory and all intermediate ones.
	 * Works like mkdir, except that any intermediate path segment (not
	 * just the rightmost) will be created if it does not exist.
	 *
	 * It ignore errors. Any problems will eventually be reported when
	 * we create the file.
	 */
	static void makedirs(std::string path);

};
/*

class StatsPrinter(Thread):
	def __init__(self,manager, log_filename):
		Thread.__init__(self)
		self.manager = manager
		self.log = open(log_filename,'w',0)

	def run(self):
		manager = self.manager
		down = 0
		found = 0
		while manager.running:
			old_down = down
			old_found = found
			down = manager.stats_down
			found = manager.docId
			msg =  "STATS : %10i downloaded, %10i found\n" % ( \
					down - old_down, found - old_found )
			self.log.write(msg)
			print msg.strip()
			time.sleep(10)
			
				
def main():
	N_OF_WORKERS = 20

	print "Starting things up..."
	boss = Laracna('/tmp/down/')
	stats = StatsPrinter(boss, boss.store_dir + "/stats")

	print "Loading data from previous invocations and from seeds"
	boss.unserialize()
	boss.addPages(['http://www.uol.com.br'])

	workers = [OfficeBoy(boss) for i in range(N_OF_WORKERS)]

	print "Dispaching officeboys"
	for w in workers:
		w.start()

	stats.start()
	
	// Stop asa soon as the user press any key
	raw_input()
	raw_input()
	boss.running = False
	boss.running = False
	boss.running = False
	print "Exiting. Waiting for threads"

	for w in workers:
		w.join()


if __name__ == '__main__':
//    import sys
//    if len(sys.argv) > 1 and sys.argv[1] != '-v':
//        pass
//    else:
//        if not _test():
    main()



*/





#endif // __DEEPTHOUGHT_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
