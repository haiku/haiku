/*
	$Id: ConstructionTest1.h 301 2002-07-18 05:32:00Z tylerdauwalder $
	
	This file defines a class for performing one test of BLocker
	functionality.
	
	*/


#ifndef ConstructionTest1_H
#define ConstructionTest1_H


#include "LockerTestCase.h"

	
class ConstructionTest1 :
	public LockerTestCase {
	
private:
	bool NameMatches(const char *, BLocker *);
	bool IsBenaphore(BLocker *);
	
protected:
	
public:
	void PerformTest(void);
	ConstructionTest1(std::string name = "");
	virtual ~ConstructionTest1();
	static CppUnit::Test *suite(void);
	};
	
#endif



