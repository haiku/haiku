// LazyInitializable.cpp

#include "LazyInitializable.h"

// constructor
LazyInitializable::LazyInitializable()
	: fInitStatus(B_NO_INIT),
	  fInitSemaphore(-1)
{
	fInitSemaphore = create_sem(1, "init semaphore");
	if (fInitSemaphore < 0)
		fInitStatus = fInitSemaphore;
}

// constructor
LazyInitializable::LazyInitializable(bool init)
	: fInitStatus(B_NO_INIT),
	  fInitSemaphore(-1)
{
	if (init) {
		fInitSemaphore = create_sem(1, "init semaphore");
		if (fInitSemaphore < 0)
			fInitStatus = fInitSemaphore;
	} else
		fInitStatus = B_OK;
}

// destructor
LazyInitializable::~LazyInitializable()
{
	if (fInitSemaphore >= 0)
		delete_sem(fInitSemaphore);
}

// Access
status_t
LazyInitializable::Access()
{
	if (fInitSemaphore >= 0) {
		status_t error = B_OK;
		do {
			error = acquire_sem(fInitSemaphore);
		} while (error == B_INTERRUPTED);
		if (error == B_OK) {
			// we are the first: initialize
			fInitStatus = FirstTimeInit();
			delete_sem(fInitSemaphore);
			fInitSemaphore = -1;
		}
	}
	return fInitStatus;
}

// InitCheck
status_t
LazyInitializable::InitCheck() const
{
	return fInitStatus;
}

