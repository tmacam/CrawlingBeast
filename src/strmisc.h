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
 * @author Tiago Alves Macambira
 *
 */


#include "filebuf.h"
#include <string>
#include <vector>
#include <sstream>

/* ********************************************************************** *
			     CONSTANTS AND SYMBOLS
 * ********************************************************************** */

//!@defgroup xmlconstants
/**@name Symbols and Commom String Constants
 *
 * This constants have the same meaning as the corresponding rules of the
 * XML Specification, Section 2.3 Common Syntatic Constructs
 */
//@{

const std::string LETTERS = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
const char _WHITESPACE[] = " \t\n\x0b\x0c\r\0";
const std::string WHITESPACE(_WHITESPACE,sizeof(_WHITESPACE));
const std::string DIGITS = "0123456789";
const std::string HEXDIGITS = "0123456789abcdefABCDEF";
const std::string ALPHANUM = LETTERS + DIGITS;

//@}


/* **********************************************************************
 *			     STRING TRANFORMATIONS
 *			  AND PYTHON COMPAT. FUNCTIONS
 * ********************************************************************** */

//!@defgroup strpycompat
/**@name String Transformation and Python string compatibility functions.
 *
 * Most of the functions defined here were made to provide some
 * "compatibility" layer between C++ string functions and the most
 * commom string funcionalities found in scripting languages.
 *
 * @{
 */

/**Coverts a string to lowercase.
 *
 * @warning The string is modified in-place!
 *
 * @param s[in,out] String to be converted to lowercase.
 * @return A reference to s, already converted to lowercase.
 */
std::string& to_lower(std::string& s);

/**Coverts a wide-string to lowercase.
 *
 * @warning The wide-string is modified in-place!
 *
 * @param s[in,out] Wide String to be converted to lowercase.
 * @return A reference to s, already converted to lowercase.
 */
inline std::wstring& to_lower(std::wstring& s)
{
	std::transform(s.begin(), s.end(), s.begin(),
			(wint_t(*)(wint_t))towlower);
	return s;
}


/**Coverts a string to uppercase.
 *
 * @warning The string is modified in-place!
 *
 * @param s[in,out] String to be converted to uppercase.
 * @return A reference to s, already converted to uppercase.
 */
std::string& to_upper(std::string& s);


/**Remove whitespace from the start and from the end of a string.
 *
 * Works just like perl's strip and python's strip.
 *
 * @param[in,out] s The string to be trimmed/striped.
 * @return A copy of s with trailing and leading whitespace removed.
 *
 * @warning the original string IS MODIFIED
 */
std::string strip(const std::string s);


inline bool is_in(unsigned char c, const std::string& where)
{
        return where.find(c) != std::string::npos;
}

template<class C> bool is_in(typename C::value_type v, const C& c)
{
	return std::find(c.begin(), c.end(), v) != c.end();
}


//! u.startswith(start)
bool startswith(const filebuf& u, const std::string& start );

//! data.startswith(start)
bool startswith(const std::string& data, const std::string& start );

//! u.endswith(end)
bool endswith(const std::string& u, const std::string& end );


/**Remove leading whitespace from a filebuf.
 *
 * @param[in,out] s Filebuf to be striped
 * @return a reference to s, with whitespace removed.
 *
 * @warning This function modifies the given string.
 *
 */
filebuf& lstrip(filebuf& s);


/**Convert Unicode codepoint to it's UTF-8 representation.
 *
 * @param uv Unicode value of the character to convert to UTF-8
 *
 * @warning unicode values that require more then UTF32 to be represented
 * are not handled and silently converted to 0x7fffffff.
 *
 * @note Code taken from http://search.cpan.org/src/GSAR/perl-5.6.1/utf8.c
 *
 */
std::string unichr(unsigned int uv);

//@}

/* ********************************************************************** *
			SPLIT, EXPLODE, JOIN AND FRIENDS
 * ********************************************************************** */


