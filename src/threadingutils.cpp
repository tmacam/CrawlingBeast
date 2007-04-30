#include "threadingutils.h"


int BaseThread::thread_count = 0;

void* thread_start_callback(void* _t_ptr)
{
	BaseThread* t = (BaseThread*)_t_ptr;
	return t->run();

}





// vim:syn=cpp.doxygen:autoindent:smartindent:fileencoding=utf-8:fo+=tcroq:
