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
#ifndef _LOCKING_LIST_H
#define _LOCKING_LIST_H


#include <Locker.h>
#include "ObjectList.h"


namespace BPrivate {

template <class T>
class LockingList : public BObjectList<T> {
public:
	LockingList(int32 itemsPerBlock = 20, bool owning = false);
	~LockingList()
		{
		Lock();
		}

	bool Lock();
	void Unlock();
	bool IsLocked() const;

private:
	BLocker lock;
};


template<class T>
LockingList<T>::LockingList(int32 itemsPerBlock, bool owning)
	:	BObjectList<T>(itemsPerBlock, owning)
{
}


template<class T>
bool
LockingList<T>::Lock()
{
	return lock.Lock();
}


template<class T>
void
LockingList<T>::Unlock()
{
	lock.Unlock();
}


template<class T>
bool
LockingList<T>::IsLocked() const
{
	return lock.IsLocked();
}

} // namespace BPrivate

using namespace BPrivate;

#endif	// _LOCKING_LIST_H
