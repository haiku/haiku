// Blocker.cpp

#include <new>

#include "Blocker.h"

// Data
struct Blocker::Data {
	Data()
		: semaphore(create_sem(0, "blocker")),
		  references(1),
		  userData(0)
	{
	}

	Data(sem_id semaphore)
		: semaphore(semaphore),
		  references(1)
	{
	}

	~Data()
	{
		if (semaphore >= 0)
			delete_sem(semaphore);
	}

	sem_id	semaphore;
	int32	references;
	int32	userData;
};

// constructor
Blocker::Blocker()
	: fData(new(std::nothrow) Data)
{
}

// constructor
Blocker::Blocker(sem_id semaphore)
	: fData(new(std::nothrow) Data(semaphore))
{
	if (!fData)
		delete_sem(semaphore);
}

// copy constructor
Blocker::Blocker(const Blocker& other)
	: fData(NULL)
{
	*this = other;
}

// destructor
Blocker::~Blocker()
{
	_Unset();
}

// InitCheck
status_t
Blocker::InitCheck() const
{
	if (!fData)
		return B_NO_MEMORY;
	return (fData->semaphore < 0 ? fData->semaphore : B_OK);
}

// PrepareForUse
status_t
Blocker::PrepareForUse()
{
	// check initialization
	if (!fData || fData->semaphore < 0)
		return B_NO_INIT;
	// get semaphore count
	int32 count;
	status_t error = get_sem_count(fData->semaphore, &count);
	if (error != B_OK)
		return error;
	// set the semaphore count to zero
	if (count > 0)
		error = acquire_sem_etc(fData->semaphore, count, B_RELATIVE_TIMEOUT, 0);
	else if (count < 0)
		error = release_sem_etc(fData->semaphore, -count, 0);
	return error;
}

// Block
status_t
Blocker::Block(int32* userData)
{
	if (!fData || fData->semaphore < 0)
		return B_NO_INIT;

	status_t error = acquire_sem(fData->semaphore);

	if (userData)
		*userData = fData->userData;

	return error;
}

// Unblock
status_t
Blocker::Unblock(int32 userData)
{
	if (!fData || fData->semaphore < 0)
		return B_NO_INIT;

	fData->userData = userData;

	return release_sem(fData->semaphore);
}

// =
Blocker&
Blocker::operator=(const Blocker& other)
{
	_Unset();
	fData = other.fData;
	if (fData)
		fData->references++;
	return *this;
}

// ==
bool
Blocker::operator==(const Blocker& other) const
{
	return (fData == other.fData);
}

// !=
bool
Blocker::operator!=(const Blocker& other) const
{
	return (fData != other.fData);
}

// _Unset
void
Blocker::_Unset()
{
	if (fData) {
		if (--fData->references == 0)
			delete fData;
		fData = NULL;
	}
}

