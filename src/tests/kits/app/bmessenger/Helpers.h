//------------------------------------------------------------------------------
//	Helpers.h
//
//------------------------------------------------------------------------------

#ifndef HELPERS_H
#define HELPERS_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Looper.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------


// helper class: quits a BLooper on destruction
class LooperQuitter {
public:
	inline LooperQuitter(BLooper *looper) : fLooper(looper) {}
	inline ~LooperQuitter()
	{
		fLooper->Lock();
		fLooper->Quit();
	}

private:
	BLooper	*fLooper;
};

// helper class: deletes an object on destruction
template<typename T>
class AutoDeleter {
public:
	inline AutoDeleter(T *object, bool array = false)
		: fObject(object), fArray(array) {}
	inline ~AutoDeleter()
	{
		if (fArray)
			delete[] fObject;
		else
			delete fObject;
	}

protected:
	T		*fObject;
	bool	fArray;
};

// helper class: deletes an BHandler on destruction
class HandlerDeleter : AutoDeleter<BHandler> {
public:
	inline HandlerDeleter(BHandler *handler)
		: AutoDeleter<BHandler>(handler) {}
	inline ~HandlerDeleter()
	{
		if (fObject) {
			if (BLooper *looper = fObject->Looper()) {
				looper->Lock();
				looper->RemoveHandler(fObject);
				looper->Unlock();
			}
		}
	}
};

// helper function: return the this team's ID
static inline
team_id
get_this_team()
{
	thread_info info;
	get_thread_info(find_thread(NULL), &info);
	return info.team;
}

#endif	// HELPERS_H
