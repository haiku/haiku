/*
	$Id: AddMessageTest1.h,v 1.1 2002/07/09 12:24:56 ejakowatz Exp $
	
	This file defines a class for performing one test of BMessageQueue
	functionality.
	
	*/


#ifndef AddMessageTest1_H
#define AddMessageTest1_H


#include "MessageQueueTestCase.h"
#include "Test.h"
#include "ThreadedTestCaller.h"

	
template<class MessageQueue> class AddMessageTest1 :
	public MessageQueueTestCase<MessageQueue> {
	
private:
	typedef ThreadedTestCaller <AddMessageTest1<MessageQueue> >
		AddMessageTest1Caller;
	
public:
	static Test *suite(void);
	void setUp(void);
	void PerformTest(void);
	AddMessageTest1(std::string);
	virtual ~AddMessageTest1();
	};
	
#endif