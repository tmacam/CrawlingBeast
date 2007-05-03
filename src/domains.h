#ifndef __DOMAINS_H
#define __DOMAINS_H
/**@file domains.h
 * @brief domain abstraction.
 *
 */

#include <string>
#include <deque> // FIXME
#include "time.h"

#include "threadingutils.h"
#include "urltools.h"
#include "pagedownloader.h"
#include "robotshandler.h"


/* ********************************************************************** *
				    TYPEDEFS
 * ********************************************************************** */

//typedef __gnu_cxx::hash_set<std::string,str_hash,eqstr> StrSet; // FIXME UNUSED


typedef RobotsParser::rule_t rule_t;
typedef RobotsParser::robots_rules_t robots_rules_t;




/* ********************************************************************** *
                                    DOMAIN
 * ********************************************************************** */

/**Encapsulates our notion of something that about a domain.
 * 
 * Domain objects control certain domain's URL lists:
 *  - known URL, already downloaded or not
 *  - pending URLs that must be downloaded.
 *  - robots.txt rules
 *
 *  Robots rules are tested in popPage().
 */
class Domain{
protected:
	CatholicShameMutex PAGES_LOCK;

	URLSet known_pages;
	std::deque<PageRef> pages_queue;

	AbstractHyperDimentionalCrawlerDeity& manager;

	bool got_robots;
	docid_t robots_docid;
	robots_rules_t rules;
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
	: known_pages(), pages_queue() , manager(manager),
	  got_robots(false), robots_docid(0), rules(), name(name),
	  in_queue(false), timestamp(0)
	{
		// FIXME if we had a url->docid map we could
		// FIXME see if we already downloaded the robots.txt
		// FIXME file...

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
				if ( not unserializing ) {
					// This page must be enqueued
					std::string url_str = p->str();
					docid_t id = manager.registerURL(
								url_str);
					pages_queue.push_back(
							PageRef(url_str, id));
				}
			}
		}
	}

	bool empty() { return pages_queue.empty(); }

	/**Get a page from the queue.
	 *
	 * @throw GetRobotsForMePlzException
	 *
	 * @see checkRobotsFile
	 *
	 * @synchronized(PAGES_LOCK)
	 */
	PageRef popPage()
	{
		AutoLock synchronized(PAGES_LOCK);

		checkRobotsFile();

		while (not pages_queue.empty()) {
			PageRef p = pages_queue.front();
			pages_queue.pop_front();
			if ( allowedByRobotsTxt(p) ) {
				return p;
			}
		}

		// If we got here then no suitable page was found...
		return PageRef();
	}

	/**Vefiries if a given URL is allowed by the domains
	 * robots.txt file.
	 *
	 * We expect that checkRobotsFile was called before
	 */
	bool allowedByRobotsTxt(const PageRef& page);


	/** Verifies we have downloaded the domains robots.txt .
	 * 
	 * If the domain still has got no robot rules, then 
	 * it is not possible to download from this domain.
	 * Being this the case, a GetRobotsForMePlzException
	 * will be raised, signaling the caller that a robot.txt
	 * file must be obtained.
	 */
	void checkRobotsFile();

	/**Register rules found in the robots.txt of this domain.
	 *
	 * @synchronized PAGES_LOCK
	 */
	void setRobotsRules(robots_rules_t newrules)
	{
		AutoLock synchronized(PAGES_LOCK);

		if (not got_robots) {
			// We may have called this method before...
			this->rules = newrules;
			got_robots = true;
		}
	}

	// FIXME We probably should've used a lock here
	int queueLength() const { return pages_queue.size(); }

};        

/**Used to make a min-priority queue.
 * 
 * Notice: priority queues are designed to find the element with the
 * "HIGHEST priority". In our case, this is the Domain with the
 * SMALLEST timestamp.
 */
struct DomainPtrSmallest {
	inline bool operator()(const Domain* a, const Domain* b) const
	{
		// TRUE -> B has a higher priority
		if (a->timestamp == b->timestamp) {
			// If both can be crawled, the preffered one is the
			// one with the biggest queue
			return a->queueLength() < b->queueLength();
		} else {
			return a->timestamp > b->timestamp;
		}
	}
};

/* ********************************************************************** *
				   EXCEPTIONS
 * ********************************************************************** */

/**A robot.txt file must be obtained.
 *
 * This exception is used to inform the caller of popPage (and checkRobotsFile)
 * that a robot.txt file must be obtained.
 */
class GetRobotsForMePlzException: public std::runtime_error {
public:
	/**To which domain should the caller report back after obtaining
	 * a robots.txt file.
	 */
	Domain* domain;
	/**Information about the robots.txt file */
	PageRef page;

	GetRobotsForMePlzException(const std::string msg="", Domain* dom = 0,
		PageRef page=PageRef())
	: std::runtime_error(msg), domain(dom), page(page) {}

	~GetRobotsForMePlzException() throw() {}
};


#endif // __DOMAINS_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
