/*
	$Id: ConcurrencyTest2.h 383 2002-07-22 09:28:00Z tylerdauwalder $
	
	This file defines a classes for performing one test of BMessageQueue
	functionality.
	
	*/


#ifndef _ConcurrencyTest2_H
#define _ConcurrencyTest2_H


#include "MessageQueueTestCase.h"
#include "../common.h"

	
 class ConcurrencyTest2 :
	public MessageQueueTestCase {
	
private:
		
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




