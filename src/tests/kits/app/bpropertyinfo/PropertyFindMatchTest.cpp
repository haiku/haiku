/*
	$Id: PropertyFindMatchTest.cpp 851 2002-08-22 03:15:35Z jrand $
	
	This file implements the FindMatch test for the OpenBeOS BPropertyInfo
	code.  This class tests the following usecases:
	  - Find Match
	
	*/


#include "PropertyFindMatchTest.h"

#include <assert.h>

#include <Message.h>


/*
 *  Method:  PropertyFindMatchTest::PropertyFindMatchTest()
 *   Descr:  This is the constructor for this class.
 */
		

	PropertyFindMatchTest::PropertyFindMatchTest(std::string name) :
		PropertyTestcase(name)
{
	}


/*
 *  Method:  PropertyFindMatchTest::~PropertyFindMatchTest()
 *   Descr:  This is the destructor for this class.
 */
 

	PropertyFindMatchTest::~PropertyFindMatchTest()
{
	}
	

/*
 *  Method:  PropertyFindMatchTest::TestProperty()
 *   Descr:  This member function performs this test.
 */	


	void PropertyFindMatchTest::TestProperty(
		BPropertyInfo *propTest,
	    const property_info *prop_list,
	    const value_info *value_list,
	    int32 prop_count,
	    int32 value_count,
	    ssize_t flat_size,
	    const char *lflat_data,
	    const char *bflat_data)
{
	const uint32 *commands;
	const uint32 *specifiers;
	const property_info *theProps = propTest->Properties();
	int i, j, k;
	bool wildcardCommand, wildcardSpec;
	
	ExecFindMatch(propTest, uniquePropName, uniqueCommand, uniqueSpecifier, false, -1);
	ExecFindMatch(propTest, commonPropName, uniqueCommand, uniqueSpecifier, false, -1);
	ExecFindMatch(propTest, uniquePropName, commonCommand, uniqueSpecifier, false, -1);
	ExecFindMatch(propTest, uniquePropName, uniqueCommand, commonSpecifier, false, -1);
	
	for (i=0; i < prop_count; i++) {
		wildcardCommand = (theProps[i].commands[0] == 0);
		wildcardSpec = (theProps[i].specifiers[0] == 0);
		if (wildcardCommand) {
			commands = wildcardCommandTests;
		} else {
			commands = theProps[i].commands;
			}
		if (wildcardSpec) {
			specifiers = wildcardSpecifierTests;
		} else {
			specifiers = theProps[i].specifiers;
		}
		for(j=0; j<10; j++) {
			if (commands[j] == 0) {
				break;
			}
			if (!wildcardSpec) {
				ExecFindMatch(propTest, theProps[i].name, commands[j], uniqueSpecifier,
							  wildcardCommand, -1);
			}
			for(k=0; k<10; k++) {
				if (specifiers[k] == 0) {
					break;
				}
				if (!wildcardCommand) {
					ExecFindMatch(propTest, theProps[i].name, uniqueCommand, specifiers[k],
							      wildcardCommand, -1);
				}
				ExecFindMatch(propTest, theProps[i].name, commands[j], specifiers[k],
							  wildcardCommand, i);
			}
		}	
	}
}	


/*
 *  Method:  PropertyFindMatchTest::ExecFindMatch()
 *   Descr:  This member function executes the FindMatch() member on the
 *           BPropertyInfo instance and ensures that the result is what is
 *           expected.  It calls FindMatch() normally with a zero index
 *           (meaning to match wildcard and non-wildcard command instances)
 *           and with a non-NULL specifier message.  The extra_data member
 *           is checked to make sure it is what is expected if a match is
 *           returned.
 *
 *           The Be implementation takes a pointer to a BMessage specifier
 *           but it doesn't seem to need it.  So, the FindMatch() is called
 *           again with a NULL BMessage specifier and we expect the same
 *           result.
 *
 *           Finally, the result is checked with a non-zero index.  If index
 *           is non-zero, a match will only be found if the property uses
 *           a wildcard for the command.  Depending on whether we are testing
 *           a wildcard command (from the wildcardCommand flag), we check the
 *           result with and without a BMessage specifier.
 */	


	void PropertyFindMatchTest::ExecFindMatch(
		BPropertyInfo *propTest,
		const char *prop,
		uint32 comm,
		uint32 spec,
		bool wildcardCommand,
		int32 result
		)
{
	BMessage msg(comm);
	BMessage specMsg(spec);
	specMsg.AddString("property", prop);
	msg.AddSpecifier(&specMsg);
	uint32 extra_data;
	
	assert(propTest->FindMatch(&msg, 0, &specMsg, spec, prop, &extra_data) == result);
	if (result >= 0) {
		assert((propTest->Properties())[result].extra_data == extra_data);
	}
	assert(propTest->FindMatch(&msg, 0, NULL, spec, prop, &extra_data) == result);
	if (wildcardCommand) {
		assert(propTest->FindMatch(&msg, 1, &specMsg, spec, prop, &extra_data) == result);
		assert(propTest->FindMatch(&msg, 1, NULL, spec, prop, &extra_data) == result);
	} else {
		assert(propTest->FindMatch(&msg, 1, &specMsg, spec, prop, &extra_data) == -1);
		assert(propTest->FindMatch(&msg, 1, NULL, spec, prop, &extra_data) == -1);
	}
}


/*
 *  Method:  PropertyFindMatchTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "PropertyFindMatchTest".  The test
 *           is created as a ThreadedTestCase (typedef'd as
 *           PropertyFindMatchTestCaller) with only one thread.
 */

 Test *PropertyFindMatchTest::suite(void)
{	
	typedef CppUnit::TestCaller<PropertyFindMatchTest>
		PropertyFindMatchTestCaller;
		
	return(new PropertyFindMatchTestCaller("BPropertyInfo::FindMatch Test", &PropertyFindMatchTest::PerformTest));
	}
 

