/*
	$Id: AddMessageTest2.h,v 1.2 2002/07/22 09:28:00 tylerdauwalder Exp $
	
	This file defines a class for performing one test of BMessageQueue
	functionality.
	
	*/


#ifndef AddMessageTest2_H
#define AddMessageTest2_H


#include "MessageQueueTestCase.h"
#include "../common.h"

	
 class AddMessageTest2 :
	public MessageQueueTestCase {
	
private:
	
public:
	static Test *suite(void);
	virtual void PerformTest(void);
	AddMessageTest2(std::string);
	virtual ~AddMessageTest2();
	};
	
#endif






