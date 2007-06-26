// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __ISAMUTILS_H__
#define __ISAMUTILS_H__

/**@file isamutils.hpp
 * @brief Utilities to write and read ISAM or Indexed Storange Files.
 *
 * ISAM (Indexed Sequential Acess Method) files is a method for storing data
 * for fast retrieval. In our code, ISAMs data is commonly called an
 * <em>"Indexed Storange"</em>.
 *
 * An Indexed Storange consists of two parts:
 *
 * - An index file, that goes by the prefix .hdr
 *
 *   Which stores a fixed width index. Each entry consists of an instance of
 *   a certain header file, particular to the data stored in that storange.
 *   For instance, a document indexed store  index file consists of
 *   @c store_hdr_entry_t entries.
 *
 * - A group of store data files, that goes by the prefix .data_xxxx.
 *
 *   For each entry in the index file there is a corresponding blob
 *   in the data one of the data files. Entries in the data files
 *   can have variable length, so it is important that entries in the index
 *   file give enought information to locate in which file (@c entriy->fileno)
 *   and where in the file (@c entry->pos ) the contents of its blob is.
 *
 *   For instance, for a document indexed storange, the contents of its
 *   store data consist of a sequence of
 *
 *   - docid
 *   - length of the document contents compressed
 *   - document contents
 *
 */

#include "mmapedfile.h"
#include "common.h"

#include <fstream>
#include <sstream>
#include <vector>
#include <iomanip>
#include <assert.h>


/***********************************************************************
				    Typedefs
 ***********************************************************************/

/***********************************************************************
			       Utility Functions
 ***********************************************************************/
inline std::string mk_isam_data_filename(std::string path, std::string name,
				    int n)
{
	std::ostringstream filename_stream;
	filename_stream << path << "/" << name << ".data_" <<
		std::hex <<  std::setw(4) << std::setfill('0') <<
		n; //"%s/%s.data_%04x"
	return filename_stream.str();
}




/***********************************************************************
			      IndexedStoreOutputer
 ***********************************************************************/

/**Save items into a Indexed Storange.
 *
 * A indexed storage. It is sort of a ISAM but records can be of
 * arbitrary length.
 *
 * - Index
 *   
 *   Direct acess by key is not garanteed and is the programer's
 *   responsabilty to store @e pointer-to-record information
 *   inside the index entry.
 *
 *
 * - data files.
 *
 *   Holds the records.
 *
 *   the record data may spread over multiple data files
 *   if a the aggregate size of the records @c S exceed the
 *   pre-determined maximum length of @p max_data_size.
 *
 * Data files are automatically rotated.
 *
 */
template <class idx_hdr_t, size_t max_data_size=512*1024*1024>
class IndexedStoreOutputer {
	std::vector<char> _real_arena; // bugger!
	char* arena;
	std::vector<idx_hdr_t> header_data;

	// Memory I/O management counters and "pointers"
	size_t n_files;	//!< Number of data files issued so far.
	filebuf data_buf;	/**< Controls the current reading position
				 *   and the ammount of space left in the
				 *   arena.
				 */

	// File I/O related things
	std::string output_path;
	std::string store_name;

	std::ofstream header_file;	//!< The document store index writer
	std::ofstream data_file;	//!< Data file writer

	// Auxiliary functions
	size_t getCurOffset() const {return data_buf.current - data_buf.start;}

	/** (forcibily) Rotate data files.
	 *
	 * @warning This is different from BaseInvertedFileDumper's rotate.
	 */
	void rotateDataFile();

	//! Turn on excetions and unbuffering on this stream.
	static void setupFStream(std::ofstream& out)
	{
		out.exceptions( std::ios::badbit|std::ios::failbit);
		out.rdbuf()->pubsetbuf(0, 0); // unbuffering output
	}

public:
	IndexedStoreOutputer(	const char* output_dir,
				const char* name,
				size_t index_reserve)
	: _real_arena(max_data_size),
	  arena(&_real_arena[0]),
	  n_files(0),
	  data_buf(arena,max_data_size),
	  output_path(output_dir),
	  store_name(name)
	{
		// Prepare files for I/O
		setupFStream(header_file);
		header_file.open(
			std::string(output_path+"/"+store_name+".hdr").c_str(),
			std::ios::binary | std::ios::out);
		
		setupFStream(data_file);
		data_file.open(
			mk_isam_data_filename(output_path,store_name,n_files).c_str(),
			std::ios::binary | std::ios::out);

		header_data.reserve(index_reserve);
	}

	~IndexedStoreOutputer()
	{
		// Flush remaining data file - if any;
		if ( getCurOffset() > 0 ) {
			data_file.write( data_buf.start, getCurOffset());
			data_file.flush();
			data_file.close();
		}

		// Write store index file
		const char* hdr = (char*) &header_data[0];
		size_t len = header_data.size() * sizeof(idx_hdr_t);
		header_file.write(hdr, len);
		header_file.flush();
		header_file.close();


	}

