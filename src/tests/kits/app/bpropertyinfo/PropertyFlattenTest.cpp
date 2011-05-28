/*
	$Id: PropertyFlattenTest.cpp 1218 2002-09-28 00:19:49Z shatty $
	
	This file implements the flatten test for the OpenBeOS BPropertyInfo code.
	This class tests the following usecases:
	  - Fixed Size
	  - Type Code
	  - Allows Type Code
	  - Flattened Size
	  - Flatten
	
	*/


#include "PropertyFlattenTest.h"

#include <assert.h>

#include <ByteOrder.h>


/*
 *  Method:  PropertyFlattenTest::PropertyFlattenTest()
 *   Descr:  This is the constructor for this class.
 */	

	PropertyFlattenTest::PropertyFlattenTest(std::string name) :
		PropertyTestcase(name)
{
	}


/*
 *  Method:  PropertyFlattenTest::~PropertyFlattenTest()
 *   Descr:  This is the destructor for this class.
 */

	PropertyFlattenTest::~PropertyFlattenTest()
{
	}


/*
 *  Method:  PropertyFlattenTest::TestProperty()
 *   Descr:  This member function performs this test.
 */	

	void PropertyFlattenTest::TestProperty(
		BPropertyInfo *propTest,
	    const property_info *prop_list,
	    const value_info *value_list,
	    int32 prop_count,
	    int32 value_count,
	    ssize_t flat_size,
	    const char *lflat_data,
	    const char *bflat_data)
{
	char buffer[768];
	
	assert(!propTest->IsFixedSize());
	assert(propTest->TypeCode() == B_PROPERTY_INFO_TYPE);
	assert(propTest->AllowsTypeCode(B_PROPERTY_INFO_TYPE));
	assert(!propTest->AllowsTypeCode(B_TIME_TYPE));
	assert(propTest->FlattenedSize() == flat_size);
	assert(propTest->Flatten(buffer, sizeof(buffer)/ sizeof(buffer[0])) == B_OK);
	if (B_HOST_IS_BENDIAN) {
		assert(memcmp(buffer, bflat_data, propTest->FlattenedSize()) == 0);
	} else {
		assert(memcmp(buffer, lflat_data, propTest->FlattenedSize()) == 0);
	}
}


/*
 *  Method:  PropertyFlattenTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "PropertyFlattenTest".
 */

 Test *PropertyFlattenTest::suite(void)
{	
	typedef CppUnit::TestCaller<PropertyFlattenTest>
		PropertyFlattenTestCaller;
		
	return(new PropertyFlattenTestCaller("BPropertyInfo::Flatten Test", &PropertyFlattenTest::PerformTest));
	}
