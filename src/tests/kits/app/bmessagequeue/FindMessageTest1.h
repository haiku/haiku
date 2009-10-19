/*
	$Id: FindMessageTest1.h 383 2002-07-22 09:28:00Z tylerdauwalder $
	
	This file defines a classes for performing one test of BMessageQueue
	functionality.
	
	*/


#ifndef FindMessageTest1_H
#define FindMessageTest1_H


#include "MessageQueueTestCase.h"
#include "../common.h"

	
 class FindMessageTest1 :
	public MessageQueueTestCase {
	
private:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	FindMessageTest1(std::string);
	virtual ~FindMessageTest1();
	};
	
#endif






