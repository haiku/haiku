/*
	$Id: FindMessageTest1.h,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file defines a classes for performing one test of BMessageQueue
	functionality.
	
	*/


#ifndef FindMessageTest1_H
#define FindMessageTest1_H


#include "MessageQueueTestCase.h"
#include "Test.h"
#include "ThreadedTestCaller.h"

	
template<class MessageQueue> class FindMessageTest1 :
	public MessageQueueTestCase<MessageQueue> {
	
private:
	typedef ThreadedTestCaller<FindMessageTest1<MessageQueue> >
		FindMessageTest1Caller;
	
public:
	static Test *suite(void);
	void PerformTest(void);
	FindMessageTest1(std::string);
	virtual ~FindMessageTest1();
	};
	
#endif