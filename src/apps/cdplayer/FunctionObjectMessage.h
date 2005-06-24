/*
	Copyright 1999, Be Incorporated.   All Rights Reserved.
	This file may be used under the terms of the Be Sample Code License.
*/

// This defines some function object glue code

// See the Be Newsletter article about using Function Objects in the Be messaging
// model for more information

#ifndef __FUNCTION_OBJECT_MESSAGE__
#define __FUNCTION_OBJECT_MESSAGE__

#ifndef _BE_H
#include <Message.h>
#include <MessageFilter.h>
#endif

class FunctionObject {
public:
	virtual void operator()() = 0;
	virtual ~FunctionObject() {}
	virtual ulong Size() const = 0;
};

template<class FT, class T>
class PlainMemberFunctionObject : public FunctionObject {
public:
	PlainMemberFunctionObject(FT callThis, T *onThis)
		:	function(callThis),
			target(onThis)
		{
		}
	virtual ~PlainMemberFunctionObject() {}

	virtual void operator()()
		{ (target->*function)(); }

	virtual ulong Size() const { return sizeof(*this); }
private:
	FT function;
	T *target;
};

template<class FT, class T, class P>
class SingleParamMemberFunctionObject : public FunctionObject {
public:
	SingleParamMemberFunctionObject(FT callThis, T *onThis, P withThis)
		:	function(callThis),
			target(onThis),
			parameter(withThis)
		{
		}
	virtual ~SingleParamMemberFunctionObject() {}

	virtual void operator()()
		{ (target->*function)(parameter); }

	virtual ulong Size() const { return sizeof(*this); }
private:
	FT function;
	T *target;
	P parameter;
};

class FunctorFactoryCommon {
public:
	static bool DispatchIfFunctionObject(BMessage *);
protected:
	static BMessage *NewMessage(const FunctionObject *);
};

#endif