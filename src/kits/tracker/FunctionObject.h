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
#ifndef __FUNCTION_OBJECT__
#define __FUNCTION_OBJECT__


#include <Message.h>
#include <Messenger.h>
#include <MessageFilter.h>

#include <Entry.h>
#include <Node.h>


// parameter binders serve to store a copy of a struct and
// pass it in and out by pointers, allowing struct parameters to share
// the same syntax as scalar ones

// You will mostly want to use the NewFunctionObject... convenience
// factories
//

// add more function objects and specialized binders as needed

namespace BPrivate {

template<class P>
class ParameterBinder {
// primitive default binder for scalars
public:
	ParameterBinder() {}
	ParameterBinder(P p)
		:	p(p)
		{}

	P Pass() const
		{ return p; }
private:
	P p;
};


template<>
class ParameterBinder<const BEntry*> {
public:
	ParameterBinder() {}
	ParameterBinder(const BEntry* p)
		:	p(*p)
		{}

	ParameterBinder &operator=(const BEntry* newp)
		{ p = *newp; return *this; }

	const BEntry* Pass() const
		{ return &p; }
private:
	BEntry p;
};


template<>
class ParameterBinder<const entry_ref*> {
public:
	ParameterBinder() {}
	ParameterBinder(const entry_ref* p)
		{
			if (p)
				this->p = *p;
		}

	ParameterBinder &operator=(const entry_ref* newp)
		{ p = *newp; return *this; }

	const entry_ref* Pass() const
		{ return &p; }
private:
	entry_ref p;
};


template<>
class ParameterBinder<const node_ref*> {
public:
	ParameterBinder() {}
	ParameterBinder(const node_ref* p)
		:	p(*p)
		{}

	ParameterBinder &operator=(const node_ref* newp)
		{ p = *newp; return *this; }

	const node_ref* Pass() const
		{ return &p; }
private:
	node_ref p;
};


template<>
class ParameterBinder<const BMessage*> {
public:
	ParameterBinder() {}
	ParameterBinder(const BMessage* p)
		:	p(p ? new BMessage(*p) : NULL)
		{}

	~ParameterBinder()
		{
			delete p;
		}

	ParameterBinder &operator=(const BMessage* newp)
		{
			delete p;
			p = (newp ? new BMessage(*newp) : NULL);
			return *this;
		}

	const BMessage* Pass() const
		{ return p; }

private:
	BMessage* p;
};


class FunctionObject {
public:
	virtual void operator()() = 0;
	virtual ~FunctionObject() {}
};


template<class R>
class FunctionObjectWithResult : public FunctionObject {
public:
	const R &Result() const { return result; }

protected:
	R result;
};


template <class Param1>
class SingleParamFunctionObject : public FunctionObject {
public:
	SingleParamFunctionObject(void (*callThis)(Param1),
		Param1 p1)
		:	function(callThis),
			p1(p1)
		{
		}

	virtual void operator()() { (function)(p1.Pass()); }

private:
	void (*function)(Param1);
	ParameterBinder<Param1> p1;
};


template <class Result, class Param1>
class SingleParamFunctionObjectWithResult : public
	FunctionObjectWithResult<Result> {
public:
	SingleParamFunctionObjectWithResult(Result (*function)(Param1), Param1 p1)
		:	function(function),
			p1(p1)
		{
		}

	virtual void operator()()
		{ FunctionObjectWithResult<Result>::result = (function)(p1.Pass()); }

private:
	Result (*function)(Param1);
	ParameterBinder<Param1> p1;
};


template <class Param1, class Param2>
class TwoParamFunctionObject : public FunctionObject {
public:
	TwoParamFunctionObject(void (*callThis)(Param1, Param2),
		Param1 p1, Param2 p2)
		:	function(callThis),
			p1(p1),
			p2(p2)
		{
		}

	virtual void operator()() { (function)(p1.Pass(), p2.Pass()); }

private:
	void (*function)(Param1, Param2);
	ParameterBinder<Param1> p1;
	ParameterBinder<Param2> p2;
};


template <class Param1, class Param2, class Param3>
class ThreeParamFunctionObject : public FunctionObject {
public:
	ThreeParamFunctionObject(void (*callThis)(Param1, Param2, Param3),
		Param1 p1, Param2 p2, Param3 p3)
		:	function(callThis),
			p1(p1),
			p2(p2),
			p3(p3)
		{
		}
	