//!@defgroup split
/**@name Explit and friend.
 *
 * Dirty and simple implementations of python's str.split, PHP's Explode,
 * perl's split and so on.
 *
 *
 * Most of this code is based on a message sent to comp.lang.c++
 * http://groups.google.com/group/comp.lang.c++/msg/e19335f1c396d288
 * from Brian Rodenborn, on 21 Ago 2003
 *
 * This module defines 2 functions:
 *
 * - a generic template
 * - a specific function splits a string into a std::vector< std::string >
 *
 * Observe that both functions mimic the behaviour of their dynamic-languages
 * counterpart:
 * 
 * - split("/", "/") -> ["",""]
 * - split("", "/") -> [""]
 * - split("/A/B/C/D/", "/") -> ["","A","B","C", "D",""]
 * - split("a:b", ":") -> ["a","b"]
 *
 * - split("/A/B/C/D/", "/", 1) -> ["", "A/B/C/D/"]
 * - split("/A/B/C/D/", "/", 2) -> ["","A","B/C/D/"]
 *
 *
 *
 */

//@{
/**Splits a string into pieces using separator as separator.
 *
 * @param inString The string to be splited
 * @param separator The string used as delimiter.
 * @param maxsplit If non-zero, at most maxsplit splits are done.
 * 		   Defaults to 0, i.e., there is no limit of splits.
 *
 */
template<class Cont> 
Cont split (const typename Cont::value_type& inString,
            const typename Cont::value_type& separator,
	    unsigned int maxsplit = 0)
{
	Cont returnVector;
	typedef typename Cont::value_type value_type;
	typename value_type::size_type start = 0;
	typename value_type::size_type end = 0;
	unsigned int count = maxsplit;

	while( ((end=inString.find(separator, start)) != value_type::npos) &&
			( maxsplit ? count-- : true) )
	{
		returnVector.push_back (inString.substr (start, end-start));
		start = end+separator.size();
	}

	returnVector.push_back (inString.substr (start));

	return returnVector;

} 



/**Splits a string into pieces using separator as separator.
 *
 * @param inString The string to be splited
 * @param separator The string used as delimiter.
 * @param maxsplit If non-zero, at most maxsplit splits are done.
 * 		   Defaults to 0, i.e., there is no limit of splits.
 *
 */
std::vector<std::string>
split (const std::string &inString, const std::string &separator,
		unsigned int maxsplit = 0);

/**Joins a string using separator as separator.
 *
 * @param delimiter The string used as delimiter.
 * @param values A STL conteinter with forward iterators.
 *
 * @return A string with elements from values joined by
 * 	separators.
 *
 * @note We work differently from ostream_iterator, in that the
 * former always append a delimiter after a element, and we don't
 * do that. We only add delimiters between elements.
 *
 * Notice:
 *  - join("/",["a"]) -> "a"
 *  - join("/",["a","b"]) -> "a/b"
 *
 */
template<class C> 
std::string join( const typename C::value_type& delimiter,
			     const C& values)
{
	std::ostringstream out;

	typename C::const_iterator i = values.begin();

	if (not values.empty() ) {

		if (i != values.end() ){
			out << *i;
			++i;
		}

		for(; i != values.end(); ++i){
			out << delimiter << *i;
		}
	}

	return out.str();

} 

//@}

/* ********************************************************************** *
		     WIDE CHAR STRINGS CONVERSION UTILITIES
 * ********************************************************************** */

/**Converts string with multi-byte encoded content to wide-char strings.
 *
 * Convertion is done using plain C locale utils (mbsrtowcs and wcstombs).
 */
class WideCharConverter {
protected:
	std::string locale_name;
public:

	WideCharConverter(const std::string locale_name = "pt_BR.UTF-8")
	: locale_name(locale_name)
	{
		if (setlocale(LC_CTYPE, "pt_BR.UTF-8") == NULL) {
			throw std::runtime_error("Failed to set locale");
		}
	}

	inline std::wstring mbs_to_wcs(const std::string& mbs)
	{

		size_t wcs_len = mbs.size() + 1;
		std::auto_ptr<wchar_t> _wcs(new wchar_t[wcs_len]);
		wchar_t* wcs = _wcs.get();

		int res = mbstowcs(wcs, mbs.c_str(), wcs_len);

		if (res == -1) {
			throw std::runtime_error("WideCharConverter error in mbs_to_wcs");
		}

		return std::wstring(wcs,res);
	}

	inline std::string wcs_to_mbs(	std::wstring& wcs)
	{
		size_t mbs_len = wcstombs(NULL,wcs.c_str(),0)+1;
		std::auto_ptr<char> _mbs(new char[mbs_len]);
		char* mbs = _mbs.get();

		int res = wcstombs(mbs, wcs.c_str(), mbs_len);

		if (res == -1) {
			throw std::runtime_error("WideCharConverter error in wcs_to_mbs");
		}

		return std::string(mbs,res);

	}
};

#endif // __STRMISC_H
