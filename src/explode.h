#ifndef __EXPLODE_H
#define __EXPLODE_H
/**@file explode.h
 *
 * A dirty and simple implementation of python's str.split, PHP's Explode,
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
 * $Id$
 *
 * @author Tiago Alves Macambira
 * @namespace macambira
 *
 *
 */


#include <string>
#include <vector>
#include <sstream>


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

	if (i != values.end() ){
		out << *i;
	}
	++i;

	for(; i != values.end(); ++i){
		out << delimiter << *i;
	}

	return out.str();

} 



#endif // __EXPLODE_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
