/*
	$Id: PropertyConstructionTest.cpp 851 2002-08-22 03:15:35Z jrand $
	
	This file implements the construction test for the OpenBeOS BPropertyInfo
	code.  This class tests the following usecases:
	  - Construction 1 to 4
	  - Destruction
	  - Properties
	  - Values
	  - Count Properties
	  - Count Values
	  - Unflatten
	
	*/


#include "PropertyConstructionTest.h"

#include <assert.h>


/*
 *  Method:  PropertyConstructionTest::PropertyConstructionTest()
 *   Descr:  This is the constructor for this class.
 */
		

	PropertyConstructionTest::PropertyConstructionTest(std::string name) :
		PropertyTestcase(name)
{
	}


/*
 *  Method:  PropertyConstructionTest::~PropertyConstructionTest()
 *   Descr:  This is the destructor for this class.
 */
 

	PropertyConstructionTest::~PropertyConstructionTest()
{
	}


/*
 *  Method:  PropertyConstructionTest::CompareProperties()
 *   Descr:  This member function checks that the two property_info structures
 *           passed in match.
 */	


	void PropertyConstructionTest::CompareProperties(
	    const property_info *prop1,
	    const property_info *prop2,
	    int prop_count)
{
	int i, j, k;

	if ((prop_count != 0) && (prop1 != prop2)) {
		assert(prop1 != NULL);
		assert(prop2 != NULL);
		for (i = 0; i < prop_count; i++) {
			assert(prop1[i].name != 0);
			assert(prop2[i].name != 0);
			assert(strcmp(prop1[i].name, prop2[i].name) == 0);
			
			for(j = 0; j < 10; j++) {
				assert(prop1[i].commands[j] == prop2[i].commands[j]);
				if (prop1[i].commands[j] == 0) {
					break;
				}
			}
			
			for(j = 0; j < 10; j++) {
				assert(prop1[i].specifiers[j] == prop2[i].specifiers[j]);
				if (prop1[i].specifiers[j] == 0) {
					break;
				}
			}
			
			if (prop1[i].usage != prop2[i].usage) {
				if (prop1[i].usage == NULL) {
					assert(prop2[i].usage == NULL);
				} else {
					assert(strcmp(prop1[i].usage, prop2[i].usage) == 0);
				}
			}
			
			assert(prop1[i].extra_data == prop2[i].extra_data);
			
			for(j = 0; j < 10; j++) {
				assert(prop1[i].types[j] == prop2[i].types[j]);
				if (prop1[i].types[j] == 0) {
					break;
				}
			}
			
			for(j = 0; j < 3; j++) {
				for(k = 0; k < 5; k++) {
					if (prop1[i].ctypes[j].pairs[k].name == 0) {
						assert(prop2[i].ctypes[j].pairs[k].name == 0);
						break;
					} else {
						assert(prop2[i].ctypes[j].pairs[k].name != 0);
						assert(strcmp(prop1[i].ctypes[j].pairs[k].name,
						              prop2[i].ctypes[j].pairs[k].name) == 0);
						assert(prop1[i].ctypes[j].pairs[k].type ==
						       prop2[i].ctypes[j].pairs[k].type);
					}
				}
				if (prop1[i].ctypes[j].pairs[0].name == 0) {
					break;
				}
			}
		}
	}
}


/*
 *  Method:  PropertyConstructionTest::CompareValues()
 *   Descr:  This member function checks that the two value_info structures
 *           passed in match.
 */	


	void PropertyConstructionTest::CompareValues(
	    const value_info *value1,
	    const value_info *value2,
	    int value_count)
{
	int i;
	if ((value_count != 0) && (value1 != value2)) {
		assert(value1 != NULL);
		assert(value2 != NULL);
		for (i = 0; i < value_count; i++) {
			assert(value1[i].name != 0);
			assert(value2[i].name != 0);
			assert(strcmp(value1[i].name, value2[i].name) == 0);
			
			assert(value1[i].value == value2[i].value);
			
			assert(value1[i].kind == value2[i].kind);
			
			if (value1[i].usage == 0) {
				assert(value2[i].usage == 0);
			} else {
				assert(value2[i].usage != 0);
				assert(strcmp(value1[i].usage, value2[i].usage) == 0);
			}
			
			assert(value1[i].extra_data == value2[i].extra_data);
		}
	}		
}


/*
 *  Method:  PropertyConstructionTest::TestProperty()
 *   Descr:  This member function performs this test.
 */	


	void PropertyConstructionTest::TestProperty(
		BPropertyInfo *propTest,
	    const property_info *prop_list,
	    const value_info *value_list,
	    int32 prop_count,
	    int32 value_count,
	    ssize_t flat_size,
	    const char *lflat_data,
	    const char *bflat_data)
{
	assert(propTest->CountProperties() == prop_count);
	assert(propTest->CountValues() == value_count);
	CompareProperties(propTest->Properties(), prop_list, prop_count);
	CompareValues(propTest->Values(), value_list, value_count);
	}
	

/*
 *  Method:  PropertyConstructionTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "PropertyConstructionTest".  The test
 *           is created as a ThreadedTestCase (typedef'd as
 *           PropertyConstructionTestCaller) with only one thread.
 */

 Test *PropertyConstructionTest::suite(void)
{	
	typedef CppUnit::TestCaller<PropertyConstructionTest>
		PropertyConstructionTestCaller;
		
	return(new PropertyConstructionTestCaller("BPropertyInfo::Construction Test", &PropertyConstructionTest::PerformTest));
	}
 

