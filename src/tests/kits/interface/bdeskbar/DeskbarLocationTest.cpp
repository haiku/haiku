/*
	$Id: DeskbarLocationTest.cpp 1236 2002-09-28 07:27:00Z shatty $
	
	This file implements tests for the following use cases of BDeskbar:
	  - Frame
	  - Location
	  - Is Expanded
	  - Set Location
	  - Expand
	
	*/


#include "DeskbarLocationTest.h"
#include <Deskbar.h>
#include <InterfaceDefs.h>


/*
 *  Method:  DeskbarLocationTest::DeskbarLocationTest()
 *   Descr:  This is the constructor for this class.
 */
		

	DeskbarLocationTest::DeskbarLocationTest(std::string name) :
		TestCase(name)
{
	}


/*
 *  Method:  DeskbarLocationTest::~DeskbarLocationTest()
 *   Descr:  This is the destructor for this class.
 */
 

	DeskbarLocationTest::~DeskbarLocationTest()
{
	}
	
	
/*
 *  Method:  DeskbarLocationTest::CheckLocation()
 *   Descr:  This member fuction checks that the passed in location and
 *           expansion state is the actual deskbar state
 */
 

	void DeskbarLocationTest::CheckLocation(BDeskbar *myDeskbar,
	                                        deskbar_location currLocation,
	                                        bool currExpanded,
	                                        bool expandedSetDirectly)
{
	bool isExpanded;
	assert(myDeskbar->Location() == currLocation);
	assert(myDeskbar->Location(&isExpanded) == currLocation);
	
	if ((currLocation == B_DESKBAR_LEFT_TOP) ||
	    (currLocation == B_DESKBAR_RIGHT_TOP) ||
	    (expandedSetDirectly)) {
		assert(myDeskbar->IsExpanded() == currExpanded);
		assert(isExpanded == currExpanded);
	} else if ((currLocation == B_DESKBAR_LEFT_BOTTOM) ||
	           (currLocation == B_DESKBAR_RIGHT_BOTTOM)) {
		assert(!myDeskbar->IsExpanded());
		assert(!isExpanded);
	} else {
		assert(myDeskbar->IsExpanded());
		assert(isExpanded);
	}
	
	BRect myRect(myDeskbar->Frame());
	switch (currLocation) {
		case B_DESKBAR_TOP:
		case B_DESKBAR_LEFT_TOP:
			assert(myRect.top == 0.0);
			assert(myRect.left == 0.0);
			break;
		case B_DESKBAR_BOTTOM:
		case B_DESKBAR_LEFT_BOTTOM:
			assert(myRect.left == 0.0);
			break;
		case B_DESKBAR_RIGHT_TOP:
		case B_DESKBAR_RIGHT_BOTTOM:
			break;
		default:
			assert(false);
			break;
	}
	assert(get_deskbar_frame(&myRect) == B_OK);
	switch (currLocation) {
		case B_DESKBAR_TOP:
		case B_DESKBAR_LEFT_TOP:
			assert(myRect.top == 0.0);
			assert(myRect.left == 0.0);
			break;
		case B_DESKBAR_BOTTOM:
		case B_DESKBAR_LEFT_BOTTOM:
			assert(myRect.left == 0.0);
			break;
		case B_DESKBAR_RIGHT_TOP:
		case B_DESKBAR_RIGHT_BOTTOM:
			break;
		default:
			assert(false);
			break;
	}	
}	
	
/*
 *  Method:  DeskbarLocationTest::TestLocation()
 *   Descr:  This member fuction performs a test of one location of the
 *           deskbar.
 */
 

	void DeskbarLocationTest::TestLocation(BDeskbar *myDeskbar,
	                                       deskbar_location newLocation)
{
	assert(myDeskbar->SetLocation(newLocation) == B_OK);
	CheckLocation(myDeskbar, newLocation, false, false);
	
	assert(myDeskbar->SetLocation(B_DESKBAR_TOP) == B_OK);
	CheckLocation(myDeskbar, B_DESKBAR_TOP, false, false);
	
	assert(myDeskbar->SetLocation(newLocation, false) == B_OK);
	CheckLocation(myDeskbar, newLocation, false, false);
	
	assert(myDeskbar->SetLocation(B_DESKBAR_TOP) == B_OK);
	CheckLocation(myDeskbar, B_DESKBAR_TOP, false, false);
	
	assert(myDeskbar->SetLocation(newLocation, true) == B_OK);
	CheckLocation(myDeskbar, newLocation, true, false);
	
	assert(myDeskbar->Expand(false) == B_OK);
	CheckLocation(myDeskbar, newLocation, false, true);
	
	assert(myDeskbar->Expand(true) == B_OK);
	CheckLocation(myDeskbar, newLocation, true, true);
	}


/*
 *  Method:  DeskbarLocationTest::PerformTest()
 *   Descr:  This member function saves the starting state of the deskbar
 *           and then sets the deskbar location to each individual allowed
 *           state.  Finally, it restores the original location at the end
 *           of the test.
 */


	void DeskbarLocationTest::PerformTest(void)
{
	BDeskbar myDeskbar;
	bool origExpanded;
	deskbar_location origLocation = myDeskbar.Location(&origExpanded);
	
	assert((origLocation == B_DESKBAR_TOP) ||
	       (origLocation == B_DESKBAR_BOTTOM) ||
	       (origLocation == B_DESKBAR_LEFT_BOTTOM) ||
	       (origLocation == B_DESKBAR_RIGHT_BOTTOM) ||
	       (origLocation == B_DESKBAR_LEFT_TOP) ||
	       (origLocation == B_DESKBAR_RIGHT_TOP));
	       
	TestLocation(&myDeskbar, B_DESKBAR_TOP);
	TestLocation(&myDeskbar, B_DESKBAR_BOTTOM);
	TestLocation(&myDeskbar, B_DESKBAR_LEFT_BOTTOM);
	TestLocation(&myDeskbar, B_DESKBAR_RIGHT_BOTTOM);
	TestLocation(&myDeskbar, B_DESKBAR_LEFT_TOP);
	TestLocation(&myDeskbar, B_DESKBAR_RIGHT_TOP);
	
	assert(myDeskbar.SetLocation(origLocation, origExpanded) == B_OK);
	assert(myDeskbar.Location() == origLocation);
	assert(myDeskbar.IsExpanded() == origExpanded);
	}
	

/*
 *  Method:  PropertyConstructionTest::suite()
 *   Descr:  This static member function returns a test caller for performing 
 *           all combinations of "DeskbarLocationTest".
 */

 Test *DeskbarLocationTest::suite(void)
{	
	typedef CppUnit::TestCaller<DeskbarLocationTest>
		DeskbarLocationTestCaller;
		
	return(new DeskbarLocationTestCaller("BDeskbar::Location Test", &DeskbarLocationTest::PerformTest));
	}
