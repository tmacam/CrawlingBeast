// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include <iostream>
#include <strings.h>

#include "mergerutils.hpp"

#include "isamutils.hpp"
#include "crawlerutils.hpp"
#include "vectorial.hpp"




/***********************************************************************
				  get_norms_mf
 ***********************************************************************/

struct InvertedFileDumper{

	inline void operator()(uint32_t tid, const hdr_entry_t* i_entry, filebuf data )
	{

		std::cout << tid << " : [" <<	i_entry->ft << " | ";

		inverted_list_vec_t ilist;
		inverted_list_vec_t::const_iterator doc;

		ByteWiseCompressor::decompress(i_entry->ft,data,ilist);
		for(doc = ilist.begin(); doc != ilist.end(); ++doc){
			std::cout << "<" << doc->first << "," << doc->second << "> ";
		}
		std::cout << "]" << std::endl;

	}
};

class GetNormsVisitor{

	//!Assignment operator doesn't make sense!
	GetNormsVisitor& operator=(const GetNormsVisitor&);

public:
	
	/*
	 *  Attributes
	 */

	uint32_t N; //!< Number of documents in the colection
	wdmfdt_map_t& WdMfreq;

	/*
	 *  Methods
	 */

	/**Constructor.
	 *
	 * @param N Number of documents in the collection.
	 *
	 */
	GetNormsVisitor(uint32_t collention_size, wdmfdt_map_t& Wd_Mfdt)
	: N(collention_size), WdMfreq(Wd_Mfdt)
	{}

	//! Copy constructor
	GetNormsVisitor(const GetNormsVisitor& other)
	: N(other.N), WdMfreq(other.WdMfreq)
	{}


	inline void operator()(uint32_t tid, const hdr_entry_t* i_entry, filebuf data )
	{

		uint32_t ft = i_entry->ft;
		double idf_t = log(double(N)/double(ft));

		inverted_list_vec_t ilist;
		inverted_list_vec_t::const_iterator doc;

		ByteWiseCompressor::decompress(i_entry->ft,data,ilist);
		for(doc = ilist.begin(); doc != ilist.end(); ++doc){
			// Syntatic sugar
			const uint32_t& d = doc->first;
			const uint32_t& fdt = doc->second;

			// The real thing
			if (WdMfreq[d].maxfdt < fdt) {
				WdMfreq[d].maxfdt = fdt;
			}

			double tmp = fdt * idf_t;
			WdMfreq[d].wd +=  tmp*tmp;
		}

	}

	inline void finishWdCalc()
	{
		wdmfdt_map_t::iterator d;
		for(d = WdMfreq.begin(); d != WdMfreq.end(); ++d) {
			wdmaxfdt_t& x = d->second;

			x.wd = sqrt(x.wd) / double(x.maxfdt);
		}
	}
};



void show_usage()
{
	std::cout << "Usage:" << std::endl;
	std::cout << "mknorms docid_list ilist_dir" << std::endl;
	std::cout << "\tdocid_list\tA file with a list of docid-url pairs in the store."<< std::endl;
	std::cout << "\tilist_dir\tdirectory holding inverted list"<< std::endl;
}

int main(int argc, char* argv[])
{
	if(argc != 3) {
		show_usage();
		exit(EXIT_FAILURE);
	}

	const char* docid_list = argv[1];
	const char* list_dir = argv[2];

	docid_vec_t docids;
	wdmfdt_map_t WFMap;

	read_docid_list(docid_list, docids);
	const uint32_t N = docids.size();
	GetNormsVisitor visitor(N, WFMap);

	VisitIndexedStore<hdr_entry_t>(list_dir, "index", visitor);
	visitor.finishWdCalc();

	uint32_t pos;
	uint16_t fileno;
	docid_vec_t::const_iterator d;
	IndexedStoreOutputer<norm_hdr_entry_t> normout(list_dir, "norm", N);
	for(d = docids.begin(); d != docids.end(); ++d){
		filebuf out = normout.getDataOutputBuffer( sizeof(wdmaxfdt_t),
							   fileno,
							   pos);
		normout.putIndexEntry(norm_hdr_entry_t(*d,fileno,pos));
		memcpy((void*)out.start,&WFMap[*d],sizeof(wdmaxfdt_t));
	}

}


// EOF
