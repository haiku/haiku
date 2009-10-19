/*
	$Id: DeskbarAddItemTest.cpp 1236 2002-09-28 07:27:00Z shatty $
	
	This file implements tests for the following use cases of BDeskbar:
	  - Add Item 1
	  - Add Item 2
	  - Remove Item 1
	  - Remove Item 2
	
	*/


#include "DeskbarAddItemTest.h"
#include <Deskbar.h>
#include <View.h>
#include <Application.h>
#include <Entry.h>
#include <image.h>


const char *appName = "application/x-vnd.jsr-additemtest";
const char *pulsePath = "/boot/apps/Pulse";


/*
 *  Method:  DeskbarAddItemTest::DeskbarAddItemTest()
 *   Descr:  This is the constructor for this class.
 */
		

	DeskbarAddItemTest::DeskbarAddItemTest(std::string name) :
		TestCase(name)
{
	}


/*
 *  Method:  DeskbarAddItemTest::~DeskbarAddItemTest()
 *   Descr:  This is the destructor for this class.
 */
 

	DeskbarAddItemTest::~DeskbarAddItemTest()
{
	}


/*
 *  Method:  DeskbarAddItemTest::PerformTest()
 *   Descr:  This member function tests the ability of the BDeskbar class
 *           when adding and removing an item from the shelf.  It does so by
 *           adding and removing the pulse item.  It is a good candidate
 *           it can be added both ways (Add Item 1 and Add Item 2).
 *
 *           The code does the following:
 *             - loads the code for Pulse as an add-on
 *             - gets the "instantiate_deskbar_item()" function from the add-on
 *             - calls this function to get a pointer to a BView which can be
 *               used to test Add Item 1 and in order to get the name of the
 *               item so it can be removed etc.
 *             - it gets an entry_ref to the Pulse app in order to test Add
 *               Item 2
 *             - it stores in a boolean whether Pulse is in the deskbar at the
 *               start of the test in order to restore the users config when
 *               the test completes
 *             - it adds the item each different way and tests that it exists
 *             - between each, it removes the item and shows that it has
 *               been removed
 */


	void DeskbarAddItemTest::PerformTest(void)
{
	BApplication theApp(appName);
	
	BView *(*funcPtr)(void);
	image_id theImage;
	assert((theImage = load_add_on(pulsePath)) != B_ERROR);
	assert(get_image_symbol(theImage, "instantiate_deskbar_item",
	                        B_SYMBOL_TYPE_TEXT, (void **)&funcPtr) == B_OK);
	BView *theView = funcPtr();
	assert(theView != NULL);
	
	BEntry entry(pulsePath);
	entry_ref ref;
	assert(entry.GetRef(&ref) == B_OK);
	
	int32 theId1, theId2;
	BDeskbar myDeskbar;
	bool restorePulse = myDeskbar.HasItem(theView->Name());
	
	assert(myDeskbar.RemoveItem(theView->Name()) == B_OK);
	assert(!myDeskbar.HasItem(theView->Name()));
	
	assert(myDeskbar.AddItem(theView, &theId1) == B_OK);
	assert(myDeskbar.HasItem(theView->Name()));
	assert(myDeskbar.GetItemInfo(theView->Name(), &theId2) == B_OK);
	assert(theId1 == theId2);
	
	assert(myDeskbar.RemoveItem(theView->Name()) == B_OK);
	assert(!myDeskbar.HasItem(theView->Name()));
	
	assert(myDeskbar.AddItem(theView) == B_OK);
	assert(myDeskbar.HasItem(theView->Name()));
	
	assert(myDeskbar.RemoveItem(theView->Name()) == B_OK);
	assert(!myDeskbar.HasItem(theView->Name()));
	
	assert(myDeskbar.AddItem(&ref, &theId1) == B_OK);
	assert(myDeskbar.HasItem(theView->Name()));
	assert(myDeskbar.GetItemInfo(theView->Name(), &theId2) == B_OK);
	assert(theId1 == theId2);
	
	assert(myDeskbar.RemoveItem(theView->Name()) == B_OK);
	assert(!myDeskbar.HasItem(theView->Name()));
	
	assert(myDeskbar.AddItem(&ref) == B_OK);
	assert(myDeskbar.HasItem(theView->Name()));
	
	if (!restorePulse) {
		assert(myDeskbar.RemoveItem(theView->Name()) == B_OK);
		assert(!myDeskbar.HasItem(theView->Name()));
	}
}
	

/*
 *  Method:  PropertyConstructionTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "DeskbarAddItemTest".
 */

 Test *DeskbarAddItemTest::suite(void)
{	
	typedef CppUnit::TestCaller<DeskbarAddItemTest>
		DeskbarAddItemTestCaller;
		
	return(new DeskbarAddItemTestCaller("BDeskbar::Add Item Test", &DeskbarAddItemTest::PerformTest));
	}
