/*
	$Id: ConcurrencyTest2.h,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file defines a classes for performing one test of BMessageQueue
	functionality.
	
	*/


#ifndef ConcurrencyTest2_H
#define ConcurrencyTest2_H


#include "MessageQueueTestCase.h"
#include "Test.h"
#include "ThreadedTestCaller.h"

	
template<class MessageQueue> class ConcurrencyTest2 :
	public MessageQueueTestCase<MessageQueue> {
	
private:
	typedef ThreadedTestCaller<ConcurrencyTest2<MessageQueue> >
		ConcurrencyTest2Caller;
		
	bool unlockTest;
	bool isLocked;
	BMessage *removeMessage;
	
public:
	static Test *suite(void);
	void setUp(void);
	void TestThread1(void);
	void TestThread2(void);
	void TestThread3(void);
	void TestThread4(void);
	void TestThread5(void);
	ConcurrencyTest2(std::string, bool);
	virtual ~ConcurrencyTest2();
	};
	
#endif