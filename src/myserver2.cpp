// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
/**@file myserver2.cpp
 * @brief My Search Engine Web interface - Vector-Space Model.
 *
 * @see myserver.cpp for the VSM+PageRank implementation.
 *
 */

#include "httpserver.hpp"
#include "config.h"
#include "urltools.h"

#include "crawlerutils.hpp"
#include <sys/sendfile.h>       // For CachedCrawledDataHandler
#include "mmapedfile.h"         // For CachedCrawledDataHandler

#include "mkmeta.hpp"		// Page Metadata information

#include <map>

#include "queryvec_logic.hpp"


#include <iostream>


/******************************************************************************
				MatchLiOuputter
 ******************************************************************************/

struct MatchLiOuputter {
	std::ostringstream& out;
	TMetaBase& metabase;

	MatchLiOuputter(std::ostringstream& output, TMetaBase& meta)
	: out(output), metabase(meta)
	{}


	inline void operator()(const vec_res_t& docweight)
	{
		const uint32_t& id = docweight.first;
		const double& w = docweight.second;
		TMetaBase::TUrlTitle ut = metabase.getMetaData(id);

		out << "<li>";
		out << "<a href=\"" << ut.first << "\">" << ut.second << "</a>";
		out << "<br />";
		out << "DocId: " << id << " - Weight: " << w << " - ";
		out << "<a href='/cache/" << id << "' >(Cached version)</a>";
		out << "</li>\n";
	}
};


/******************************************************************************
				      Main
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


struct VectorialQueryHandler : public AbstractRequestHandler {
	typedef std::map<std::string, std::string> TQueryMap;

	std::string index_dir;
	VectorialQueryResolver resolver;
	vec_res_vec_t matches;

	TQueryMap _GET;
	TMetaBase metabase;

	VectorialQueryHandler(std::string dir)
	: index_dir(dir),
	  resolver(index_dir.c_str()),
	  metabase(index_dir)
	{}

	void process(HTTPClientHandler& req)
	{
		std::string results;

		BaseURLParser uri(req.uri);
		parse_GET(uri.query);

		matches.clear();
		if (_GET["q"] != "") {
			resolver.processQuery(_GET["q"],matches);
			mkResultsFragment(matches,results);
		}

		std::string response = http::mk_response_header();

		response += mkResultPage(results);


		req.write(response);

	}


	void parse_GET(std::string query)
	{
		typedef std::vector<std::string> strvec_t;

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
			"</head>\n"
			"<body>\n"
			"<h1>Heim, meu filho?</h1>\n"
			"<h2>A máquina de busca de Luiz Pareto</h2>\n"
			"<form action=\"/\" method=\"get\" >\n"
			"<input name=\"q\" type=\"text\" id=\"q2\" size=\"25\" value=\"" << q_val <<  "\">\n"
			"<input type=\"submit\" name=\"Submit\" value=\"Fala, Pareto!\">\n"
			"</form>\n" <<
			results <<
			"</body>\n"
			"</html>\n" << std::endl;

		return out.str();
	}

	void mkResultsFragment(const vec_res_vec_t& matches,
				std::string& results)
	{
		std::ostringstream out;
		out << "<hr id='resultsep'>\n";

		if (matches.empty()) {
			out << "No matches" << std::endl;
		} else {
			out << "# matches: " << matches.size() << "<br />\n"; 
			out << "<ol>";
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

	server.putChild("", new VectorialQueryHandler(index_dir));
	server.putChild( "source",
		new StaticFileHandler("myserver.cpp", "text/plain"));
	server.putChild( "err",
		new StaticFileHandler("myserver.cxx", "text/plain"));
	server.putChild( "cache", 
		new CachedCrawledDataHandler(store_dir));


	server.run();
}