	virtual void operator()() { (function)(p1.Pass(), p2.Pass(), p3.Pass()); }

private:
	void (*function)(Param1, Param2, Param3);
	ParameterBinder<Param1> p1;
	ParameterBinder<Param2> p2;
	ParameterBinder<Param3> p3;
};


template <class Result, class Param1, class Param2, class Param3>
class ThreeParamFunctionObjectWithResult : public
	FunctionObjectWithResult<Result> {
public:
	ThreeParamFunctionObjectWithResult(
		Result (*callThis)(Param1, Param2, Param3),
		Param1 p1, Param2 p2, Param3 p3)
		:	function(callThis),
			p1(p1),
			p2(p2),
			p3(p3)
		{
		}

	virtual void operator()()
		{ FunctionObjectWithResult<Result>::result
			= (function)(p1.Pass(), p2.Pass(), p3.Pass()); }

private:
	Result (*function)(Param1, Param2, Param3);
	ParameterBinder<Param1> p1;
	ParameterBinder<Param2> p2;
	ParameterBinder<Param3> p3;
};


template <class Param1, class Param2, class Param3, class Param4>
class FourParamFunctionObject : public FunctionObject {
public:
	FourParamFunctionObject(void (*callThis)(Param1, Param2, Param3, Param4),
		Param1 p1, Param2 p2, Param3 p3, Param4 p4)
		:	function(callThis),
			p1(p1),
			p2(p2),
			p3(p3),
			p4(p4)
		{
		}

	virtual void operator()()
		{ (function)(p1.Pass(), p2.Pass(), p3.Pass(), p4.Pass()); }

private:
	void (*function)(Param1, Param2, Param3, Param4);
	ParameterBinder<Param1> p1;
	ParameterBinder<Param2> p2;
	ParameterBinder<Param3> p3;
	ParameterBinder<Param4> p4;
};


template <class Result, class Param1, class Param2, class Param3,
	class Param4>
class FourParamFunctionObjectWithResult : public
	FunctionObjectWithResult<Result>  {
public:
	FourParamFunctionObjectWithResult(
		Result (*callThis)(Param1, Param2, Param3, Param4),
		Param1 p1, Param2 p2, Param3 p3, Param4 p4)
		:	function(callThis),
			p1(p1),
			p2(p2),
			p3(p3),
			p4(p4)
		{
		}

	virtual void operator()()
		{ FunctionObjectWithResult<Result>::result
			= (function)(p1.Pass(), p2.Pass(), p3.Pass(), p4.Pass()); }

private:
	Result (*function)(Param1, Param2, Param3, Param4);
	ParameterBinder<Param1> p1;
	ParameterBinder<Param2> p2;
	ParameterBinder<Param3> p3;
	ParameterBinder<Param4> p4;
};


template<class T>
class PlainMemberFunctionObject : public FunctionObject {
public:
	PlainMemberFunctionObject(void (T::*function)(), T* onThis)
		:	function(function),
			target(onThis)
		{
		}

	virtual void operator()()
		{ (target->*function)(); }

private:
	void (T::*function)();
	T* target;
};


template<class T>
class PlainLockingMemberFunctionObject : public FunctionObject {
public:
	PlainLockingMemberFunctionObject(void (T::*function)(), T* target)
		:	function(function),
			messenger(target)
		{
		}

	virtual void operator()()
		{
			T* target = dynamic_cast<T*>(messenger.Target(NULL));
			if (!target || !messenger.LockTarget())
				return;
			(target->*function)();
			target->Looper()->Unlock();
		}

private:
	void (T::*function)();
	BMessenger messenger;
};


template<class T, class R>
class PlainMemberFunctionObjectWithResult : public
	FunctionObjectWithResult<R> {
public:
	PlainMemberFunctionObjectWithResult(R (T::*function)(), T* onThis)
		:	function(function),
			target(onThis)
		{
		}

	virtual void operator()()
		{ FunctionObjectWithResult<R>::result = (target->*function)(); }


private:
	R (T::*function)();
	T* target;
};


template<class T, class Param1>
class SingleParamMemberFunctionObject : public FunctionObject {
public:
	SingleParamMemberFunctionObject(void (T::*function)(Param1),
		T* onThis, Param1 p1)
		:	function(function),
			target(onThis),
			p1(p1)
		{
		}

	virtual void operator()()
		{ (target->*function)(p1.Pass()); }

private:
	void (T::*function)(Param1);
	T* target;
	ParameterBinder<Param1> p1;
};


template<class T, class Param1, class Param2>
class TwoParamMemberFunctionObject : public FunctionObject {
public:
	TwoParamMemberFunctionObject(void (T::*function)(Param1, Param2),
		T* onThis, Param1 p1, Param2 p2)
		:	function(function),
			target(onThis),
			p1(p1),
			p2(p2)
		{
		}

