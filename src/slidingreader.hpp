// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __SLIDINGREADER_H
#define __SLIDINGREADER_H
/**@file slidingreader.h
 * @brief Efficient readers for files in memory-constrained cenarios.
 *
 */

#include "mmapedfile.h"
#include "indexerutils.hpp"
#include "memoryutils.hpp"

#include <iostream> // FIXME remove me

#include <iterator>
#include <fstream>
#include <iomanip>

/**Reads a file in blocks.
 *
 * @note Don't forget to call @code advanceWindow in derived classes'
 * constructors
 */
class BaseSlidingReader : public std::iterator<std::input_iterator_tag, run_triple>
{
protected:

	int max_elements;	//!< Max number of elements we can hold
	size_t max_memory;	//!< Amount of memory needed to hold max_elements

	value_type  value;
	value_type* cur_pos;		//!< Current reading position
	value_type* end_pos;		/**< One-past-the-end position of
					 *   the current in-memory reading
					 *   window
					 */
	bool _eof;
public:
	/**Constructor.
	 *
	 * @param filename The file to be read.
	 * @param _max_mem Maximum amount of memory a window can hold.
	 */
	BaseSlidingReader(const char* filename, size_t _max_mem)
	: max_elements(_max_mem/ sizeof(value_type)),
	  max_memory(max_elements*sizeof(value_type)),
	  value(),
	  cur_pos(0),
	  end_pos(0),
	  _eof(false)
	{
		// Derived classes should call advanceWindow() or ++*this
	}

	virtual ~BaseSlidingReader() {}

	virtual void advanceWindow() = 0;

	virtual BaseSlidingReader& operator++()
	{
		// Advance window if its end was reached
		if(cur_pos == end_pos){
			this->advanceWindow();
		}

		// If the end has not come, proceed with the suffering
		if (! eof() ) {
			value = *cur_pos;
			++cur_pos;
		}

		return *this;
	}

	//!Postfix increment operator helper class
        class Proxy {
                value_type tmp_val;
        public:
                Proxy(const value_type& val): tmp_val(val) {}
                inline value_type operator*() {return tmp_val;}
        };

        //!Postfix increment operator
        Proxy operator++(int)
        {
                Proxy d(operator*());
                ++*this;
                return d;
        }


	inline const value_type& operator*() const {return value; }
	inline const value_type* operator->() const { return &value; }

	inline bool operator<(const BaseSlidingReader& other ) const
	{
		return operator*() < *other;
	}

	inline bool eof() { return _eof;}
	
};

class FStreamSlidingReader : public BaseSlidingReader
{
	const ArrDelAdapter<char> arena;//!< Memory arena for blocks.
	std::ifstream reader;		//!< File reader
public:
	FStreamSlidingReader(const char* filename, size_t _max_mem)
	: BaseSlidingReader(filename,_max_mem),
	  arena(new char[max_memory]),
	  reader( filename , std::ios::binary | std::ios::in)
	{
		// Turn on exception reporting
		reader.exceptions(std::ios_base::badbit);
		if(!reader) {
			std::string msg("Error opening RUN file ");
			msg +=filename;
			throw std::runtime_error(msg);
		}
		// Populate current memory pos
		++*this;
	}

	virtual void advanceWindow()
	{
		// Don't perform anything if EOF was reached before and,
		// if unsure, re-check with reader it's status
		if ( (! eof()) || (_eof = reader.eof())  ) {
			// reading position is rewinded to arena's start
			cur_pos = (value_type*) arena.get();

			reader.read( (char*)cur_pos, max_memory);
			int n = reader.gcount();
			end_pos = &cur_pos[n/sizeof(value_type)];

			// Did we reach the end of this file ?
			if (n == 0) {
				_eof = true;
			}
			assert(n % sizeof(value_type) == 0);
		}
	}
};

class MMapedSlidingReader : public BaseSlidingReader
{
	std::string filename;
	ManagedFilePtr file;
	off_t filesize;
	off_t cur_offset;
	std::auto_ptr<MMapedFile> reader;	//!< File reader
public:
	MMapedSlidingReader(const char* filename, size_t _max_mem)
	: BaseSlidingReader(filename,getNearestValueAndPageMultiple(_max_mem)),
	  filename(filename),
	  file(filename),
	  filesize(file.filesize()),
	  cur_offset(0),
	  reader( new MMapedFile(filename,max_memory,0)) // This mmap wil be destructed by ++*this
	{
		// Populate current memory pos
		++*this;
	}

	/**getNearestValueAndPageMultiple.
	 *
	 * Return the nearest (and smallest) value from @p ammount that is
	 * multiple of both page size and @c value_type.
	 *
	 * @see getpagesize()
	 *
	 */
	static size_t getNearestValueAndPageMultiple(size_t ammount)
	{
		return ammount - (ammount % (sizeof(value_type)*getpagesize()));
	}

	virtual void advanceWindow()
	{
		// Don't perform anything if EOF was reached before and,
		// if unsure, re-check with reader it's status
		if ( (! eof()) && !( _eof = (cur_offset >= filesize))  ) {
			// MMapedFile doesn't deal with "out-of-file-range" request
			// gracefully
			size_t missing = filesize - cur_offset;
			size_t req_size = (missing > max_memory) ? max_memory: missing;

			// Renew mmap
			reader.reset( new MMapedFile(filename,req_size,cur_offset));
			reader->advise(MMapedFile::sequential);

			cur_offset += req_size;

			// reading position is rewinded to mmap's start
			cur_pos = (value_type*) reader->getBuf().start;
			end_pos = (value_type*) reader->getBuf().end;

		}
	}
};

#endif // __SLIDINGREADER_H




