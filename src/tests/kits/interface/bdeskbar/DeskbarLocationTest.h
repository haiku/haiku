/*
	$Id: DeskbarLocationTest.h,v 1.2 2002/09/28 07:27:00 shatty Exp $
	
	This file defines a class for performing a test of BDeskbar
	functionality.
	
	*/


#ifndef DeskbarLocationTest_H
#define DeskbarLocationTest_H


#include "../common.h"
#include <Deskbar.h>

	
class DeskbarLocationTest :
	public TestCase {
	
private:
	void CheckLocation(BDeskbar *, deskbar_location, bool, bool);
	void TestLocation(BDeskbar *, deskbar_location);
	
protected:
	
public:
	static Test *suite(void);
	void PerformTest(void);
	DeskbarLocationTest(std::string name = "");
	virtual ~DeskbarLocationTest();
	};
	
#endif
