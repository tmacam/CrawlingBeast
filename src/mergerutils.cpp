// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:

#include "mergerutils.hpp"

#include <sstream>


/***********************************************************************
				BaseIndexDumper
 ***********************************************************************/

BaseInvertedFileDumper::BaseInvertedFileDumper(std::string output_path,
		RunMerger& m, size_t _max_data_size, size_t header_reserve)
: output_dir(output_path),
  merger(m),
  n_index(0),
  max_data_size(_max_data_size),
  hdr_file(std::string(output_path + "/index.hdr").c_str(),
  		std::ios::binary | std::ios::out),
  data_file( mk_data_filename(output_path,n_index).c_str(),
  		std::ios::binary | std::ios::out)
{
	// Turn on exceptions
	hdr_file.exceptions( std::ios_base::badbit|
				std::ios_base::failbit);
	data_file.exceptions( std::ios_base::badbit|
				std::ios_base::failbit);

	header_data.reserve(header_reserve);
}


std::string BaseInvertedFileDumper::mk_data_filename(std::string path_prefix, int n)
{
	std::ostringstream filename_stream;
	filename_stream << path_prefix << "/index.data_" <<
		std::hex <<  std::setw(4) << std::setfill('0') <<
		n; //"%s/index.data_%04x"
	return filename_stream.str();
}

void BaseInvertedFileDumper::dump()
{
	uint32_t term = 0; // Terms start couting on 0
	run_triple tdf;
	uint32_t tf = 0; // # documents with a given term
	inverted_list_t ilist;

	while( !merger.eof() ) {
		tdf = merger.getNext();

		if (tdf.termid != term) {
			// Gotta dump the list we have so far
			storeTerm(term, tf,ilist);
			// Clean things up for a new inverted list/term
			ilist.clear();
			term = tdf.termid;
			tf=0;
		} 			

		// Account this <doc,freq> pair
		++tf;
		ilist.push_back(d_fdt_t(tdf.docid, tdf.freq));
	}

	// Is there something left in ilist? If so, dump it too!
	if ( ! ilist.empty()) {
		storeTerm(term,tf,ilist);
	}

	// Done with the inverted lists, write the index header.
	const char* data = (char*) &header_data[0];
	size_t len = header_data.size() * sizeof(hdr_entry_t);
	hdr_file.write(	data, len);
	hdr_file.flush();
}




void BaseInvertedFileDumper::rotateDataFile()
{
	// If the index file is too big it will be rotated
	if (data_file.tellp() > max_data_size) {
		data_file.close();
		// New index file in the house!
		++n_index; 
		data_file.open(mk_data_filename(output_dir,n_index).c_str(),
	  	    	std::ios::binary | std::ios::out);
		// Turning exceptions - again... better safe then sorry
		data_file.exceptions( std::ios_base::badbit|
					std::ios_base::failbit);
	}
}

void BaseInvertedFileDumper::storeTerm(uint32_t termid, uint32_t doc_count,
				const inverted_list_t& ilist)
{
	// Always rotate before dumping
	rotateDataFile();

	// Store information about this term in the search structure header
	uint32_t pos = data_file.tellp(); // Current write position
	header_data.push_back( hdr_entry_t( doc_count, pos, n_index));

	// write ilist do data file
	dumpInvertedList(doc_count, ilist);
}


void BaseInvertedFileDumper::dumpInvertedList(uint32_t tf,
					const inverted_list_t& ilist)
{
	inverted_list_vec_t list_dump(ilist.begin(), ilist.end());
	assert(tf == list_dump.size());

	data_file.write( (char*) &list_dump[0], tf*sizeof(d_fdt_t));

}

/***********************************************************************
		      ByteWiseCompressedInvertedFileDumper
 ***********************************************************************/

void ByteWiseCompressedInvertedFileDumper::dumpInvertedList(uint32_t tf,
					const inverted_list_t& ilist)
{
	// Clear previous invocations
	out_buffer.clear();

	//We expect the compressed list to be no bigger than the original.
	size_t expected_len = tf*sizeof(d_fdt_t);
	if (out_buffer.capacity() < expected_len ) {
		out_buffer.reserve(expected_len);
	}

	ByteWiseCompressor::compress(ilist,out_buffer);
	assert( ! out_buffer.empty()  );

	data_file.write( (char*) &out_buffer[0], out_buffer.size());
}


// EOF
