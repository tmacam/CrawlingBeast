// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
/**@file queryvec.cpp
 *
 * @brief Handles vectorial queries.
 *
 *
 * @note assuming that the inverted file is inverted.
 *
 */

#include <iostream>
#include "queryvec_logic.hpp"

/***********************************************************************
				      MAIN
 ***********************************************************************/

void do_query(VectorialQueryResolver& resolver, std::string query)
{
	vec_res_vec_t d_ids;
	resolver.processQuery(query,d_ids);


	if (d_ids.empty()) {
		std::cout << "No documents found matching query '" << query <<
			"' in documents." << std::endl;
	} else {
		std::cout << "Document(s) found matching query '" <<
				query << "': " << std::endl;
		vec_res_vec_t::const_iterator i;
		for(i = d_ids.begin(); i != d_ids.end(); ++i){
			std::cout << " doc: " << i->first << "  weight: " <<
						 i->second << std::endl;
		}
	}

}

void show_usage()
{
        std::cout << 	"Usage:\n"
			"queryvec store_dir\n"
			"\tstore_dir\twhere the index and norms data are."<< std::endl;
}

int main(int argc, char* argv[])
{
	// Parse command line
	if(argc < 2) {
		std::cerr << "Wrong number of arguments." << std::endl;
		show_usage();
		exit(EXIT_FAILURE);
	}

	std::string store_dir(argv[1]);
	std::string query;

	// Ok, let's start the show.

	std::cout << "Loading vocabulary and inverted file ... ";
	std::cout.flush();
	VectorialQueryResolver resolver(store_dir.c_str());
	std::cout << "done" << std::endl;

	std::cout <<"Type your query using spaces to split terms."<<std::endl;

	// Prompt
	std::cout << "> ";
	std::cout.flush();
	while(getline(std::cin,query)) {

		do_query(resolver, query);

		// Prompt
		std::cout << "> ";
		std::cout.flush();
	}


	exit(EXIT_SUCCESS);

}




//EOF
