/*
	$Id: DeskbarGetItemTest.h,v 1.2 2002/09/13 03:51:09 jrand Exp $
	
	This file defines a class for performing a test of BDeskbar
	functionality.
	
	*/


#ifndef DeskbarGetItemTest_H
#define DeskbarGetItemTest_H


#include "../common.h"
#include <Deskbar.h>

	
class DeskbarGetItemTest :
	public TestCase {
	
private:
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	DeskbarGetItemTest(std::string name = "");
	virtual ~DeskbarGetItemTest();
	};
	
#endif