/*
	$Id: DeskbarAddItemTest.h,v 1.1 2002/09/22 05:27:25 jrand Exp $
	
	This file defines a class for performing a test of BDeskbar
	functionality.
	
	*/


#ifndef DeskbarAddItemTest_H
#define DeskbarAddItemTest_H


#include "../common.h"
#include <Deskbar.h>

	
class DeskbarAddItemTest :
	public TestCase {
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	DeskbarAddItemTest(std::string name = "");
	virtual ~DeskbarAddItemTest();
	};
	
#endif