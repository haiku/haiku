/*
	$Id: DeskbarAddItemTest.h,v 1.2 2002/09/28 07:27:00 shatty Exp $
	
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
