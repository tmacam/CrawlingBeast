#include "paranoidandroid.h"

#include "gzstream.h"


void ParanoidAndroid::setupOfstream(std::ostream& stream)
{
	// Turn store exceptions on
	stream.exceptions( std::ios_base::badbit|std::ios_base::failbit);                // FIXME set store unbuffered
}

void ParanoidAndroid::safePageAndMetadata(docid_t docid, PageDownloader& d)
{
	// Prepare to safe files.
	std::string doc_path = manager.getDocIdPath(docid);
	std::string meta_filename = doc_path + "/meta";
	std::string data_filename = doc_path + "/data.gz";

	// Setup ifstreams
	std::ofstream meta;
	//FIXME ogzstream data;
	std::ofstream data;
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

void* ParanoidAndroid::run()
{
#define TRACE std::cout << t_id << " "; BEEN_HERE

	while (manager.running) {
		// Homenagem ao DJ ATB: Don't Stop, 'till i come
		PageRef page = manager.popPage();
		//TRACE;

		std::string& url = page.first;
		docid_t& docid = page.second;

		if ( docid ) {	// cuz we can return None...
			try {
				PageDownloader d(url);
				d.get();
				if (d.follow) {
					manager.addPages(d.links);
				}
				// FIXME we are ignoring index/noindex
				safePageAndMetadata(docid, d);
				// print "DOWN", currentThread(), page.url #DEBUG
				manager.incDownloaded();
			} catch (NotSupportedSchemeException) {
				// Seems like we got redirected to a not-supported URL
				manager.reportBadCrawling(docid, url,
							"BAD REDIRECT ");
				// FIXME We used to grab urlib2 errors...
				// FIXME shouldn't we do the same for libcurl?
			} catch (std::runtime_error& e) {
				manager.reportBadCrawling(docid, url,e.what());
			} catch (...) {
				manager.reportBadCrawling(docid, url,"UNKNOWN EXCEPTION");
			}
		}
	}
	return (void*)this;
	//print currentThread(), "exiting..."
}



// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
