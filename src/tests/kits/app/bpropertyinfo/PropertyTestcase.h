/*
	$Id: PropertyTestcase.h,v 1.1 2002/08/18 03:44:21 jrand Exp $
	
	This file defines a base class for performing all tests of BPropertyInfo
	functionality.
	
	*/


#ifndef PropertyTestcase_H
#define PropertyTestcase_H


#include "../common.h"
#include <PropertyInfo.h>

	
class PropertyTestcase :
	public TestCase {
	
private:
	property_info *DuplicateProperties(const property_info *prop1, int prop_count);
	value_info *DuplicateValues(const value_info *value1, int value_count);
	
protected:
	virtual void TestProperty(BPropertyInfo *propTest,
	                          const property_info *prop_list,
	                          const value_info *value_list,
	                          int32 prop_count,
	                          int32 value_count,
	                          ssize_t flat_size,
	                          const char *flat_data) = 0;
	    
	static const char *uniquePropName;
	static const uint32 uniqueCommand;
	static const uint32 uniqueSpecifier;
	static const char *commonPropName;
	static const uint32 commonCommand;
	static const uint32 commonSpecifier;
	static const uint32 wildcardCommandTests[];
	static const uint32 wildcardSpecifierTests[];
	
public:
	void PerformTest(void);
	PropertyTestcase(std::string name = "");
	virtual ~PropertyTestcase();
	};
	
#endif