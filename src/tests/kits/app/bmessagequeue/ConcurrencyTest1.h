/*
	$Id: ConcurrencyTest1.h 383 2002-07-22 09:28:00Z tylerdauwalder $
	
	This file defines a classes for performing one test of BMessageQueue
	functionality.
	
	*/


#ifndef ConcurrencyTest1_H
#define ConcurrencyTest1_H


#include "MessageQueueTestCase.h"
#include "../common.h"
#include <Locker.h>

	
 class ConcurrencyTest1 :
	public MessageQueueTestCase {
	
private:

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






