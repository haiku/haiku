//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		ObjectLocker.h
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//	Description:	A templatized version of BAutolock.  Client class needs to
//					supply:
//						bool Lock()		--	returns whether lock succeeded
//						void Unlock()	--	unlocks the class
//------------------------------------------------------------------------------

#ifndef OBJECTLOCKER_H
#define OBJECTLOCKER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

namespace BPrivate {

template<class T>
class BObjectLocker
{
	public:
		inline BObjectLocker(T* looper);
		inline BObjectLocker(T& locker);
		
		inline ~BObjectLocker();
		
		inline bool IsLocked(void);
	
	private:
		T*		fLockClient;
		bool	fIsLocked;
};

template<class T>
BObjectLocker<T>::BObjectLocker(T* client)
	:	fLockClient(client), fIsLocked(client->Lock())
{
}

template<class T>
BObjectLocker<T>::BObjectLocker(T& client)
	:	fLockClient(&client), fIsLocked(client.Lock())
{
}

template<class T>
BObjectLocker<T>::~BObjectLocker()
{
	if (fIsLocked)
	{
		fLockClient->Unlock();
	}
}

template<class T>
bool BObjectLocker<T>::IsLocked(void)
{
	return fIsLocked;
}

}	// namespace BPrivate

#endif	//OBJECTLOCKER_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

