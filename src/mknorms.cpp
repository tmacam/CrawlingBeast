// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include <iostream>
#include <strings.h>

#include "mergerutils.hpp"

#include <ext/hash_map>
#include <tr1/functional>

#include "isamutils.hpp"



/***********************************************************************
			     Typedefs and constants
 ***********************************************************************/



/**Preprocessed \f$W_d\f$ and \f$\max_{i}f_{i,d}\f$ values.
 *
 * \f$W_d\f$ is the norm or module of the vector of a document, and is
 * given by the form \f$\sqrt{\sum_{i\subset d}w_{t,d}^{2}}\f$,
 * where \f$w_{t,d}=\frac{f_{t,d}}{\max_{i}f_{i,d}}\cdot\log\frac{N}{f_{t}}\f$
 *
 * @todo move this definition to vectorspace.hpp
 */
struct wdmaxfdt_t {
	double wd;
	uint32_t maxfdt;

	//!Constructor
	wdmaxfdt_t(double w=0, uint32_t max=1)
	: wd(w), maxfdt(max)
	{}
}__attribute__((packed));

typedef __gnu_cxx::hash_map < docid_t, wdmaxfdt_t> wdmfdt_map_t;

/**Structure for document's norm store index's entries.
 */
struct norm_hdr_entry_t {
	uint32_t docid;	//!< DocId of the document
	uint16_t fileno; /**< Number of the store data file holding the contents
			 *   of the document with id @p docid
			 */
	uint32_t pos;	/**< Position of the document in the document store 
			 *   data file with number @c fileno
			 */


	norm_hdr_entry_t(uint32_t id = 0, uint8_t n=0, uint32_t position=0)
	: docid(id), fileno(n),  pos(position)
	{}
} __attribute__((packed));



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

typedef std::vector<docid_t> docid_vec_t;

docid_vec_t& read_docid_list(const char* docid_list, docid_vec_t& ids)
{
        ids.reserve(1<<20);
        std::ifstream known_docids(docid_list);
        std::string url;
        docid_t docid;
        while(known_docids >> docid >> url){
		std::cout  << "Reading " << docid << " " << url << std::endl; // FIXME
                ids.push_back(docid);
        }

	return ids;
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
		std::cout << *d << " Wd " << WFMap[*d].wd << " max_fdt " << WFMap[*d].maxfdt << std::endl; // FIXME
	}

}


// EOF
