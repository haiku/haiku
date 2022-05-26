/*
	$Id: DeskbarGetItemTest.cpp 1236 2002-09-28 07:27:00Z shatty $
	
	This file implements tests for the following use cases of BDeskbar:
	  - Count Items
	  - Has Item 1
	  - Has Item 2
	  - Get Item Info 1
	  - Get Item Info 2
	
	*/


#include "DeskbarGetItemTest.h"
#include <Deskbar.h>

#include <assert.h>


/*
 *  Method:  DeskbarGetItemTest::DeskbarGetItemTest()
 *   Descr:  This is the constructor for this class.
 */
		

	DeskbarGetItemTest::DeskbarGetItemTest(std::string name) :
		TestCase(name)
{
	}


/*
 *  Method:  DeskbarGetItemTest::~DeskbarGetItemTest()
 *   Descr:  This is the destructor for this class.
 */
 

	DeskbarGetItemTest::~DeskbarGetItemTest()
{
	}


/*
 *  Method:  DeskbarGetItemTest::PerformTest()
 *   Descr:  This member function iterates over all of the items in the 
 *           deskbar shelf and gets information about each item and confirms
 *           that all of the information is self-consistent.
 */


	void DeskbarGetItemTest::PerformTest(void)
{
	BDeskbar myDeskbar;
	
	int32 itemCount = myDeskbar.CountItems();
	assert(itemCount >= 0);
	
	
	int32 id=0;
	int32 lastFoundId = -1;
	char buffer[1024];
	const char *name = buffer;
	
	assert(!myDeskbar.HasItem("NameThatDoesNotExistWeHope!!"));
	assert(myDeskbar.GetItemInfo("NameThatDoesNotExistWeHope!!", &id) == B_NAME_NOT_FOUND);
	
	for(id = 0; ((id < 10000) && (itemCount > 0)); id++) {
		int32 tmpId;
		
		if (myDeskbar.HasItem(id)) {
			itemCount--;
			
			/* In Be's implementation, if the char * points to NULL, it
			   returns B_BAD_VALUE.  However, no matter what it points to
			   before you start, it is changed by the call to GetItemInfo().
			   So, if it points to allocated memory, there is a good chance
			   for a leak.  I would argue that Be should return B_BAD_VALUE
			   if it points to non-NULL.  The Haiku implementation does
			   not return B_BAD_VALUE in this case so this assert would fail
			   from Haiku.  However, this is considered to be an acceptable
			   deviation from Be's implementation.
			name = NULL;
			assert(myDeskbar.GetItemInfo(id, &name) == B_BAD_VALUE); */
			
			name = buffer;
			assert(myDeskbar.GetItemInfo(id, &name) == B_OK);
			
			assert(name != buffer);
			assert(myDeskbar.HasItem(name));
			assert(myDeskbar.GetItemInfo(name, &tmpId) == B_OK);
			delete[] name;
			name = buffer;
			assert(tmpId == id);
			lastFoundId = id;
		} else {
			assert(myDeskbar.GetItemInfo(id, &name) == B_NAME_NOT_FOUND);
			assert(name == buffer);
		}
	}
	assert(itemCount == 0);
	if (lastFoundId >= 0) {
		for(id = lastFoundId + 1; id < lastFoundId + 200; id++) {
			assert(!myDeskbar.HasItem(id));
			assert(myDeskbar.GetItemInfo(id, &name) == B_NAME_NOT_FOUND);
			assert(name == buffer);
		}
	}
}
	

/*
 *  Method:  PropertyConstructionTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "DeskbarGetItemTest".
 */

 Test *DeskbarGetItemTest::suite(void)
{	
	typedef CppUnit::TestCaller<DeskbarGetItemTest>
		DeskbarGetItemTestCaller;
		
	return(new DeskbarGetItemTestCaller("BDeskbar::Get Item Test", &DeskbarGetItemTest::PerformTest));
	}
