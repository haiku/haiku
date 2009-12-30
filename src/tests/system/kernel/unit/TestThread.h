/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef TEST_THREAD_H
#define TEST_THREAD_H


#include <KernelExport.h>

#include "TestContext.h"


template<typename ObjectType, typename ParameterType>
class TestThread {
public:
	TestThread(ObjectType* object,
		void (ObjectType::*method)(TestContext&, ParameterType*),
		ParameterType* argument)
		:
		fObject(object),
		fMethod(method),
		fArgument(argument)
	{
	}

	thread_id Spawn(const char* name, int32 priority)
	{
		return GlobalTestContext::Current()->SpawnThread(_Entry, name, priority,
			this);
	}

private:
	static status_t _Entry(void* data)
	{
		TestThread* thread = (TestThread*)data;
		(thread->fObject->*thread->fMethod)(
			*GlobalTestContext::Current()->CurrentContext(), thread->fArgument);
		delete thread;
		return B_OK;
	}

private:
	ObjectType*		fObject;
	void			(ObjectType::*fMethod)(TestContext&, ParameterType*);
	ParameterType*	fArgument;
};


template<typename ObjectType, typename ParameterType>
thread_id
SpawnThread(ObjectType* object,
	void (ObjectType::*method)(TestContext&, ParameterType*), const char* name,
	int32 priority, ParameterType* arg)
{
	TestThread<ObjectType, ParameterType>* thread
		= new(std::nothrow) TestThread<ObjectType, ParameterType>(object,
			method, arg);
	if (thread == NULL)
		return B_NO_MEMORY;

	return thread->Spawn(name, priority);
}


#endif	// TEST_THREAD_H
