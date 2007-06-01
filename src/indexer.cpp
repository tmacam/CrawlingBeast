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

	index_files(store_dir, docid_list, output_dir);
	exit(0);

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


	StrIntMap wfreq;


	for(int i = 1; i < argc; ++i) {
//                try{
			AutoFilebuf dec(decompres(argv[i]));
			filebuf f = dec.getFilebuf();
//                        dumpWFreq(f, wfreq);
			HTMLContentIterator ci(f), ciend;
			for(; ci != ciend; ++ci){
				std::cout << *ci << std::endl;
			}

//                } catch(...) {
//                        throw;
			// pass
//                }
	}
	return 0;



	std::cout << wfreq.size() << std::endl;
	std::map<std::string, int> ordenado(wfreq.begin(), wfreq.end());

	std::map<std::string, int>::const_iterator i;
	for(i = ordenado.begin(); i != ordenado.end(); ++i){
		std::cout << i->first << ": " <<
				i->second << std::endl;
	}


	std::cout << "Done." << std::endl;
//        std::string press_enter;
//        std::cin >> press_enter;




}



//EOF
