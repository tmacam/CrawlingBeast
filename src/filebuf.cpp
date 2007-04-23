#include "filebuf.h"

// TODO const nos tipos, nas chamadas de função

// TODO? class EndOfFilebuf : public std::out_of_range{};


//filebuf::filebuf(char* buffer; unsigned int length)
//{
//        /* Cheking parameters */
//        if (buffer == 0) {
//                throw invalid_argument("Buffer cannot be NULL.");
//        }
//        
//        /* Setting things up */
//        this->current = this->start = buffer;
//        this->end = this->start + len;
//}

const char* filebuf::read(unsigned int length)
{
	const char* old_pos = this->current;

	/* This read can make the current position reach the
	 * end of this filebuf...
	 */
	if ( (this->current + length) > this->end ){
		/* ... but if it goes *beyond* the end of the buffer
		 * it is an ERROR.
		 */
		throw std::out_of_range("End of filebuf reached or access out of bounds.");
	} else {
		this->current += length;
		return old_pos;
	}
}

/**@brief Seeks to a different position in the buffer.
 *
 * It the seek crosses the filebuff boundaries a std::out_of_range is
 * thrown.
 */
const char* filebuf::seek(int offset)
{
	const char* new_pos = this->current + offset;

	if (new_pos >= this->start && new_pos < this->end){
		this->current = new_pos;
		return new_pos;
	} else {
		throw std::out_of_range("Seek crosses filebuf boundaries.");
	}

}

filebuf filebuf::readf(unsigned int length)
{
	return filebuf(this->read(length),length);
}
