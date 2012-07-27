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
#ifndef __THREAD__
#define __THREAD__


#include <Debug.h>
#include <OS.h>

#include "ObjectList.h"
#include "FunctionObject.h"
#include "Utilities.h"


namespace BPrivate {

class SimpleThread {
	// this should only be used as a base class,
	// subclass needs to add proper locking mechanism
public:
	SimpleThread(int32 priority = B_LOW_PRIORITY, const char* name = 0);
	virtual ~SimpleThread();

	void Go();

private:
	static status_t RunBinder(void*);
	virtual void Run() = 0;

protected:
	thread_id fScanThread;
	int32 fPriority;
	const char* fName;
};


class Thread : private SimpleThread {
public:
	static void Launch(FunctionObject* functor,
		int32 priority = B_LOW_PRIORITY, const char* name = 0);

private:
	Thread(FunctionObject*, int32 priority, const char* name);
	~Thread();
	virtual void Run();

	FunctionObject* fFunctor;
};

class ThreadSequence : private SimpleThread {
public:
	static void Launch(BObjectList<FunctionObject>*, bool async = true,
		int32 priority = B_LOW_PRIORITY);

private:
	ThreadSequence(BObjectList<FunctionObject>*, int32 priority);
	~ThreadSequence();

	virtual void Run();
	static void Run(BObjectList<FunctionObject>*list);

	BObjectList<FunctionObject>* fFunctorList;
};

// would use SingleParamFunctionObjectWithResult, except mwcc won't handle this
template <class Param1>
class SingleParamFunctionObjectWorkaround : public FunctionObjectWithResult<status_t> {
public:
	SingleParamFunctionObjectWorkaround(status_t (*function)(Param1), Param1 param1)
		:	fFunction(function),
			fParam1(param1)
		{
		}


	virtual void operator()()
		{ (fFunction)(fParam1); }

	virtual ulong Size() const { return sizeof(*this); }

private:
	status_t (*fFunction)(Param1);
	Param1 fParam1;
};

template <class T>
class SimpleMemberFunctionObjectWorkaround : public FunctionObjectWithResult<status_t> {
public:
	SimpleMemberFunctionObjectWorkaround(status_t (T::*function)(), T* onThis)
		:	fFunction(function),
			fOnThis(onThis)
		{
		}

	virtual void operator()()
		{ (fOnThis->*fFunction)(); }

	virtual ulong Size() const { return sizeof(*this); }

private:
	status_t (T::*fFunction)();
	T fOnThis;
};


template <class Param1, class Param2>
class TwoParamFunctionObjectWorkaround : public FunctionObjectWithResult<status_t>  {
public:
	TwoParamFunctionObjectWorkaround(status_t (*callThis)(Param1, Param2),
		Param1 param1, Param2 param2)
		:	function(callThis),
			fParam1(param1),
			fParam2(param2)
		{
		}

	virtual void operator()()
		{ (function)(fParam1, fParam2); }

	virtual uint32 Size() const { return sizeof(*this); }

private:
	status_t (*function)(Param1, Param2);
	Param1 fParam1;
	Param2 fParam2;
};


template <class Param1, class Param2, class Param3>
class ThreeParamFunctionObjectWorkaround : public FunctionObjectWithResult<status_t>  {
public:
	ThreeParamFunctionObjectWorkaround(status_t (*callThis)(Param1, Param2, Param3),
		Param1 param1, Param2 param2, Param3 param3)
		:	function(callThis),
			fParam1(param1),
			fParam2(param2),
			fParam3(param3)
		{
		}

	virtual void operator()()
		{ (function)(fParam1, fParam2, fParam3); }

	virtual uint32 Size() const { return sizeof(*this); }

private:
	status_t (*function)(Param1, Param2, Param3);
	Param1 fParam1;
	Param2 fParam2;
	Param3 fParam3;
};


template <class Param1, class Param2, class Param3, class Param4>
class FourParamFunctionObjectWorkaround : public FunctionObjectWithResult<status_t>  {
public:
	FourParamFunctionObjectWorkaround(status_t (*callThis)(Param1, Param2, Param3, Param4),
		Param1 param1, Param2 param2, Param3 param3, Param4 param4)
		:	function(callThis),
			fParam1(param1),
			fParam2(param2),
			fParam3(param3),
			fParam4(param4)
		{
		}

	virtual void operator()()
		{ (function)(fParam1, fParam2, fParam3, fParam4); }

