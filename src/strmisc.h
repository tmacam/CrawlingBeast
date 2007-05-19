// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __STRMISC_H
#define __STRMISC_H
/**@file strmisc.h
 * @brief Misc. String functions
 *
 * Most of the functions defined here were made to provide some
 * "compatibility" layer between C++ string functions and the most
 * commom string funcionalities found in scripting languages.
 *
 * @todo merge the content of this file with explode's content.
 */


#include <string>
#include "filebuf.h"

#include "explode.h" // FIXME explode content should be merged into this file

/* ********************************************************************** *
				   CONSTANTS
 * ********************************************************************** */





/* ********************************************************************** *
				   FUNCTIONS
 * ********************************************************************** */

/**Remove leading whitespace from a filebuf.
 *
 * @param[in,out] s Filebuf to be striped
 * @return a reference to s, with whitespace removed.
 *
 * @warning This function modifies the given string.
 *
 * @todo This function should be inlined.
 */
filebuf& lstrip(filebuf& s);

#endif // __STRMISC_H
