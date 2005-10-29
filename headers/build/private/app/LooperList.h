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
//	File Name:		LooperList.h
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//	Description:	Maintains a global list of all loopers in a given team.
//------------------------------------------------------------------------------

#ifndef LOOPERLIST_H
#define LOOPERLIST_H

// Standard Includes -----------------------------------------------------------
#include <vector>

// System Includes -------------------------------------------------------------
#include <Locker.h>
#include <OS.h>
#include <SupportDefs.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BLooper;

namespace BPrivate {

class BLooperList
{
	public:
		BLooperList();

		bool		Lock();
		void		Unlock();
		bool		IsLocked();

		void		AddLooper(BLooper* l);
		bool		IsLooperValid(const BLooper* l);
		bool		RemoveLooper(BLooper* l);
		void		GetLooperList(BList* list);
		int32		CountLoopers();
		BLooper*	LooperAt(int32 index);
		BLooper*	LooperForThread(thread_id tid);
		BLooper*	LooperForName(const char* name);
		BLooper*	LooperForPort(port_id port);

		struct LooperData
		{
			LooperData();
			LooperData(BLooper* loop, uint32 i);
			LooperData(const LooperData& rhs);
			LooperData& operator=(const LooperData& rhs);

			BLooper*	looper;
			uint32		id;
		};

	private:
		static	bool	EmptySlotPred(LooperData& Data);
		struct FindLooperPred
		{
			FindLooperPred(const BLooper* loop) : looper(loop) {;}
			bool operator()(LooperData& Data);
			const BLooper* looper;
		};
		struct FindThreadPred
		{
			FindThreadPred(thread_id tid) : thread(tid) {;}
			bool operator()(LooperData& Data);
			thread_id thread;
		};
		struct FindNamePred
		{
			FindNamePred(const char* n) : name(n) {;}
			bool operator()(LooperData& Data);
			const char* name;
		};
		struct FindPortPred
		{
			FindPortPred(port_id pid) : port(pid) {;}
			bool operator()(LooperData& Data);
			port_id port;
		};

		void	AssertLocked();

		BLocker					fLock;
		uint32					fLooperID;
		std::vector<LooperData>	fData;
};

extern _IMPEXP_BE BLooperList gLooperList;
}


#endif	//LOOPERLIST_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

