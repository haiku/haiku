/*
	$Id: AddMessageTest2.h,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file defines a class for performing one test of BMessageQueue
	functionality.
	
	*/


#ifndef AddMessageTest2_H
#define AddMessageTest2_H


#include "MessageQueueTestCase.h"
#include "Test.h"
#include "ThreadedTestCaller.h"

	
template<class MessageQueue> class AddMessageTest2 :
	public MessageQueueTestCase<MessageQueue> {
	
private:
	typedef ThreadedTestCaller <AddMessageTest2<MessageQueue> >
		AddMessageTest2Caller;
	
public:
	static Test *suite(void);
	virtual void PerformTest(void);
	AddMessageTest2(std::string);
	virtual ~AddMessageTest2();
	};
	
#endif