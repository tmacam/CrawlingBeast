#include "domains.h"

Domain::Domain(std::string name,const URLSet& pages,
	AbstractHyperDimentionalCrawlerDeity& manager,
	bool unserializing)
: known_pages(), pages_queue() , manager(manager),
  got_robots(false), robots_docid(0), rules(), name(name),
  in_queue(false), timestamp(0), previous_queue_length(0)
{
	// FIXME if we had a url->docid map we could
	// FIXME see if we already downloaded the robots.txt
	// FIXME file...

	// Add initial set of known pages
	addPages(pages,unserializing);
}



void Domain::addPages(const URLSet& pages, bool unserializing)
{
	AutoLock synchronized(PAGES_LOCK);

	URLSet::const_iterator p;


	for(p = pages.begin(); p != pages.end(); ++p) {
		// We just add paths to our structures..
		const std::string& path = p->path;
		// Unknown page?
		if (known_pages.count(path) == 0) {
			known_pages.insert(path);
			if ( not unserializing ) {
				// This page must be enqueued
				std::string url_str = p->str();
				docid_t id = manager.registerURL(
							url_str);
				pages_queue.push_back( PathRef(path, id) );
			}
		}
	}
}


PageRef Domain::popPage()
{
	AutoLock synchronized(PAGES_LOCK);

	checkRobotsFile();

	while (not pages_queue.empty()) {
		// Get a page from the queue
		PathRef p = pages_queue.front();

		std::string& path = p.first;
		docid_t& id = p.second;

		pages_queue.pop_front();
		if ( allowedByRobotsTxt(p.first) ) {
			return PageRef( "http://" + this->name + path, id);
		}
	}

	// If we got here then no suitable page was found...
	return PageRef();
}

void Domain::setRobotsRules(robots_rules_t newrules)
{
	AutoLock synchronized(PAGES_LOCK);

	if (not got_robots) {
		// We may have called this method before...
		this->rules = newrules;
		got_robots = true;
	}
}



void Domain::checkRobotsFile()
{
	if (got_robots){
		// We alreadyd downloaded a robots .txt file
		return;
	}

	std::string robots_url("http://");
	robots_url.append(name);
	robots_url.append("/robots.txt");

	known_pages.insert("/robots.txt");

	if (robots_docid == 0) {
		// Ok, first time in this function?
		// Ok, this is how it goes.
		// We get a docid for the robots.txt file
		// XXX Beware of possible dead-locks in DOCID_LOCK
		robots_docid = manager.registerURL(robots_url);
	}
	throw GetRobotsForMePlzException("GET ROBOTS FIRST", this,
			PageRef(robots_url,robots_docid));

}

bool Domain::allowedByRobotsTxt(const std::string& path)
{
	robots_rules_t::const_iterator i;

	for(i = rules.begin(); i != rules.end(); ++i) {
		if (startswith(path, i->first) ) {
			//std::cout << "DEBUG allowedByRobotsTxt " <<
			//	path << "  match, res = " <<
			//	i->second << std::endl;

			// Houston, we've got a match!
			return i->second;
		}
	}

	// Default behaviour is to allow
	return true;
}


// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
