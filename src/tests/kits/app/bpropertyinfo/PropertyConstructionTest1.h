/*
	$Id: PropertyConstructionTest1.h,v 1.2 2002/08/13 05:02:46 jrand Exp $
	
	This file defines a class for performing one test of BPropertyInfo
	functionality.
	
	*/


#ifndef PropertyConstructionTest1_H
#define PropertyConstructionTest1_H


#include "../common.h"
#include <PropertyInfo.h>

	
class PropertyConstructionTest1 :
	public TestCase {
	
private:
	void CheckProperty(BPropertyInfo *propTest,
	                   property_info *prop_list,
	                   value_info *value_list,
	                   int32 prop_count,
	                   int32 value_count,
	                   ssize_t flat_size,
	                   const char *flat_data);
	
public:
	static Test *suite(void);
	void setUp(void);
	void PerformTest(void);
	PropertyConstructionTest1(std::string name = "");
	virtual ~PropertyConstructionTest1();
	};
	
#endif






