/*
	$Id: ConstructionTest1.cpp,v 1.2 2002/07/18 05:32:00 tylerdauwalder Exp $
	
	This file implements a test class for testing BLocker construction
	functionality.  It checks the "Construction 1", "Construction 2" and
	"Sem" use cases.  It does so by testing all the documented constructors
	and uses the Sem() member function to confirm that the name and style
	were set correctly.
	
	*/


#include "ConstructionTest1.h"
#include <cppunit/TestCaller.h>
#include <OS.h>
#include <Locker.h>


/*
 *  Method:  ConstructionTest1::ConstructionTest1()
 *   Descr:  This is the constructor for this class.
 */
		

	ConstructionTest1::ConstructionTest1(std::string name) :
		LockerTestCase(name, true)
{
	}


/*
 *  Method:  ConstructionTest1::~ConstructionTest1()
 *   Descr:  This is the desctructor for this BLocker test class.
 */


	ConstructionTest1::~ConstructionTest1()
{
	}


/*
 *  Method:  ConstructionTest1::NameMatches()
 *   Descr:  This member checks that the semaphore owned by the lock
 *           passed in has the name passed in.
 */
	
bool
	ConstructionTest1::NameMatches(const char *name,
									       BLocker *lockerArg)
{
	sem_info theSemInfo;
	
	assert(get_sem_info(lockerArg->Sem(), &theSemInfo) == B_OK);
	return(strcmp(name, theSemInfo.name) == 0);
}


/*
 *  Method:  ConstructionTest1::IsBenaphore()
 *   Descr:  This member attempts to confirm that the BLocker passed in
 *           is a benaphore or a semaphore style locker.  It returns true
 *           if it is a benaphore, false if it is a semaphore.  An
 *           assertion is raised if an error occurs.
 */
	
bool
	ConstructionTest1::IsBenaphore(BLocker *lockerArg)
{
	int32 semCount;
	
	assert(get_sem_count(lockerArg->Sem(), &semCount) == B_OK);
	switch (semCount) {
		case 0: return(true);
				break;
		case 1: return(false);
				break;
		default:
				// This should not happen.  The semaphore count should be
				// 0 for a benaphore, 1 for a semaphore.  No other value
				// is legal in this case.
				assert(false);
				break;
		}
	return(false);
	}


/*
 *  Method:  ConstructionTest1::PerformTest()
 *   Descr:  This member function is used to test each of the constructors
 *           for the BLocker.  The resulting BLocker is tested to show
 *           that the BLocker was constructed correctly.
 */

void ConstructionTest1::PerformTest(void)
{
	NextSubTest();
	assert(NameMatches("some BLocker", theLocker));
	assert(IsBenaphore(theLocker));
	
	NextSubTest();
	BLocker locker1("test string");
	assert(NameMatches("test string", &locker1));
	assert(IsBenaphore(&locker1));
	
	NextSubTest();
	BLocker locker2(false);
	assert(NameMatches("some BLocker", &locker2));
	assert(!IsBenaphore(&locker2));
	
	NextSubTest();
	BLocker locker3(true);
	assert(NameMatches("some BLocker", &locker3));
	assert(IsBenaphore(&locker3));
	
	NextSubTest();
	BLocker locker4("test string", false);
	assert(NameMatches("test string", &locker4));
	assert(!IsBenaphore(&locker4));
	
	NextSubTest();
	BLocker locker5("test string", true);
	assert(NameMatches("test string", &locker5));
	assert(IsBenaphore(&locker5));
}


/*
 *  Method:  ConstructionTest1::suite()
 *   Descr:  This static member function returns a threaded test caller for
 *           performing "ConstructionTest1".  The threaded test caller
 *           only has a single thread pointint to the PerformTest() member
 *           function of this class.
 */

CppUnit::Test *ConstructionTest1::suite(void)
{
	typedef CppUnit::TestCaller <ConstructionTest1 >
		ConstructionTest1Caller;
		
	return new ConstructionTest1Caller("BLocker::Construction Test", &ConstructionTest1::PerformTest);
}



