/*
	$Id: PropertyConstructionTest.h 1218 2002-09-28 00:19:49Z shatty $
	
	This file defines a class for performing one test of BPropertyInfo
	functionality.
	
	*/


#ifndef PropertyConstructionTest_H
#define PropertyConstructionTest_H


#include "PropertyTestcase.h"
#include <PropertyInfo.h>

	
class PropertyConstructionTest :
	public PropertyTestcase {
	
private:
	void CompareProperties(const property_info *prop1,
	                       const property_info *prop2,
	                       int prop_count);
	void CompareValues(const value_info *value1,
	                   const value_info *value2,
	                   int value_count);

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
	PropertyConstructionTest(std::string name = "");
	virtual ~PropertyConstructionTest();
	};
	
#endif
