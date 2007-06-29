// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
/**@file myserver.cpp
 * @brief My Search Engine Web interface - PageRank and Vector-Space Model.
 *
 * @see myserver2.cpp for the VSM-only implementation.
 *
 * @todo There is too much code here that could be combined/shared with
 * myserve.cpp. Fix this in a new incarnation.
 *
 */

#include "httpserver.hpp"
#include "config.h"
#include "urltools.h"

#include "crawlerutils.hpp"
#include <sys/sendfile.h>       // For CachedCrawledDataHandler
#include "mmapedfile.h"         // For CachedCrawledDataHandler

#include "mkmeta.hpp"		// Page Metadata information
#include "mkpagerank.hpp"

#include <map>

#include "queryvec_logic.hpp"

#include <valarray>


#include <iostream>


/******************************************************************************
				    Typedefs
 ******************************************************************************/

typedef hash_map<uint32_t, float> TPageRankMap;

struct pagerank_res_t{
	uint32_t docid;		//!< The document this entry is about
	float   pagerank;	//!< The page's PageRank
	float  similarity;	//!< Vector-Space model similarity
	float  score;		//!< Final combination of a doc PR and Sim.

	explicit pagerank_res_t(uint32_t d, float pr, float sim, float w)
	: docid(d), pagerank(pr), similarity(sim), score(w)
	{}

	pagerank_res_t(const vec_res_t& vs)
	: docid(vs.first), pagerank(0), similarity(vs.second), score(0)
	{}

	//! This is reverse-order operator< !!!
	inline bool operator< (const pagerank_res_t& other) const
	{
		return score > other.score;
	}
};

typedef std::vector<pagerank_res_t> pr_res_vec_t;

/******************************************************************************
				MatchLiOuputter
 ******************************************************************************/

struct MatchLiOuputter {
	std::ostringstream& out;
	TMetaBase& metabase;

	MatchLiOuputter(std::ostringstream& output, TMetaBase& meta)
	: out(output), metabase(meta)
	{}


	inline void operator()(const pagerank_res_t& doc)
	{
		const uint32_t& id = doc.docid;
		TMetaBase::TUrlTitle ut = metabase.getMetaData(id);

		out << "<li>";
		out << "<a class='title' href=\"" << ut.first << "\">" << ut.second << "</a>";
		out << "<div class='stats'>";
		out << "DocId: " << id;
		out << " - Score: " << doc.score;
		out << " - PageRank: " << doc.pagerank;
		out << " - Similarity: " << doc.similarity;
		out << " - ";
		out << "<a href='/cache/" << id << "' >(Cached version)</a>";
		out << "</div></li>\n";
		out << "</li>\n";
	}
};


/******************************************************************************
		       Support / Example Request Handlers
 ******************************************************************************/

struct IndexHandler : public AbstractRequestHandler {
	void process(HTTPClientHandler& req)
	{
		std::string response;
		
		response = http::mk_response_header();
		response += "Hi /";

		req.write(response);
	}
};

struct CachedCrawledDataHandler : public AbstractRequestHandler {
	std::string store_dir;

	CachedCrawledDataHandler(std::string dir)
	: store_dir(dir)
	{}

	void process(HTTPClientHandler& req)
	{
		try{
			/* Parse request */
			BaseURLParser uri(req.uri);
			std::vector<std::string> pathc = split(uri.path,"/");
			if (pathc.size() < 1){
				throw NotFoundHTTPException();
			}
			uint32_t docid = fromString<uint32_t>(pathc[2]);
			std::string name = make_crawler_filename(
						store_dir.c_str(),docid);

			/* Prepare resonse */
			std::string response;
			std::string extra_hdr = "Content-Encoding: gzip" + http::CRLF;
			response = http::mk_response_header("OK", 200, "text/html",
					extra_hdr);
			ManagedFilePtr file(name.c_str());

			req.write(response);
			sendfile( req.fd, file.getFileno(),
					NULL, file.filesize());
		} catch (ErrnoSysException& e) {
			throw NotFoundHTTPException();
		}
	}
};


/******************************************************************************
			      PageRankQueryHandler
 ******************************************************************************/


struct PageRankQueryHandler : public AbstractRequestHandler {
	typedef std::map<std::string, std::string> TQueryMap;

	std::string index_dir;
	VectorialQueryResolver resolver;
	vec_res_vec_t vs_matches;	//!< Vector-Space results
	pr_res_vec_t matches;		//!< The result of a query
	TPageRankMap pagerank;

	TQueryMap _GET;
	TMetaBase metabase;

	PageRankQueryHandler(std::string dir)
	: index_dir(dir),
	  resolver(index_dir.c_str()),
	  metabase(index_dir)
	{
		// Load PageRank
		MMapedFile prfile(index_dir + PAGERANK_HDR_SUFIX);
		filebuf prdata = prfile.getBuf();
		pagerank_hdr_entry_t* cur = (pagerank_hdr_entry_t*) prdata.start;
		pagerank_hdr_entry_t* end = (pagerank_hdr_entry_t*) prdata.end;

		for(; cur < end; ++cur) {
			pagerank[cur->docid] = cur->pagerank;
		}
	}

	void process(HTTPClientHandler& req)
	{
		std::string results;

		BaseURLParser uri(req.uri);
		parse_GET(uri.query);

		matches.clear();
		vs_matches.clear();

		// Process Query
		if (_GET["q"] != "") {
			resolver.processQuery(_GET["q"],vs_matches);
			combinePRandVSM(vs_matches, matches);
			mkResultsFragment(matches,results);
		}

		// Make response page
		std::string response = http::mk_response_header();

		response += mkResultPage(results);


		req.write(response);

	}

