// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "indexer.h"
#include "zfilebuf.h"
#include "strmisc.h"
#include "indexerutils.hpp"

#include <fstream>
#include <sstream>



/***********************************************************************
				      MAIN
 ***********************************************************************/

int main(int argc, char* argv[])
{
	const char* store_dir;
	const char* docid_list;
	const char* output_dir;

	if(argc < 4) {
		std::cerr << "wrong number of arguments" << std::endl;
		std::cerr << "indexer store_dir docid_list output_dir" << std::endl;
		exit(1);
	}

	store_dir = argv[1];
	docid_list = argv[2];
	output_dir = argv[3];

	unsigned int run_size = 1<<28;

	std::cout << "# Reading docid list ... "<< std::endl;
	std::vector<docid_t> ids;
	ids.reserve(1<<20);
	std::ifstream known_docids(docid_list);
	std::string url;
	docid_t docid;
	while(known_docids >> docid >> url){
		ids.push_back(docid);
	}
	std::cout << "# Reading docid list ... done." << std::endl;

	index_files(store_dir, ids, output_dir, run_size);
	exit(0);
}

	/* WARNING  WARNING   WARNING   WARNING   WARNING   WARNING   WARNING  
	 * WARNING  WARNING   WARNING   WARNING   WARNING   WARNING   WARNING  
	 *
	 *
	 * The code below was left just for testing purposes. IGNORE IT.
	 *
	 *
	 * WARNING  WARNING   WARNING   WARNING   WARNING   WARNING   WARNING  
	 * WARNING  WARNING   WARNING   WARNING   WARNING   WARNING   WARNING  
	 */

//        std::string degenerado("aÇão weißbier 1ªcolocada palavra«perdida»©");
//        filebuf degen(degenerado.c_str(), degenerado.size());
//        dumpWFreq(degen);
//
//        WideCharConverter wconv;
//        std::string in("ßá");
//        std::wstring out =  wconv.mbs_to_wcs(in);
//        std::cout << "Out: " << wconv.wcs_to_mbs(out) << std::endl;

//        return 1;


//EOF
