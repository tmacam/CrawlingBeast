#ifndef __DOMAINS_H
#define __DOMAINS_H
/**@file domains.h
 * @brief domain abstraction.
 *
 * FIXME we still handle more than plain text/html - FIXME
 * FIXME we still handle more than plain text/html - FIXME
 * FIXME we still handle more than plain text/html - FIXME
 * FIXME we still handle more than plain text/html - FIXME
 * FIXME we still handle more than plain text/html - FIXME
 * FIXME we still handle more than plain text/html - FIXME
 */

#include <string>
#include <ext/hash_set>
#include <deque>
#include "time.h"

#include "threadingutils.h"
#include "urltools.h"



/* ********************************************************************** *
				    TYPEDEFS
 * ********************************************************************** */

struct eqstr
{
  bool operator()(const std::string& s1, const std::string& s2) const
  {
    return s1 == s2;
  }
};


struct str_hash
{
  __gnu_cxx::hash<const char*> H;
  size_t operator()(const std::string& s1) const
  {
    return H(s1.c_str());
  }
};

struct url_path_hash
{
  __gnu_cxx::hash<const char*> H;
  size_t operator()(const BaseURLParser& s1) const
  {
    return H(s1.path.c_str());
  }
};

struct equrl
{
  bool operator()(const BaseURLParser& s1, const BaseURLParser& s2) const
  {
    return s1 == s2;
  }
};


typedef __gnu_cxx::hash_set<std::string,str_hash,eqstr> StrSet;

typedef __gnu_cxx::hash_set<BaseURLParser,url_path_hash,equrl> URLSet;

//!A simple struct-like class just to store information about a page.
typedef std::pair<std::string, docid_t> PageRef;



/* ********************************************************************** *
                                    DOMAIN
 * ********************************************************************** */

/**Encapsulates our notion of something that about a domain.
 * 
 * Domain objects control certain domain's URL lists:
 *  - known URL, already downloaded or not
 *  - pending URLs that must be downloaded.
 */
class Domain{
	CatholicShameMutex PAGES_LOCK;

	URLSet known_pages;
	std::deque<PageRef> pages_queue;

	AbstractHyperDimentionalCrawlerDeity& manager;
public:
	std::string name;
	
	/**used by AbstractHyperDimentionalCrawlerDeity to control whether
	 * or not this domain is in the queue
	 * or not.
	 */
	bool in_queue;

	//!The last time this domain was crawled.
	time_t timestamp;

	/**Constructor.
	 *
	 * @param name The name of this domain.
	 * @param pages Initial set of known pages to this domain.
	 * @param manager Reference DeepThought or to a concrete
	 *	  AbstractHyperDimentionalCrawlerDeity&  instance.
	 * @param unserializing Are we reading URLs back from the disk?
	 */
	Domain(std::string name,const URLSet& pages,
		AbstractHyperDimentionalCrawlerDeity& manager,
		bool unserializing)
	: known_pages(), pages_queue() , manager(manager), name(name),
	  in_queue(false), timestamp(0)
	{
		// Add initial set of known pages
		addPages(pages,unserializing);
	}

	/**Add pages for this domain.
	 *
	 * Add pages to the list of known pages and enque the previously
	 * unknown ones to future download.
	 *
	 * @param pages A list of URLs (str)
	 *
	 * @synchronized(PAGES_LOCK)
	 */
	void addPages(const URLSet& pages, bool unserializing=false)
	{
		AutoLock synchronized(PAGES_LOCK);

		URLSet::const_iterator p;

		for(p = pages.begin(); p != pages.end(); ++p) {
			// Unknown page?
			if (known_pages.count(*p) == 0) {
				known_pages.insert(*p);
				if (not unserializing and allowedByRobotsTxt(*p)) {
					std::string url_str = p->str();
					docid_t id = manager.registerURL(url_str);
					pages_queue.push_back(PageRef(url_str, id));
				}
			}
		}
	}

	bool empty() { return pages_queue.empty(); }

	//!@synchronized(PAGES_LOCK)
	PageRef popPage()
	{
		AutoLock synchronized(PAGES_LOCK);

		if (not pages_queue.empty()) {
			PageRef p = pages_queue.front();
			pages_queue.pop_front();
			return p;
		} else {
			return PageRef();
		}
	}

	//FIXME not implemented 
	bool allowedByRobotsTxt(const BaseURLParser& urlstr) { return true; }

};        

//!Used to make a min-priority queue
struct DomainPtrSmallest {
	inline bool operator()(const Domain* a, const Domain* b) const
	{
		return a->timestamp > b->timestamp;
	}
};



#endif // __DOMAINS_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