	//!Combine Vector-Space and PageRank
	void combinePRandVSM(const vec_res_vec_t& vs_result,
			     pr_res_vec_t& result)
	{
		size_t n = vs_result.size();

		if (n == 0) {
			return;
		}

		float max_vs = vs_result[0].second; // Results are sorted by VS
		float max_pr = 0.0;

		std::valarray<float> vs(n);
		std::valarray<float> pr(n);
		std::valarray<float> res(n);

		// Load data
		for(size_t i = 0; i < n; ++i){
			pagerank_res_t page(vs_result[i]);

			float prval = pagerank[page.docid];
			vs[i] = page.similarity;
			pr[i] = page.pagerank = prval;

			if (prval > max_pr){ max_pr = prval; }

			result.push_back(page);
		}

		// Normalize vectors
		vs /= max_vs;
		pr /= max_pr;

		// Apply combination factor
		vs *= (1.0 - PAGERANK_WEIGHT);
		pr *= PAGERANK_WEIGHT;

		res = vs + pr;

		// Save resulting scores
		for(size_t i = 0; i < n; ++i) {
			result[i].score = res[i];
		}

		// Sort the result
		std::sort(result.begin(), result.end());
	}


	void parse_GET(std::string query)
	{
		typedef std::vector<std::string> strvec_t;

		_GET.clear();

		strvec_t tuples = split(query,"&");
		for(size_t i = 0; i < tuples.size(); ++i) {
			strvec_t keyval = split(tuples[i],"=",1);
			if(keyval.size() == 2) {
				_GET[keyval[0]] = decodePE(keyval[1]);
			}
		}
	}

	//! Parse percent encoded data
	static std::string decodePE(std::string msg)
	{
		using std::istringstream;

		std::string result;
		size_t pos = 0;
		while( pos < msg.size() ) {

			if (msg[pos] == '+') {
				result += ' ';
				 ++pos;
			} else if (msg[pos] == '%') {
				if (pos + 2 < msg.size() &&
				    isxdigit(msg[pos+1]) &&
				    isxdigit(msg[pos+2]))
				{
					char c;
					size_t value;

					istringstream in(msg.substr(pos+1,2));
					in >> std::hex >> value;
					c = value;

					result += c;
					pos += 3;
				} else {
					result += '%';
					++pos;
				}
			} else {
				size_t len = msg.find_first_of("+%",pos) - pos;
				result += msg.substr(pos, len);
				pos += len;
			}

		}

		return result;
	}

	std::string mkResultPage(std::string results = "")
	{
		std::string title;
		std::string q_val;

		if (_GET["q"] != "") {
			title = "Busca por \"" + _GET["q"] + "\" - ";
			q_val = _GET["q"];
		}


		std::ostringstream out;
		out <<  "<html>\n"
			"<head>\n"
			"<title>" << title << "Heim, meu filho?</title>\n"
			"<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\" />\n"
			"<link rel=stylesheet type='text/css' href='http://homepages.dcc.ufmg.br/~tmacam/irstyle.css'>"
			"</head>\n"
			"<body>\n"
			"<div class='head'>\n"
			"<h1>Heim, meu filho?</h1>\n"
			"<h2>A máquina de busca de Luiz Pareto</h2>\n"
			"<form action=\"/\" method=\"get\" >\n"
			"<input name=\"q\" type=\"text\" id=\"q2\" size=\"25\" value=\"" << q_val <<  "\">\n"
			"<input type=\"submit\" name=\"Submit\" value=\"Fala, Pareto!\">\n"
			"</form>\n"
			"</div> <!-- head -->"<<
			results <<
			"</body>\n"
			"</html>\n" << std::endl;

		return out.str();
	}

	void mkResultsFragment(const pr_res_vec_t& matches,
				std::string& results)
	{
		std::ostringstream out;
		out << "<hr id='resultsep'>\n";

		if (matches.empty()) {
			out << "No matches" << std::endl;
		} else {
			out << "# matches: " << matches.size() << "<br />\n"; 
			out << "<ol class='results'>";
			MatchLiOuputter li(out, metabase);
			std::for_each(matches.begin(),matches.end(),li);
			out << "</ol>";
		}

		results = out.str();
	}
};

void show_usage()
{
	std::cout << 	"Usage:\t myserver store_dir index_dir\n"
			"\n"
			"\tstore_dir\tWhere the crawled data is\n"
			"\tindex_dir\tWhere the index, voc. and aux. data are\n"
			<< std::endl;
			
}

int main(int argc, char* argv[])
{
	/* Parse command line */
	if(argc < 3) {
		std::cerr << "Wrong number of argments" << std::endl;
		show_usage();
		exit(EXIT_FAILURE);
	}
	std::string store_dir(argv[1]);
	std::string index_dir(argv[2]);

	/* Setup server */
	BaseHTTPServer server(SERVER_PORT);

	server.putChild("", new PageRankQueryHandler(index_dir));
	server.putChild( "source",
		new StaticFileHandler("myserver.cpp", "text/plain"));
	server.putChild( "err",
		new StaticFileHandler("myserver.cxx", "text/plain"));
	server.putChild( "cache", 
		new CachedCrawledDataHandler(store_dir));


	server.run();
}
