//------------------------------------------------------------------------------
//	LockTargetTester.h
//
//------------------------------------------------------------------------------

#ifndef LOCK_TARGET_TESTER_H
#define LOCK_TARGET_TESTER_H

// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include <ThreadedTestCase.h>

// Local Includes --------------------------------------------------------------
#include "../common.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

class BHandler;
class BLooper;

class LockTargetTester : public BThreadedTestCase
{
public:
	LockTargetTester();
	LockTargetTester(std::string name);
	virtual ~LockTargetTester();

	void LockTargetTest1();
	void LockTargetTest2();
	void LockTargetTest3();
	void LockTargetTest4A();
	void LockTargetTest4B();
	void LockTargetTest5A();
	void LockTargetTest5B();
	void LockTargetTest6();
	void LockTargetTest7();

	static Test* Suite();

private:
	BHandler *fHandler;
	BLooper *fLooper;
};

#endif	// LOCK_TARGET_TESTER_H

