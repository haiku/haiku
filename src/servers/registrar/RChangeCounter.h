//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef CHANGE_COUNTER_H
#define CHANGE_COUNTER_H

#include <ObjectLocker.h>

class RChangeCounter {
public:
	RChangeCounter() : fCount(0), fLocked(0), fChanged(false) {}
	~RChangeCounter() {}

	int32 Count() const { return fCount; }

	bool Lock() { fLocked++; return true; }
	void Unlock() { if (fLocked > 0) fLocked--; }

	bool Increment()
	{
		bool changed = false;
		if (fLocked && !fChanged) {
			fCount++;
			changed = fChanged = true;
		}
		return changed;
	}

	void Reset() { fCount = 0; fLocked = 0; fChanged = false; }

public:
	typedef BPrivate::BObjectLocker<RChangeCounter> Locker;

private:
	int32	fCount;
	int32	fLocked;
	bool	fChanged;
};

#endif	// CHANGE_COUNTER_H
