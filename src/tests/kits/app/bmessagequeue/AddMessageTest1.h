/*
	$Id: AddMessageTest1.h 383 2002-07-22 09:28:00Z tylerdauwalder $
	
	This file defines a class for performing one test of BMessageQueue
	functionality.
	
	*/


#ifndef AddMessageTest1_H
#define AddMessageTest1_H


#include "MessageQueueTestCase.h"
#include "../common.h"

	
 class AddMessageTest1 :
	public MessageQueueTestCase {
	
private:
	
public:
	static Test *suite(void);
	void setUp(void);
	void PerformTest(void);
	AddMessageTest1(std::string);
	virtual ~AddMessageTest1();
	};
	
#endif






