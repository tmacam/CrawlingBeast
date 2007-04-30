#ifndef __THREADINGUTILS_H
#define __THREADINGUTILS_H
/**@file threadingutils.h
 * @brief Threading utilities - reinventing the whell since circa. 1970.
 *
 * All you old indie thread facilities friends, in one single place.
 *
 * Classe defined here habe a close resemblance with the ones in 
 * Python's threading module.
 */
#include "common.h"
#include <pthread.h>
#include "mmapedfile.h" // For ErrnoSysException

/* ********************************************************************** *
			       LOCK ABSTRACTIONS
 * ********************************************************************** */


struct AbstractLock{
	virtual void aquire() = 0;
	virtual void release() = 0;
	virtual ~AbstractLock(){}
};

/**It can smell you catholic shame and trap it in a safe lock.
 *
 * Can you see the thrills it is into?
 */
class CatholicShameMutex: public AbstractLock {
	//! Prevent Copying and assignment.
	CatholicShameMutex(const CatholicShameMutex&);
	//! Prevent Copying and assignment.
	CatholicShameMutex& operator=(const CatholicShameMutex&);
public:
	pthread_mutex_t _lock;

	CatholicShameMutex() :AbstractLock()
	{
		if(pthread_mutex_init(&_lock,NULL)){
			throw ErrnoSysException("Lock creation");
		}
	}

	virtual ~CatholicShameMutex() {pthread_mutex_destroy(&_lock); }

	void aquire() { pthread_mutex_lock(&_lock);}
	void release() { pthread_mutex_unlock(&_lock);}
};

/**I got a picture of a photograph.
 *
 * You know... a coditional variable...
 */
class BigBangBabyConditional : public AbstractLock{
	//! Prevent Copying and assignment.
	BigBangBabyConditional(const BigBangBabyConditional&);
	//! Prevent Copying and assignment.
	BigBangBabyConditional& operator=(const BigBangBabyConditional&);

	CatholicShameMutex CondLock;
public:
	pthread_cond_t _cond;

	BigBangBabyConditional() : AbstractLock(), CondLock()
	{ 
		//_cond = PTHREAD_COND_INITIALIZER;
		if(pthread_cond_init(&_cond,NULL)) {
			throw ErrnoSysException("Conditional Creation");
		};
	}

	~BigBangBabyConditional() {pthread_cond_destroy(&_cond); }

	void aquire() { CondLock.aquire(); }
	void release() { CondLock.release();}

	//!@name Conditional specific methods
	//!@{
	void wait() { pthread_cond_wait(&_cond, & (CondLock._lock) ); }
	void notify() { pthread_cond_signal(&_cond); }
	void notifyAll() { pthread_cond_broadcast(&_cond); }
	//!@}

};


/**Scoped lock idiom.
 *
 * AutoLock locks the associated mutex on construction, and
 * unlocks on destruction, in accordance with RAII style.
 *
 * This is a simplified rip-off of GNU'S C++ STL _Auto_Lock and lock.
 *
 * There are no checks performed, and this class follows the RAII
 * principle;
 *
 */
class AutoLock{
	AbstractLock& _lock; 

	//! Prevent Copying and assignment.
	AutoLock(const AutoLock& );

	//! Prevent Copying and assignment.
	AutoLock& operator=(const AutoLock&);

	//! It is not default constrictible!
	AutoLock();
public:	
	/**Constructor.
	 * 
	 * The mutex is locked.
	 *
	 * @throw ErrnoSysException
	 */
	AutoLock(AbstractLock& _lock) : _lock(_lock)
	{
		_lock.aquire();
	}

	/**Destructor.
	 * 
	 * The mutex is unlocked.
	 *
	 * @throw ErrnoSysException
	 */
	inline ~AutoLock()
	{
		_lock.release();
	}
};


/* ********************************************************************** *
			     THREADING ABSTRACTIONS
 * ********************************************************************** */

void* thread_start_callback(void* _t_ptr);

struct BaseThread {
	static int thread_count;

	pthread_t _handle;
	int t_id;

	BaseThread() : t_id(++thread_count){}
	virtual ~BaseThread(){}

	void start()
	{
		if (pthread_create(&_handle,NULL, thread_start_callback,this)){
			throw ErrnoSysException("Thread creation/start");
		}
	}

	virtual void* run() = 0;

	int join()
	{
		return pthread_join(_handle, NULL);
	}

};


#endif // __THREADINGUTILS_H
// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
