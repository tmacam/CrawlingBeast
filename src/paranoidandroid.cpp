#include "paranoidandroid.h"

#include "gzstream.h"
#include "assert.h"


void ParanoidAndroid::setupOfstream(std::ostream& stream)
{
	// Turn store exceptions on
	stream.exceptions( std::ios_base::badbit|std::ios_base::failbit);                // FIXME set store unbuffered
}

void ParanoidAndroid::savePageAndMetadata(docid_t docid, PageDownloader& d)
{
	// Prepare to safe files.
	std::string doc_path = manager.getDocIdPath(docid);
	std::string meta_filename = doc_path + "/meta";
	std::string data_filename = doc_path + PAGE_DATA_PREFIX;

	// Setup ifstreams
	std::ofstream meta;
	ogzstream data;
	// XXX for uuncompressed std::ofstream data;
	setupOfstream(meta);
	setupOfstream(data);

	manager.makedirs(doc_path);

	// Write metadata
	meta.open(meta_filename.c_str());
	d.writeMeta(meta);
	// Write crawling contents
	data.open(data_filename.c_str());
	filebuf contents = d.contents.getFilebuf();
	data.write(contents.start, contents.len());
	meta.close();
	data.close();
}

bool ParanoidAndroid::downloadRobots(const std::string& url, Domain* dom)
{
	assert(dom);

	URLRetriever ret(url, false);
	try {
		ret.go();
		filebuf robots_data = ret.getData();
		// Our robot-speak speking robot
		RobotsParser r2d2(robots_data);
		r2d2.parse();
		dom->setRobotsRules(r2d2.getRules());
	} catch(UndeterminedURLRetrieverException) {
		// Well, we did our best to get the robots
		// file. Let's just pretend we couldn't find one.
		dom->setRobotsRules(robots_rules_t());
		throw;
	}


	return true;
}

bool ParanoidAndroid::downloadPage(const std::string& url, docid_t docid)
{
	PageDownloader d(url);
	d.get();
	if (d.follow) {
		manager.addPages(d.links);
	}
	// FIXME we are ignoring index/noindex
	savePageAndMetadata(docid, d);
	// print "DOWN", currentThread(), page.url #DEBUG
	return true;
}

void* ParanoidAndroid::run()
{
	bool successfuly_parsed;
	bool is_a_robot_txt;
	PageRef page;
	Domain* dom;
#define TRACE std::cout << t_id << " "; BEEN_HERE

	while (manager.running) {
		// Set defaults for this iteration
		successfuly_parsed = false;
		is_a_robot_txt = false;
		dom = NULL;

		try {
			page = manager.popPage();
		} catch (GetRobotsForMePlzException& e) {
			is_a_robot_txt = true;
			page = e.page;
			dom = e.domain;
		}
		//TRACE;

		std::string& url = page.first;
		docid_t& docid = page.second;

		if ( docid ) {	// cuz He can return None...
			try {
				if (is_a_robot_txt) {
					successfuly_parsed=downloadRobots(url,
							dom);
				} else {
					successfuly_parsed = downloadPage(url,
							docid);
				}
			} catch (NotSupportedSchemeException) {
				// Seems like we got redirected to a
				// not-supported URL
				manager.reportBadCrawling(docid, url,
							"BAD REDIRECT ");
				// FIXME We used to grab urlib2 errors...
				// FIXME shouldn't we do the same for libcurl?
			} catch (std::runtime_error& e) {
				manager.reportBadCrawling(docid, url,e.what());
			} catch (...) {
				manager.reportBadCrawling(docid, url,
							"UNKNOWN EXCEPTION");
			}

			// Report the we crawled this page
			manager.incCrawled(successfuly_parsed, docid, url);
		}
	}
	return (void*)this;
	//print currentThread(), "exiting..."
}



// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
