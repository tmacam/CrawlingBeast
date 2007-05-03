#include "domains.h"

void Domain::checkRobotsFile()
{
	if (got_robots){
		// We alreadyd downloaded a robots .txt file
		return;
	}

	std::string robots_url("http://");
	robots_url.append(name);
	robots_url.append("/robots.txt");

	known_pages.insert(robots_url);

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

bool Domain::allowedByRobotsTxt(const PageRef& page)
{
	robots_rules_t::const_iterator i;
	BaseURLParser url(page.first);

	for(i = rules.begin(); i != rules.end(); ++i) {
		if (startswith(url.path, i->first) ) {
			std::cout << "DEBUG allowedByRobotsTxt " <<
				page.first << "  match, res = " <<
				i->second << std::endl;
			// Houston, we've got a match!
			return i->second;
		}
	}

	// Default behaviour is to allow
	return true;
}


// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
