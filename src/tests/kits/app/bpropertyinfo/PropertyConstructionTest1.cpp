/*
	$Id: PropertyConstructionTest1.cpp,v 1.2 2002/08/08 04:31:43 jrand Exp $
	
	This file implements the first test for the OpenBeOS BPropertyInfo code.
	It tests the Construction use cases.  It does so by doing the following:
	
	*/


#include "PropertyConstructionTest1.h"
#include <PropertyInfo.h>
#include <AppDefs.h>
#include <Message.h>
#include <TypeConstants.h>


/*
 *  Method:  PropertyConstructionTest1::PropertyConstructionTest1()
 *   Descr:  This is the constructor for this class.
 */
		

	PropertyConstructionTest1::PropertyConstructionTest1(std::string name) :
		TestCase(name)
{
	}


/*
 *  Method:  PropertyConstructionTest1::~PropertyConstructionTest1()
 *   Descr:  This is the destructor for this class.
 */
 

	PropertyConstructionTest1::~PropertyConstructionTest1()
{
	}
	
	
/*
 *  Method:  PropertyConstructionTest1::setUp()
 *   Descr:  This member function is called just prior to running the test.
 *           It resets the destructor count for testMessageClass.
 */


	void PropertyConstructionTest1::setUp(void)
{
	}


/*
 *  Method:  PropertyConstructionTest1::PerformTest()
 *   Descr:  This member function performs this test.  It adds
 *           10000 messages to the message queue and confirms that
 *           the queue contains the right messages.  Then it confirms
 *           that all 10000 messages are deleted when the message
 *           queue is deleted.
 */	


	void PropertyConstructionTest1::PerformTest(void)
{
	struct property_info prop1[] = { 0 };
	struct property_info prop2[] = {
		{ "duck", {B_GET_PROPERTY, B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, B_INDEX_SPECIFIER, 0}, "get or set duck"}, 
    	{ "head", {B_GET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "get head"}, 
        { "head", {B_SET_PROPERTY, 0}, {B_DIRECT_SPECIFIER, 0}, "set head"}, 
        { "feet", {0}, {0}, "can do anything with his orange feet"}, 
        0 // terminate list 
    }; 
	struct property_info *prop_lists[] = { NULL, prop1, prop2 };
	int prop_counts[] = { 0, 0, 4 };
	
	struct value_info value1[] = { 0 };
	struct value_info value2[] = {
		{ "Value1", 5, B_COMMAND_KIND, "This is the usage" },
		{ "Value2", 6, B_TYPE_CODE_KIND, "This is the usage" },
		0
	};
	struct value_info *value_lists[] = { NULL, value1, value2 };
	int value_counts[] = { 0, 0, 2 };
	
	BPropertyInfo *propTest = new BPropertyInfo;
	assert(propTest->Properties() == NULL);
	assert(propTest->Values() == NULL);
	assert(propTest->CountProperties() == 0);
	assert(propTest->CountValues() == 0);
	assert(!propTest->IsFixedSize());
	assert(propTest->TypeCode() == B_PROPERTY_INFO_TYPE);
	assert(propTest->AllowsTypeCode(B_PROPERTY_INFO_TYPE));
	assert(!propTest->AllowsTypeCode(B_TIME_TYPE));
	delete propTest;
	
	int i, j;

	for (i=0; i < sizeof(prop_counts) / sizeof(int); i++) {
		propTest = new BPropertyInfo(prop_lists[i]);
		assert(propTest->Properties() == prop_lists[i]);
		assert(propTest->Values() == NULL);
		assert(propTest->CountProperties() == prop_counts[i]);
		assert(propTest->CountValues() == 0);
		assert(!propTest->IsFixedSize());
		assert(propTest->TypeCode() == B_PROPERTY_INFO_TYPE);
		assert(propTest->AllowsTypeCode(B_PROPERTY_INFO_TYPE));
		assert(!propTest->AllowsTypeCode(B_TIME_TYPE));
		delete propTest;
	
		for (j=0; j < sizeof(value_counts) / sizeof(int); j++) {
			propTest = new BPropertyInfo(prop_lists[i], value_lists[j]);
			assert(propTest->Properties() == prop_lists[i]);
			assert(propTest->Values() == value_lists[j]);
			assert(propTest->CountProperties() == prop_counts[i]);
			assert(propTest->CountValues() == value_counts[j]);
			assert(!propTest->IsFixedSize());
			assert(propTest->TypeCode() == B_PROPERTY_INFO_TYPE);
			assert(propTest->AllowsTypeCode(B_PROPERTY_INFO_TYPE));
			assert(!propTest->AllowsTypeCode(B_TIME_TYPE));
			delete propTest;
			
			propTest = new BPropertyInfo(prop_lists[i], value_lists[j], false);
			assert(propTest->Properties() == prop_lists[i]);
			assert(propTest->Values() == value_lists[j]);
			assert(propTest->CountProperties() == prop_counts[i]);
			assert(propTest->CountValues() == value_counts[j]);
			assert(!propTest->IsFixedSize());
			assert(propTest->TypeCode() == B_PROPERTY_INFO_TYPE);
			assert(propTest->AllowsTypeCode(B_PROPERTY_INFO_TYPE));
			assert(!propTest->AllowsTypeCode(B_TIME_TYPE));
			delete propTest;
		}
	}
}


/*
 *  Method:  PropertyConstructionTest1::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "PropertyConstructionTest1".  The test
 *           is created as a ThreadedTestCase (typedef'd as
 *           PropertyConstructionTest1Caller) with only one thread.
 */

 Test *PropertyConstructionTest1::suite(void)
{	
	typedef CppUnit::TestCaller<PropertyConstructionTest1>
		PropertyConstructionTest1Caller;
		
	return(new PropertyConstructionTest1Caller("BPropertyInfo::Construction Test", &PropertyConstructionTest1::PerformTest));
	}
 

