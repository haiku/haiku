/*
	$Id: ConstructionTest1.h,v 1.1 2002/07/09 12:24:58 ejakowatz Exp $
	
	This file defines a class for performing one test of BLocker
	functionality.
	
	*/


#ifndef ConstructionTest1_H
#define ConstructionTest1_H


#include "LockerTestCase.h"
#include "ThreadedTestCaller.h"

	
template<class Locker> class ConstructionTest1 :
	public LockerTestCase<Locker> {
	
private:
	typedef ThreadedTestCaller <ConstructionTest1<Locker> >
		ConstructionTest1Caller;
	bool NameMatches(const char *, Locker *);
	bool IsBenaphore(Locker *);
	
protected:
	
public:
	void PerformTest(void);
	ConstructionTest1(std::string name);
	virtual ~ConstructionTest1();
	static Test *suite(void);
	};
	
#endif