	virtual uint32 Size() const { return sizeof(*this); }

private:
	status_t (*function)(Param1, Param2, Param3, Param4);
	Param1 fParam1;
	Param2 fParam2;
	Param3 fParam3;
	Param4 fParam4;
};


template<class Param1>
void
LaunchInNewThread(const char* name, int32 priority, status_t (*func)(Param1),
	Param1 p1)
{
	Thread::Launch(new SingleParamFunctionObjectWorkaround<Param1>(func, p1),
		priority, name);
}


template<class T>
void
LaunchInNewThread(const char* name, int32 priority, status_t (T::*function)(),
	T* onThis)
{
	Thread::Launch(new SimpleMemberFunctionObjectWorkaround<T>(function,
		onThis), priority, name);
}


template<class Param1, class Param2>
void
LaunchInNewThread(const char* name, int32 priority,
	status_t (*func)(Param1, Param2),
	Param1 p1, Param2 p2)
{
	Thread::Launch(new
		TwoParamFunctionObjectWorkaround<Param1, Param2>(func, p1, p2),
			priority, name);
}


template<class Param1, class Param2, class Param3>
void
LaunchInNewThread(const char* name, int32 priority,
	status_t (*func)(Param1, Param2, Param3),
	Param1 p1, Param2 p2, Param3 p3)
{
	Thread::Launch(new ThreeParamFunctionObjectWorkaround<Param1, Param2,
		Param3>(func, p1, p2, p3), priority, name);
}


template<class Param1, class Param2, class Param3, class Param4>
void
LaunchInNewThread(const char* name, int32 priority,
	status_t (*func)(Param1, Param2, Param3, Param4),
	Param1 p1, Param2 p2, Param3 p3, Param4 p4)
{
	Thread::Launch(new FourParamFunctionObjectWorkaround<Param1, Param2,
		Param3, Param4>(func, p1, p2, p3, p4), priority, name);
}


template<class View>
class MouseDownThread {
public:
	static void TrackMouse(View* view, void (View::*)(BPoint),
		void (View::*)(BPoint, uint32) = 0, bigtime_t pressingPeriod = 100000);

protected:
	MouseDownThread(View* view, void (View::*)(BPoint),
		void (View::*)(BPoint, uint32), bigtime_t pressingPeriod);

	virtual ~MouseDownThread();

	void Go();
	virtual void Track();

	static status_t TrackBinder(void*);

private:
	BMessenger fOwner;
	void (View::*fDonePressing)(BPoint);
	void (View::*fPressing)(BPoint, uint32);
	bigtime_t fPressingPeriod;
	volatile thread_id fThreadID;
};


template<class View>
void
MouseDownThread<View>::TrackMouse(View* view,
	void(View::*donePressing)(BPoint),
	void(View::*pressing)(BPoint, uint32), bigtime_t pressingPeriod)
{
	(new MouseDownThread(view, donePressing, pressing, pressingPeriod))->Go();
}


template<class View>
MouseDownThread<View>::MouseDownThread(View* view,
	void (View::*donePressing)(BPoint),
	void (View::*pressing)(BPoint, uint32), bigtime_t pressingPeriod)
	:	fOwner(view, view->Window()),
		fDonePressing(donePressing),
		fPressing(pressing),
		fPressingPeriod(pressingPeriod)
{
}


template<class View>
MouseDownThread<View>::~MouseDownThread()
{
	if (fThreadID > 0) {
		kill_thread(fThreadID);
		// dead at this point
		TRESPASS();
	}
}


template<class View>
void
MouseDownThread<View>::Go()
{
	fThreadID = spawn_thread(&MouseDownThread::TrackBinder,
		"MouseTrackingThread", B_NORMAL_PRIORITY, this);

	if (fThreadID <= 0 || resume_thread(fThreadID) != B_OK)
		// didn't start, don't leak self
		delete this;
}


template<class View>
status_t
MouseDownThread<View>::TrackBinder(void* castToThis)
{
	MouseDownThread* self = static_cast<MouseDownThread*>(castToThis);
	self->Track();
	// dead at this point
	TRESPASS();
	return B_OK;
}


template<class View>
void
MouseDownThread<View>::Track()
{
	for (;;) {
		MessengerAutoLocker lock(&fOwner);
		if (!lock)
			break;

		BLooper* looper;
		View* view = dynamic_cast<View*>(fOwner.Target(&looper));
		if (!view)
			break;

		uint32 buttons;
		BPoint location;
		view->GetMouse(&location, &buttons, false);
		if (!buttons) {
			(view->*fDonePressing)(location);
			break;
		}
		if (fPressing)
			(view->*fPressing)(location, buttons);

		lock.Unlock();
		snooze(fPressingPeriod);
	}

	delete this;
	ASSERT(!"should not be here");
}

} // namespace BPrivate

using namespace BPrivate;

#endif	// __THREAD__
