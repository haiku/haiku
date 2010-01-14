// ThreadLocal.h

#ifndef THREAD_LOCAL_H
#define THREAD_LOCAL_H

#include <SupportDefs.h>

// ThreadLocalFreeHandler
class ThreadLocalFreeHandler {
public:
								ThreadLocalFreeHandler();
	virtual						~ThreadLocalFreeHandler();

	virtual	void				Free(void* data) = 0;
};

// ThreadLocal
class ThreadLocal {
public:
								ThreadLocal(
									ThreadLocalFreeHandler* freeHandler = NULL);
								~ThreadLocal();

			status_t			Set(void* data);
			void				Unset();
			void*				Get() const;

private:
			struct ThreadLocalMap;

			ThreadLocalMap*		fMap;
			ThreadLocalFreeHandler* fFreeHandler;
};

// ThreadLocalUnsetter
class ThreadLocalUnsetter {
public:
	ThreadLocalUnsetter(ThreadLocal* threadLocal)
		: fThreadLocal(threadLocal)
	{
	}

	ThreadLocalUnsetter(ThreadLocal& threadLocal)
		: fThreadLocal(&threadLocal)
	{
	}

	~ThreadLocalUnsetter()
	{
		if (fThreadLocal)
			fThreadLocal->Unset();
	}

private:
	ThreadLocal*	fThreadLocal;
};

#endif	// THREAD_LOCAL_H