	virtual void operator()()
		{ (target->*function)(p1.Pass(), p2.Pass()); }


protected:
	void (T::*function)(Param1, Param2);
	T* target;
	ParameterBinder<Param1> p1;
	ParameterBinder<Param2> p2;
};


template<class T, class R, class Param1>
class SingleParamMemberFunctionObjectWithResult : public
	FunctionObjectWithResult<R> {
public:
	SingleParamMemberFunctionObjectWithResult(R (T::*function)(Param1),
		T* onThis, Param1 p1)
		:	function(function),
			target(onThis),
			p1(p1)
		{
		}

	virtual void operator()()
		{ FunctionObjectWithResult<R>::result
			= (target->*function)(p1.Pass()); }

protected:
	R (T::*function)(Param1);
	T* target;
	ParameterBinder<Param1> p1;
};


template<class T, class R, class Param1, class Param2>
class TwoParamMemberFunctionObjectWithResult : public
	FunctionObjectWithResult<R> {
public:
	TwoParamMemberFunctionObjectWithResult(R (T::*function)(Param1, Param2),
		T* onThis, Param1 p1, Param2 p2)
		:	function(function),
			target(onThis),
			p1(p1),
			p2(p2)
		{
		}

	virtual void operator()()
		{ FunctionObjectWithResult<R>::result
			= (target->*function)(p1.Pass(), p2.Pass()); }

protected:
	R (T::*function)(Param1, Param2);
	T* target;
	ParameterBinder<Param1> p1;
	ParameterBinder<Param2> p2;
};


// convenience factory functions
// NewFunctionObject
// NewMemberFunctionObject
// NewMemberFunctionObjectWithResult
// NewLockingMemberFunctionObject
//
// ... add the missing ones as needed

template<class Param1>
SingleParamFunctionObject<Param1>*
NewFunctionObject(void (*function)(Param1), Param1 p1)
{
	return new SingleParamFunctionObject<Param1>(function, p1);
}


template<class Param1, class Param2>
TwoParamFunctionObject<Param1, Param2>*
NewFunctionObject(void (*function)(Param1, Param2), Param1 p1, Param2 p2)
{
	return new TwoParamFunctionObject<Param1, Param2>(function, p1, p2);
}


template<class Param1, class Param2, class Param3>
ThreeParamFunctionObject<Param1, Param2, Param3>*
NewFunctionObject(void (*function)(Param1, Param2, Param3),
	Param1 p1, Param2 p2, Param3 p3)
{
	return new ThreeParamFunctionObject<Param1, Param2, Param3>
		(function, p1, p2, p3);
}


template<class T>
PlainMemberFunctionObject<T>*
NewMemberFunctionObject(void (T::*function)(), T* onThis)
{
	return new PlainMemberFunctionObject<T>(function, onThis);
}


template<class T, class Param1>
SingleParamMemberFunctionObject<T, Param1>*
NewMemberFunctionObject(void (T::*function)(Param1), T* onThis, Param1 p1)
{
	return new SingleParamMemberFunctionObject<T, Param1>
		(function, onThis, p1);
}


template<class T, class Param1, class Param2>
TwoParamMemberFunctionObject<T, Param1, Param2>*
NewMemberFunctionObject(void (T::*function)(Param1, Param2), T* onThis,
		Param1 p1, Param2 p2)
{
	return new TwoParamMemberFunctionObject<T, Param1, Param2>
		(function, onThis, p1, p2);
}


template<class T, class R, class Param1, class Param2>
TwoParamMemberFunctionObjectWithResult<T, R, Param1, Param2>*
NewMemberFunctionObjectWithResult(R (T::*function)(Param1, Param2),
	T* onThis, Param1 p1, Param2 p2)
{
	return new TwoParamMemberFunctionObjectWithResult<T, R, Param1, Param2>
		(function, onThis, p1, p2);
}


template<class HandlerOrSubclass>
PlainLockingMemberFunctionObject<HandlerOrSubclass>*
NewLockingMemberFunctionObject(void (HandlerOrSubclass::*function)(),
	HandlerOrSubclass* onThis)
{
	return new PlainLockingMemberFunctionObject<HandlerOrSubclass>
		(function, onThis);
}

} // namespace BPrivate

using namespace BPrivate;

#endif	// __FUNCTION_OBJECT__
