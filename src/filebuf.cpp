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

//const char* filebuf::read(unsigned int length)
//{
//        const char* old_pos = this->current;
//
//        /* This read can make the current position reach the
//         * end of this filebuf...
//         */
//        if ( (this->current + length) > this->end ){
//                /* ... but if it goes *beyond* the end of the buffer
//                 * it is an ERROR.
//                 */
//                throw std::out_of_range("End of filebuf reached or access out of bounds.");
//        } else {
//                this->current += length;
//                return old_pos;
//        }
//}



