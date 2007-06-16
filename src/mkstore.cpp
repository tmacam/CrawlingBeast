// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
/**@file mkstore.cpp
 * @brief Build a document store from set of crawled and downloaded docids.
 *
 * The document store consists of two parts:
 *
 * - An index file, store.idx
 *
 *   Which stores a fixed width index. Each entry in this index is a
 *   @c store_hdr_entry_t
 *
 * - A group of store data files (store.data_xxxx).
 *
 *   Each store data file consists of a sequence of
 *
 *   - docid
 *   - length of the document contents compressed
 *   - document contents
 *
 */
#include "mmapedfile.h"
#include "common.h"
#include "isamutils.hpp"

#include <time.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <assert.h>


/***********************************************************************
				    Typedefs
 ***********************************************************************/

/**Structure for document store index's entries.
 */
struct store_hdr_entry_t {
	uint32_t docid;	//!< DocId of the document
	uint16_t fileno; /**< Number of the store data file holding the contents
			 *   of the document with id @p docid
			 */
	uint32_t pos;	/**< Position of the document in the document store 
			 *   data file with number @c fileno
			 */


	store_hdr_entry_t(uint32_t id = 0, uint8_t n=0, uint32_t position=0)
	: docid(id), fileno(n),  pos(position)
	{}
} __attribute__((packed));


struct store_data_entry_t {
	uint32_t docid;	//!< DocId of the document
	uint32_t len;	//!< Length of the document

	store_data_entry_t(uint32_t _id=0, uint32_t _len=0)
	: docid(_id), len(_len)
	{}

} __attribute__((packed));


/***********************************************************************
				  StoreBuilder
 ***********************************************************************/

struct StoreBuilder {

	std::string store_path; //!< Location of the crawled data
	std::string docid_list; //!< List of valid docids

	std::vector<docid_t> ids;

	IndexedStoreOutputer<store_hdr_entry_t> outputer;

	StoreBuilder(const char* store, const char* list, const char* output)
	: store_path(store),
	  docid_list(list),
	  outputer(output,"store",2*1024*1024)
	{
	}

	~StoreBuilder() {}

	/**
	 * @todo this method is a dup from indexerutils make_filename. Perhaps we should
	 * create a crawlerutils to hold this sort of funcion.
	 */
	std::string make_crawler_filename(docid_t docid);

	
	/**
	 * @todo Move the contents of this function to a common file.
	 */
	void readDocids();

	void buildStore();

};


std::string StoreBuilder::make_crawler_filename(docid_t docid)
{
	std::ostringstream id_hex;

	id_hex << std::uppercase << std::hex << std::setw(8) <<
		std::setfill('0') << docid;
	std::string id_hex_str = id_hex.str();

	std::string id_path =  store_path + "/" + 
		id_hex_str.substr(0,2) + "/" +
		id_hex_str.substr(2,2) + "/" +
		id_hex_str.substr(4,2) + "/" +
		id_hex_str.substr(6,2) + "/data.gz"; // FIXME data.gz constant
		return id_path;
}



void StoreBuilder::readDocids()
{
	std::cout << "# Reading docid list ... "<< std::endl;
	ids.reserve(1<<20);
	std::ifstream known_docids(docid_list.c_str());
	std::string url;
	docid_t docid;
	while(known_docids >> docid >> url){
		ids.push_back(docid);
	}
	std::cout << "# Reading docid list ... done." << std::endl;
}


void StoreBuilder::buildStore()
{
	docid_t docid;
	uint32_t total_needed;
	store_data_entry_t data_header;
	uint16_t fileno;
	uint32_t pos;
	filebuf out;

	// Statistics
	docid_t d_count = 0;
	uint64_t byte_count = 0;
	uint64_t last_byte_count = 0;
	time_t last_broadcast = time(NULL);
	time_t time_started = time(NULL);


	// For every docid / retrieved document
	for(unsigned int i = 0; i < ids.size(); ++i){
		try {

			docid = ids[i];

			// read document
			std::string filename = make_crawler_filename(docid);
			MMapedFile file(filename.c_str());
			filebuf contents = file.getBuf();
			data_header.docid = docid;
			data_header.len = contents.len();

			total_needed = data_header.len + sizeof(store_data_entry_t);
			out = outputer.getDataOutputBuffer(total_needed, fileno, pos);

			// Store information about this document in the document store 
			// index
			outputer.putIndexEntry(store_hdr_entry_t(docid,fileno,pos));


			//FIXME
			//                std::cout << "Data id=" << data_header.docid <<
			//                                 " len=" << data_header.len <<
			//                                 " pos=" << pos << 
			//                                 std::endl;

			// Copy file contents to store
			// WARNING - filebuf idioms
			//	data header
			size_t n = sizeof(data_header);
			void* dest = (void*)out.read(n);
			memcpy(dest, &data_header, n);
			//	data contents
			n = data_header.len;
			dest = (void*) out.read(n);
			memcpy(dest, contents.start, n);
			assert(out.eof());


			// Statistics
			++d_count;
			byte_count += contents.len();
			if (d_count  % 1000 == 0) {
				time_t now = time(NULL);

				uint64_t byte_amount = byte_count - last_byte_count;
				std::cout << "# docs: " << d_count << " bytes: " <<
					byte_amount << " / " << byte_count << " bps: "<<
					byte_amount/(now - last_broadcast) << 
					" elapsed " << now - time_started << std::endl;

				last_broadcast = now;
				last_byte_count = byte_count;
			} // stats
		} catch(MMapedFileException& e) {
			std::cerr << "ERROR with docid " << docid << " " << e.what() << std::endl;
		}
	} // end for each document
}


/***********************************************************************
				      main
 ***********************************************************************/

void go(char* argv[])
{
	StoreBuilder store(argv[1], argv[2], argv[3]);

	store.readDocids();
	store.buildStore();
//        IndexedStoreOutputer<store_hdr_entry_t> outputer(argv[3],"store",2*1014*1014);
}

void dump()
{
	store_data_entry_t* data_header;

	MMapedFile idx_f("/tmp/down/store.hdr");
	MMapedFile data_f("/tmp/down/store.data_0000");

	filebuf idx = idx_f.getBuf();
	filebuf data = data_f.getBuf();

	const store_hdr_entry_t* entry = (store_hdr_entry_t*)idx.start;
	const store_hdr_entry_t* entry_end = (store_hdr_entry_t*)idx.end;

	for(; entry != entry_end; ++entry ){
		std::cout << "Entry id=" << entry->docid <<
				  " fileno=" << entry->fileno <<
				  " pos=" << entry->pos << std::endl; 
	}

	while(! data.eof()){
		size_t pos = data.current - data.start;

		data_header = (store_data_entry_t*) data.read(sizeof(store_data_entry_t));
		data.read(data_header->len);

		size_t next_pos = data.current - data.start;

		std::cout << "Data id=" << data_header->docid <<
				 " len=" << data_header->len <<
				 " pos=" << pos << 
				 " next_pos=" << next_pos <<
				 std::endl;
	}
}


int main(int argc, char* argv[])
{

	/*
	 * Parse comand line
	 */

	if(argc < 4) {
		std::cerr << "wrong number of arguments" << std::endl;
		std::cerr << "mkstore store_dir docid_list output_dir" << std::endl;
		exit(1);
	}

	go(argv);
	
//	dump();


	exit(0);
}



//EOF
