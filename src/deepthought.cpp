#include "deepthought.h"

#include <sstream>
#include <iomanip>


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include "strmisc.h"


const int DeepThought::MINIMUM_INTERVAL = 30;



void DeepThought::unserialize()
{
	URLSet retrieved;
	URLSet pending;

	// open the document with the list of known URLs
	std::ifstream docids_file(store_filename.c_str());
	docid_t id;
	std::string url_str;

	while(docids_file >> id >> url_str ) {
		BaseURLParser url(url_str);
		/*FIXME We should not need to "strip" the URLS
		 * 	because it's supposed that they were
		 * 	striped upon registration anyway.
		 * 	But...
		 */
		url.strip();
		if (pageExists(id)) {
			retrieved.insert(url);
		} else {
			pending.insert(url);
		}
	}
	addPages(retrieved,true); // unserializing
	addPages(pending,false); // not unserializing - adding pending pages
}

//@synchronized(DOMAIN_LOCK) // domains may be updated while we read it...
void DeepThought::addPages(const URLSet& urls, bool unserializing)
{
	AutoLock synchronized(DOMAIN_LOCK);

	URLSet::const_iterator u;
	DomainURLSetMap::iterator d;

	// Group pages by domain
	DomainURLSetMap domains; // It seemed so much nicer in python...
	for(u = urls.begin(); u != urls.end(); ++u){
		domains[u->host].insert(*u);
	}

	// Add pages per domain:
	for(d = domains.begin(); d != domains.end(); ++d) {
		const std::string& domainname = d->first;
		URLSet& pages = d->second;
		if (not endswith(domainname, ".br")) {
			//ignore non-br domains
			continue;
		}
		// New domain...
		if ( known_domains.find(domainname) == known_domains.end() ) {
			addNewDomain(domainname, pages, unserializing);
			// pages added by Domains constructor,
			// domain was put in queue by _addNewDomain
		} else {
			Domain* dom = known_domains[domainname];
			dom->addPages(pages, unserializing);
			enqueueDomain(dom);
		}
	}
}

//@synchronized(DOMAIN_LOCK)
Domain* DeepThought::addNewDomain(const std::string& domain_name, const URLSet& pages, bool unserializing)
{
	//XXX This function is only called by addPages,
	//XXX that already holds DOMAIN_LOCK
	//XXX AutoLock synchronized(DOMAIN_LOCK);
	
	Domain* dom = new Domain(domain_name, pages, *this, unserializing);
	dom->timestamp = makeNextValidTimestamp();
	known_domains[domain_name] = dom;
	enqueueDomain(dom);
	return dom;
}

bool DeepThought::pageExists(docid_t docid)
{
	std::string filename = getDocIdPath(docid) + PAGE_DATA_PREFIX;
	
	return pathExists(filename);
}

bool DeepThought::pathExists(const std::string& filename)
{
	struct stat _statbuf;

	if (stat(filename.c_str(),&_statbuf) == 0) {
		return true;
	} else {
		return false;
	}
}


std::string DeepThought::getDocIdPath(docid_t docid)
{
	std::ostringstream id_hex;

	// FIXME it should be setw(4) - docid_t is a uint32_t now
	id_hex << std::uppercase << std::hex << std::setw(8) <<
                std::setfill('0') << docid;
	std::string id_hex_str = id_hex.str();

	std::string id_path =  store_dir + "/" + 
				id_hex_str.substr(0,2) + "/" +
				id_hex_str.substr(2,2) + "/" +
				id_hex_str.substr(4,2) + "/" +
				id_hex_str.substr(6,2) + "/";
	return id_path;
}

//@synchronized(DOMAIN_LOCK)
void DeepThought::enqueueDomain(Domain* dom)
{
	//XXX This function is only called by addPages, and addNewDomain
	//XXX that already holds DOMAIN_LOCK
	//XXX AutoLock synchronized(DOMAIN_LOCK);

	bool was_empty = false;
	if (domainQueuesEmpty()) {
		was_empty = true;
	}
	// should this domain really be enqueued?
	if (not dom->empty() and not dom->in_queue) {
		dom->in_queue = true;
		// our hopes and expectations
		// black holes and revelations
		idle_domain_queue.push_back(dom);
	}
	// If the list of domains in queue was empty,  there may be threads
	// waiting to be notified
	if (was_empty) DOMAIN_LOCK.notifyAll();
}

//@synchronized(DOCID_LOCK)
docid_t DeepThought::getNewDocId()
{
	AutoLock synchronized(DOCID_LOCK);

	return ++last_docid;
}

//@synchronized(DOCID_LOCK)
docid_t DeepThought::getLastDocId()
{
	AutoLock synchronized(DOCID_LOCK);

	return this->last_docid;
}


