/*
	$Id: PropertyFlattenTest.h,v 1.1 2002/08/18 03:44:21 jrand Exp $
	
	This file defines a class for performing one test of BPropertyInfo
	functionality.
	
	*/


#ifndef PropertyFlattenTest_H
#define PropertyFlattenTest_H


#include "PropertyTestcase.h"
#include <PropertyInfo.h>

	
class PropertyFlattenTest :
	public PropertyTestcase {
	
protected:
	void TestProperty(BPropertyInfo *propTest,
	                  const property_info *prop_list,
	                  const value_info *value_list,
	                  int32 prop_count,
	                  int32 value_count,
	                  ssize_t flat_size,
	                  const char *flat_data);
	
public:
	static Test *suite(void);
	PropertyFlattenTest(std::string name = "");
	virtual ~PropertyFlattenTest();
	};
	
#endif