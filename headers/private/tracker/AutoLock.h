/*
Open Tracker License

Terms and Conditions

Copyright (c) 1991-2000, Be Incorporated. All rights reserved.

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice applies to all licensees
and shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF TITLE, MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
BE INCORPORATED BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF, OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

Except as contained in this notice, the name of Be Incorporated shall not be
used in advertising or otherwise to promote the sale, use or other dealings in
this Software without prior written authorization from Be Incorporated.

Tracker(TM), Be(R), BeOS(R), and BeIA(TM) are trademarks or registered trademarks
of Be Incorporated in the United States and other countries. Other brand product
names are registered trademarks or trademarks of their respective holders.
All rights reserved.
*/

#ifndef _AUTO_LOCK_H
#define _AUTO_LOCK_H

template<class T>
class AutoLock
{

// exception safe locking mechanism, allocate on stack and have
// destructor unlock for you whenever the lock goes out of scope

public:
	AutoLock(T *lock, bool lockNow = true)
		// use <lockLater> for lazy locking
		:	fLock(lock),
			fHasLock(false)
		{
			if (lockNow)
				fHasLock = fLock->Lock();
		}

	AutoLock(T &lock, bool lockNow = true)
		:	fLock(&lock),
			fHasLock(false)
		{
			if (lockNow)
				fHasLock = fLock->Lock();
		}

	~AutoLock()
		{
			if (fHasLock)
				fLock->Unlock();
		}
	
	bool operator!() const
		{ return !fHasLock; }
	
	bool IsLocked() const
		{ return fHasLock; }
	
	// explicit Lock/Unlock calls are only used in special cases
	// for unlocking before lock goes out of scope and successive
	// re-locking
	// usually constructor/destructor locks/unlocks are sufficient
	void Unlock()
		{
			if (fHasLock) {
				fLock->Unlock();
				fHasLock = false;
			}
		}
	
	bool Lock()
		{
			if (!fHasLock)
				fHasLock = fLock->Lock();
	
			return fHasLock;	
		}

	// convenience call used when passing the AutoLock and the locked object
	// around
	T *LockedItem() const
		{ return fLock; }

private:
	T *fLock;
	bool fHasLock;
};

#endif