//@synchronized(ERRLOG_LOCK)
void DeepThought::reportBadCrawling(docid_t id, const std::string& url,
				    const std::string& msg)
{
	AutoLock synchronized(ERRLOG_LOCK);

	errlog << now() << " " << id << "\t" << url << "\t" << msg << std::endl;
}

/**Increment the crawled (and downloaded) pages counter(s)
 *
 * @param downloaded Pass true to report successful download and parsing
 * 		     of a page.
 *
 * @synchronized(STATS_LOCK)
 */
void DeepThought::incCrawled(bool downloaded, docid_t id, const std::string& url)
{
	AutoLock synchronized(STATS_LOCK);

	++crawled_counter;

	if (downloaded) {
		++download_counter;
		crawllog << now() << " DOWN\t" << id << "\t"<< url << std::endl;
	} else {
		crawllog << now() << " CRAW\t" << id << "\t"<< url << std::endl;
	}
}


//@synchronized(STATS_LOCK)
//@synchronized(DOMAIN_LOCK)
crawl_stat_t DeepThought::getCrawlingStats()
{
	AutoLock synchronized(STATS_LOCK);
	AutoLock synchronized_domains(DOMAIN_LOCK);

	time_t next_ts = 0;
	if (not idle_domain_queue.empty()) {
		next_ts = idle_domain_queue.front()->timestamp;
	}

	return crawl_stat_t(  getLastDocId(), crawled_counter, download_counter,
				known_domains.size(),active_domain_queue.size(),
				idle_domain_queue.size(), next_ts);
}


//@synchronized(DOMAIN_LOCK)
PageRef DeepThought::popPage()
{
	AutoLock synchronized(DOMAIN_LOCK);

	PageRef page;
	Domain* dom = 0;

	// print currentThread(), "popPage", "WAIT" # DEBUG
	while (domainQueuesEmpty()) {
		DOMAIN_LOCK.wait(); 
	}
	if ( not domainQueuesEmpty() ) {
		// Queue management...
		/* Move elegible domains from the idle queue
		 * This first refresh MUST be done despite the loop
		 * bellow because there may be "better" and elegible domains
		 * in the idle queue that otherwise would not be crawled until
		 * the active queue was empty.
		 */
		refreshActiveDomainQueue();
		while(active_domain_queue.empty()) {
			/* To avoid sleep/timing issues, we will do this
			 * inside a loop
			 */
			waitUntillSafeToDownload();
			refreshActiveDomainQueue();
		}
		assert( not active_domain_queue.empty());

		dom = active_domain_queue.top();
		active_domain_queue.pop();

		dom->timestamp = makeNextValidTimestamp();

		try{
			page = dom->popPage();
		} catch(GetRobotsForMePlzException) {
			// Ok, I got it, we will ask
			// someone else to get the robots.txt
			// for you. Just rest for now.
			idle_domain_queue.push_back(dom);
			throw;
		}

		// Deal with empty domains
		if ( dom->empty() ) {
			//std::cerr << "DEBUG DeepThought popPage removed domain " << dom->name << std::endl;
			dom->in_queue = false;
		} else {
			// non-empty domains should be added back to the queue
			idle_domain_queue.push_back(dom);
		}
	}

	return page;
}

void DeepThought::waitUntillSafeToDownload()
{
	int time_left = 0;
	Domain* dom = 0;

	if (active_domain_queue.empty()) {
		// Sanity check - there must be idle domains....
		if ( idle_domain_queue.empty()) {
			throw std::runtime_error("No domains left to crawl!");
		}

		dom = idle_domain_queue.front();

		time_left = dom->timestamp - now();
		if (time_left > 0) {
			sleep(time_left + 1); // +1 just to avoid timing issues
		}
	} // else, there is no reason to wait here...
}

docid_t DeepThought::registerURL(std::string new_url)
{
	docid_t new_id = getNewDocId();
	//FIXME shouldn't we have a lock just for this file?
	store << new_id << "\t" << new_url << std::endl;

	return new_id;
}


void DeepThought::makedirs(std::string path)
{
	std::vector<std::string>::iterator i;
	std::string fullpath;

	std::vector<std::string> segments = split(path,"/");

	for(i = segments.begin(); i != segments.end(); ++i) {
		// We may have to keep going even after an EEXIST,
		// since the path may contain ".."s; and when there
		// is an EEXIST failure the system may return some other
		// error number.
		fullpath.append(*i);
		fullpath.append("/");
		mkdir(fullpath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	}

}



void DeepThought::refreshActiveDomainQueue()
{
	Domain* dom = NULL;
	time_t eligible_ts = now();

	while( not idle_domain_queue.empty() and 
		idle_domain_queue.front()->timestamp <= eligible_ts)
	{
		dom = idle_domain_queue.front();
		dom->previous_queue_length = dom->queueLength();
		// Move the domain from the idle into the active queue
		idle_domain_queue.pop_front();
		active_domain_queue.push(dom);
	}
}




// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
