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

#include <time.h>

#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>


/**Structure for document store index's entries.
 */
struct store_hdr_entry_t {
	uint32_t docid;	//!< DocId of the document
	uint8_t fileno; /**< Number of the store data file holding the contents
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
	uint32_t id;	//!< DocId of the document
	uint32_t len;	//!< Length of the document

	store_data_entry_t(uint32_t _id, uint32_t _len)
	: id(_id), len(_len)
	{}

} __attribute__((packed));


/***********************************************************************
				      MAIN
 ***********************************************************************/
struct StoreBuilder {

	std::string store_path; //!< Location of the crawled data
	const char* docid_list; //!< List of valid docids
	std::string output_path;/**< Output directory where inverted-file file's
				 *   will be written.
				 */
	unsigned int max_data_size;

	int n_index;		//!< Number of data files issued so far.


	std::vector<docid_t> ids;
	std::vector<store_hdr_entry_t> header_data;

	std::ofstream hdr_file; //!< The document store index writer
	std::ofstream data_file;//!< Data file writer


	StoreBuilder(const char* store, const char* list, const char* output,
		     unsigned int max_data_sz = 512*1024*1024)
	: store_path(store),
	  docid_list(list),
	  output_path(output),
	  max_data_size(max_data_sz),
	  n_index(0),
	  hdr_file(std::string(output_path + "/store.hdr").c_str(),
			std::ios::binary | std::ios::out),
	  data_file( mk_data_filename(output_path,n_index).c_str(),
			std::ios::binary | std::ios::out)
	{}

	~StoreBuilder()
	{
		if( hdr_file.is_open() ) { hdr_file.flush(); hdr_file.close() ;}
		if( data_file.is_open() ) { data_file.flush(); data_file.close() ;}
	}

	static std::string mk_data_filename(std::string path_prefix, int n);

	/**
	 * @todo this method is a dup from indexerutils make_filename. Perhaps we should
	 * create a crawlerutils to hold this sort of funcion.
	 */
	std::string make_crawler_filename(docid_t docid);

	void rotateDataFile();

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


std::string StoreBuilder::mk_data_filename(std::string path_prefix,
							int n)
{
	std::ostringstream filename_stream;
	filename_stream << path_prefix << "/store.data_" <<
		std::hex <<  std::setw(4) << std::setfill('0') <<
		n; //"%s/store.data_%04x"
	return filename_stream.str();
}


void StoreBuilder::rotateDataFile()
{
	// If the current store data file is too big it will be rotated
	if (data_file.tellp() > max_data_size) {
		data_file.close();
		// New index file in the house!
		++n_index; 
		data_file.open(mk_data_filename(output_path,n_index).c_str(),
				std::ios::binary | std::ios::out);
		// Turning exceptions - again... better safe then sorry
		data_file.exceptions( std::ios_base::badbit|
				std::ios_base::failbit);
	}
}



void StoreBuilder::readDocids()
{
	std::cout << "# Reading docid list ... "<< std::endl;
	ids.reserve(1<<20);
	std::ifstream known_docids(docid_list);
	std::string url;
	docid_t docid;
	while(known_docids >> docid >> url){
		ids.push_back(docid);
	}
	std::cout << "# Reading docid list ... done." << std::endl;

	header_data.reserve(ids.size());
}


void StoreBuilder::buildStore()
{
	docid_t docid;

	// Statistics
	docid_t d_count = 0;
	uint64_t byte_count = 0;
	uint64_t last_byte_count = 0;
	time_t last_broadcast = time(NULL);
	time_t time_started = time(NULL);


	// For every docid / retrieved document
	for(unsigned int i = 0; i < ids.size(); ++i){

		// Always check if data file needs to be rotated
		rotateDataFile();

		docid = ids[i];
		std::string filename = make_crawler_filename(docid);

		// read document
		MMapedFile file(filename.c_str());
		filebuf contents = file.getBuf();

		// Store information about this document in the document store 
		// index
		uint32_t pos = data_file.tellp(); // Current write position
		header_data.push_back(store_hdr_entry_t( docid, n_index, pos));

		// write file contents to store
		store_data_entry_t data_header(docid, contents.len());
		data_file.write((char*) &data_header, sizeof(data_header));
		data_file.write( contents.start, contents.len());

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
		}

	} // end for each document

	// Write store index file
	const char* data = (char*) &header_data[0];
	size_t len = header_data.size() * sizeof(store_hdr_entry_t);
	hdr_file.write( data, len);
	hdr_file.flush();
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

	StoreBuilder store(argv[1], argv[2], argv[3]);

	store.readDocids();
	store.buildStore();

	exit(0);
}



//EOF
