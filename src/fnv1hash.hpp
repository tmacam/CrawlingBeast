// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __FNV1HASH_H__
#define __FNV1HASH_H__

/**@file fnv1hash.h
 * @brief FNV-1 hash function implementations.
 *
 * This code is adapted from "hash.c" from
 * <code> anisio@dcc.ufmg.br </code>
 *
 * See @c FNV namaspace documentation for more information
 * on the code in this file.
 *
 * @see FNV
 *
 */

#include <string>

using std::string;

/**FNV-1 hash function implementations.
 *
 * From : http://www.isthe.com/chongo/tech/comp/fnv/
 *
 *
 * The basis of the FNV hash algorithm was taken from an idea sent as reviewer
 * comments to the IEEE POSIX P1003.2 committee by Glenn Fowler and Phong Vo.
 * In a subsequent ballot round: Landon Curt Noll improved on their algorithm.
 * Some people tried this hash and found that it worked rather well. In an
 * EMail message to Landon, they named it the ``Fowler/Noll/Vo'' or FNV hash.
 *
 * FNV hashes are designed to be fast while maintaining a low collision rate.
 * The FNV speed allows one to quickly hash lots of data while maintaining a
 * reasonable collision rate. The high dispersion of the FNV hashes makes them
 * well suited for hashing nearly identical strings such as URLs, hostnames,
 * filenames, text, IP addresses, etc. 
 */
namespace FNV {
	inline uint64_t hash64(const string& key)
	{
		const static uint64_t fnvPrime = 1099511628211ULL;
		const static uint64_t offsetBasis = 14695981039346656037ULL;
		uint64_t hash = offsetBasis;

		for (size_t i = 0, l = key.size(); i < l; i++)
		{
			hash *= fnvPrime;
			hash ^= key[i];
		}

		return hash;
	}
	
	inline size_t hash32(const char* key, size_t size)
	{
		const static uint32_t fnvPrime = 16777619UL;
		const static uint32_t offsetBasis = 2166136261UL;
		uint32_t hash = offsetBasis;

		for (size_t i = 0; i < size; i++)
		{
			hash *= fnvPrime;
			hash ^= key[i];
		}

		return hash;
	}

	inline size_t hash32(const string& key)
	{
		if (key.empty()) {
			return hash32("",0);
		} else {
			return hash32(&key[0],key.size());
		}
	}
};

//! Dirty hack to force a uint64_t fit into a size_t.
struct UInt64Hasher {
	inline size_t operator()(const uint64_t& val) const
	{
		return (val >> 32) ^ val;
	}
};

/*******************************************************************************
				 Header tricks
 *******************************************************************************/


#if __GNUC__ > 2
using namespace __gnu_cxx;
#include <ext/hash_set>
#include <ext/hash_map>
namespace __gnu_cxx
{
	template<> struct hash< std::string >
	{
		size_t operator()( const std::string& x ) const
		{
			return FNV::hash32( x );
		}
	};

	//! This is ugly but will do the trick!
	template<> struct hash< uint64_t >
	{
		UInt64Hasher hasher;
		inline size_t operator()(const uint64_t& val) const
		{
			return hasher(val);
		}
	};

}
#else
#include <hash_set>
#include <hash_map>
#endif




#endif //__FNV1HASH_H__


// EOF
