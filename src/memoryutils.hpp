// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
#ifndef __MEMORYUTILS_H
#define __MEMORYUTILS_H
/**@file memoryutils.h
 * @brief Memory Utilities.
 *
 * Think of it as our own extensions and utilities to complement <memory>
 *
 */

#include <sys/types.h>

/**Get total memory usage in KBytes.
 *
 * Get the total memory usage of a given process, as 
 * informed by /proc/<pid>/statm first field.
 *
 * @param pid The pid of the process whose memory usage information you want.
 *            The default value (0) can be used to retrieve memory information
 *            for the current process.
 *
 * @return The memory usage of the process or -1 if any error ocurred
 *         (the pid given does not exist etc) during this function execution.
 *
 */
int getMemoryUsage(pid_t pid = 0);

/**Array deletion adapter.
 *
 * A RAII or Scoped array adapter.
 *
 * You can associate it with an std::auto_ptr or use it alone.
 *
 * Based on code from http://www.gotw.ca/gotw/042.htm
 */
template <class T>
class ArrDelAdapter {
private:
	T* p_;

	//! Not default constructible
	ArrDelAdapter();

	//! Not copiable
	ArrDelAdapter(const ArrDelAdapter&);

	//! Not copiable
	ArrDelAdapter& operator=(const ArrDelAdapter&);
public:
	ArrDelAdapter( T* p ) : p_(p) { }
	~ArrDelAdapter() { delete[] p_; }
	// operators like "->" "T*" and other helpers
	inline T* get() const {return p_;}
};

#endif // __MEMORYUTILS_H
