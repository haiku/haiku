/*
	$Id: PropertyConstructionTest1.h,v 1.1 2002/08/07 05:12:53 jrand Exp $
	
	This file defines a class for performing one test of BPropertyInfo
	functionality.
	
	*/


#ifndef PropertyConstructionTest1_H
#define PropertyConstructionTest1_H


#include "../common.h"

	
class PropertyConstructionTest1 :
	public TestCase {
	
private:
	
public:
	static Test *suite(void);
	void setUp(void);
	void PerformTest(void);
	PropertyConstructionTest1(std::string name = "");
	virtual ~PropertyConstructionTest1();
	};
	
#endif






