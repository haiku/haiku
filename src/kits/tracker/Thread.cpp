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


#include "Thread.h"
#include "FunctionObject.h"


SimpleThread::SimpleThread(int32 priority, const char* name)
	:	fScanThread(-1),
		fPriority(priority),
		fName(name)
{
}


SimpleThread::~SimpleThread()
{
	if (fScanThread > 0 && fScanThread != find_thread(NULL)) {
		// kill the thread if it is not the one we are running in
		kill_thread(fScanThread);
	}
}


void
SimpleThread::Go()
{
	fScanThread = spawn_thread(SimpleThread::RunBinder,
		fName ? fName : "TrackerTaskLoop", fPriority, this);
	resume_thread(fScanThread);
}


status_t
SimpleThread::RunBinder(void* castToThis)
{
	SimpleThread* self = static_cast<SimpleThread*>(castToThis);
	self->Run();
	return B_OK;
}


void
Thread::Launch(FunctionObject* functor, int32 priority, const char* name)
{
	new Thread(functor, priority, name);
}


Thread::Thread(FunctionObject* functor, int32 priority, const char* name)
	:	SimpleThread(priority, name),
		fFunctor(functor)
{
	Go();
}


Thread::~Thread()
{
	delete fFunctor;
}


void
Thread::Run()
{
	(*fFunctor)();
	delete this;
		// commit suicide
}


void
ThreadSequence::Launch(BObjectList<FunctionObject>* list, bool async, int32 priority)
{
	if (!async) {
		// if not async, don't even create a thread, just do it right away
		Run(list);
	} else
		new ThreadSequence(list, priority);
}


ThreadSequence::ThreadSequence(BObjectList<FunctionObject>* list, int32 priority)
	:	SimpleThread(priority),
 		fFunctorList(list)
{
	Go();
}


ThreadSequence::~ThreadSequence()
{
	delete fFunctorList;
}


void
ThreadSequence::Run(BObjectList<FunctionObject>* list)
{
	int32 count = list->CountItems();
	for (int32 index = 0; index < count; index++)
		(*list->ItemAt(index))();
}


void
ThreadSequence::Run()
{
	Run(fFunctorList);
	delete this;
		// commit suicide
}
