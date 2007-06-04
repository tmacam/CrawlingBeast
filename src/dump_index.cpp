// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include <iostream>
#include "mergerutils.hpp"
#include "mmapedfile.h"

/* We are assuming that the whole vocabulary fits in the memory
 * and is in a single file.
 */
void dump_inverted_file(IntStrMap& id2term, const char* store_dir)
{
	IntStrMap::const_iterator i;

	std::string prefix(store_dir);
	std::string hdr_filename = prefix + "/index.hdr";
	std::string data_filename = 
			BaseInvertedFileDumper::mk_data_filename(prefix,0);

	MMapedFile _hdr(hdr_filename);
	MMapedFile _data(data_filename);

	hdr_entry_t* _header = (hdr_entry_t*) _hdr.getBuf().start;
	const char* data = _data.getBuf().start;
	const d_fdt_t* ilist = NULL;

	for(i = id2term.begin(); i != id2term.end(); ++i){
		hdr_entry_t& header = _header[i->first];

		std::cout << i->first << " " <<
				i->second << ": <" <<
			header.ft << "; ";
		ilist = (d_fdt_t*)(data + header.pos);
		for(uint32_t doc = 0; doc < header.ft; ++doc){
			std::cout << ilist[doc].first << ", ";
		}
		std::cout << ">" << std::endl;


	}
}


void show_usage()
{
	std::cout << "Usage:" << std::endl;
	std::cout << "dump_index <option> store_dir" << std::endl;
	std::cout << "\t-v\tdump vocabulary" << std::endl;
	std::cout << "\t-i\tdump inverted file" << std::endl;
}

int main(int argc, char* argv[])
{
	if(argc != 3) {
		show_usage();
		exit(EXIT_FAILURE);
	}

	std::string option(argv[1]);
	std::string store_dir(argv[2]);

	StrIntMap voc;
	IntStrMap id2t;

	load_vocabulary(voc,store_dir.c_str());
	mapIdToTerms(voc, id2t);


	if (option == "-v") {
		IntStrMap::const_iterator i;
		
		for(i = id2t.begin(); i != id2t.end(); ++i){
			std::cout << i->first << " " <<
					i->second << std::endl;
		}
	}else if(option == "-i" ) {
		dump_inverted_file(id2t, store_dir.c_str());
	} else {
		std::cout << "Unknown option '" << option << "'." << std::endl;
		show_usage();
		exit(EXIT_FAILURE);
	}

}
