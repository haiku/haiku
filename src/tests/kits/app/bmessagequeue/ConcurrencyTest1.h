/*
	$Id: ConcurrencyTest1.h,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file defines a classes for performing one test of BMessageQueue
	functionality.
	
	*/


#ifndef ConcurrencyTest1_H
#define ConcurrencyTest1_H


#include "MessageQueueTestCase.h"
#include "Test.h"
#include "ThreadedTestCaller.h"
#include <Locker.h>

	
template<class MessageQueue> class ConcurrencyTest1 :
	public MessageQueueTestCase<MessageQueue> {
	
private:
	typedef ThreadedTestCaller<ConcurrencyTest1<MessageQueue> >
		ConcurrencyTest1Caller;

	bool useList;
	BLocker thread2Lock;
	BLocker thread3Lock;
	
protected:
	
public:
	static Test *suite(void);
	void setUp(void);
	void TestThread1(void);
	void TestThread2(void);
	void TestThread3(void);
	ConcurrencyTest1(std::string, bool);
	virtual ~ConcurrencyTest1();
	};
	
#endif