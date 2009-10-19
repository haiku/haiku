/*
	$Id: PropertyFlattenTest.h 1218 2002-09-28 00:19:49Z shatty $
	
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
	                  const char *lflat_data,
	                  const char *bflat_data);
	
public:
	static Test *suite(void);
	PropertyFlattenTest(std::string name = "");
	virtual ~PropertyFlattenTest();
	};
	
#endif