	void putIndexEntry(idx_hdr_t entry) { header_data.push_back(entry); }

	/**Reserve and retrive some space in the output buffer.
	 *
	 * It is the caller responability to copy date into the
	 * returned filebuf.
	 *
	 * @param[in] bytes_needed
	 * @param[out] file_no Use this to fill _hdr_
	 * @param[out] pos Use this to fill _hdr_
	 */
	filebuf getDataOutputBuffer(uint32_t bytes_needed,
				    uint16_t& file_no,
				    uint32_t& pos)
	{
		if ( bytes_needed > data_buf.len() ) {
			assert(bytes_needed < max_data_size);
			rotateDataFile();
		}

		file_no = n_files;
		pos = getCurOffset();
		//std::cout << "need " << bytes_needed << " pos = " << pos << std::endl; // FIXME
		return data_buf.readf(bytes_needed);
	}

};

template <class idx_hdr_t, size_t max_data_size>
void IndexedStoreOutputer<idx_hdr_t,max_data_size>::rotateDataFile()
{

	data_file.write( data_buf.start, getCurOffset());
	data_file.flush();
	data_file.close();
	// New index file in the house!
	++n_files; 
	data_file.open(
		mk_isam_data_filename(output_path,store_name,n_files).c_str(),
		std::ios::binary | std::ios::out);
	// Reset data buffer and file
	data_buf.current = data_buf.start; // Reset data buffer
	setupFStream(data_file); // Turning exceptions and unbuffering - again!
}



/***********************************************************************
			       VisitIndexedStore
 ***********************************************************************/

/**Iterate over all items of a Indexed Store.
 *
 * This functions implements the "Visitor" pattern for ISAM / Indexed
 * Storanges. Items are visited in the order they appear in the
 * index file.
 *
 * @param _entry_t Type/Struct of entries in the index file.
 * 		   Those entries @b must have @c pos and @c fileno
 * 		   attributes.
 *
 * @param _visitor_t A functor whose operator() will be called for each
 * 		     item in the Indexed Storage. See NullISAMVisitor
 * 		     for documentation on this functor.
 *
 * @param store_dir Location o the Indexed Storange files.
 *
 * @param name	Name of this ISAM. Used to rebuild the filename of the
 * 		files (index and data files) in this ISAM.
 *
 * @param visitor A instance of the functor defined in @p _visitor_t.
 *
 *
 */
template <class _entry_t, class _visitor_t>
void VisitIndexedStore(const char* store_dir, std::string name, _visitor_t visitor = _visitor_t())
{


	uint32_t counter = 0;

	std::string prefix(store_dir);

	std::string hdr_filename = prefix + "/" + name + ".hdr";
	MMapedFile _hdr(hdr_filename);
	_entry_t* ilist = (_entry_t*) _hdr.getBuf().start;
	_entry_t* ilist_end = (_entry_t*) _hdr.getBuf().end;

	uint32_t fileno=0;
	std::string data_filename = mk_isam_data_filename(prefix,name,fileno);

	std::auto_ptr<MMapedFile> data_mm( new MMapedFile(data_filename));
	data_mm->advise(MMapedFile::sequential);

	for(; ilist != ilist_end; ++ilist, ++counter){
		if (ilist->fileno != fileno) {
			// Data file changed. Get new one.
			fileno = ilist->fileno;
			std::string data_filename = mk_isam_data_filename(prefix,name,fileno);
			// Release now and free memory for next mmap
			data_mm.reset(); 
			data_mm.reset(new MMapedFile(data_filename));
			data_mm->advise(MMapedFile::sequential);
		}

		filebuf cur_data = data_mm->getBuf();
		cur_data.read(ilist->pos);
		visitor(counter, ilist, cur_data);
	}
}

/**Example visitor for VisitIndexedStore.
 *
 * @param count The record count of the item being visited. For sequentially
 * 		and continuous indexed data, this will also be the ID of this
 * 		record.
 *
 * @param i_entry The entry in the index file for this record.
 *
 * @param data  The contents of the data file holding the this record data, from
 * 		from the point where this record data starts up to the end of the
 * 		data file.
 */
template <class _entry_t>
struct NullISAMVisitor {
	inline void operator()(uint32_t count, const _entry_t* i_entry, filebuf data )
	{
	}
};

/***********************************************************************
			     Misc Helper Functions
 ***********************************************************************/

template<class _T>
filebuf& dumpToFilebuf(_T t, filebuf& out)
{
	size_t len =  sizeof(_T);
	void* pos = (void*) out.read(len);
	memcpy(pos,&t,len);
	
	return out;
}

filebuf& dumpStrToFilebuf(const std::string& t, filebuf& out)
{
	size_t len = t.size();
	void* pos = (void*) out.read(len);
	memcpy(pos,t.c_str(),len);
	
	return out;
}




#endif // __ISAMUTILS_H__


//EOF
