#include "deepthought.h"

#include <sstream>
#include <iomanip>


#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>


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
	AutoLock synchronized(DOMAIN_LOCK);
	
	Domain* dom = new Domain(domain_name, pages, *this, unserializing);
	known_domains[domain_name] = dom;
	enqueueDomain(dom);
	return dom;
}

bool DeepThought::pageExists(docid_t docid)
{
	struct stat _statbuf;
	std::string filename = getDocIdPath(docid) + "/data";

	if (stat(filename.c_str(),&_statbuf) == 0) {
		return true;
	} else {
		return false;
	}
}


std::string DeepThought::getDocIdPath(docid_t docid)
{
	std::ostringstream id_hex;

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
	AutoLock synchronized(DOMAIN_LOCK);

	bool was_empty = false;
	if (domain_queue.empty()) {
		was_empty = true;
	}
	// should this domain really be enqueued?
	if (not dom->empty() and not dom->in_queue) {
		dom->in_queue = true;
		// our hopes and expectations
		// black holes and revelations
		domain_queue.push(dom);
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

//@synchronized(ERRLOG_LOCK)
void DeepThought::reportBadCrawling(docid_t id, const std::string& url,
				    const std::string& msg)
{
	AutoLock synchronized(ERRLOG_LOCK);

	errlog << id << "\t" << url << "\t" << msg << std::endl;
}

//@synchronized(STATS_LOCK)
void DeepThought::incDownloaded()
{
	AutoLock synchronized(STATS_LOCK);

	++download_counter;
}


//@synchronized(DOMAIN_LOCK)
PageRef DeepThought::popPage()
{
	AutoLock synchronized(DOMAIN_LOCK);

	PageRef page;
	Domain* dom = 0;

	// print currentThread(), "popPage", "WAIT" # DEBUG
	while (domain_queue.empty()) {
		DOMAIN_LOCK.wait(); 
	}
	if ( not domain_queue.empty() ) {
		waitUntillSafeToDownload();
		dom = domain_queue.top();
		domain_queue.pop();

		dom->timestamp = makeNextValidTimestamp();
		page = dom->popPage();

		// Deal with empty domains
		if ( dom->empty() ) {
			dom->in_queue = false;
		} else {
			// non-empty domains should be added back to the queue
			domain_queue.push(dom);
		}
	}

	return page;
}

void DeepThought::waitUntillSafeToDownload()
{
	int time_left = 0;
	Domain* dom = 0;

	if (not domain_queue.empty() ) {
		// domain_queue is a heap whose smallest element is in pos. 0
		dom = domain_queue.top();

		time_left = dom->timestamp - now();
		// print currentThread(), "popPage", "WAITING", self.domain_queue[0].timestamp, time_left # DEBUG
		if (time_left <= 0) {
			return;
		} else {
			sleep(time_left);
		}
	}
}

//@synchronized(DOCID_LOCK)
docid_t DeepThought::registerURL(std::string new_url)
{
	AutoLock synchronized(DOCID_LOCK);

	docid_t new_id = getNewDocId();
	store << new_id << "\t" << new_url << std::endl;

	return new_id;
}

// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
