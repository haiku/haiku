//------------------------------------------------------------------------------
//	LockTargetWithTimeoutTester.h
//
//------------------------------------------------------------------------------

#ifndef LOCK_TARGET_WITH_TIMEOUT_TESTER_H
#define LOCK_TARGET_WITH_TIMEOUT_TESTER_H

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

class LockTargetWithTimeoutTester : public BThreadedTestCase
{
public:
	LockTargetWithTimeoutTester();
	LockTargetWithTimeoutTester(std::string name);
	virtual ~LockTargetWithTimeoutTester();

	void LockTargetWithTimeoutTest1();
	void LockTargetWithTimeoutTest2();
	void LockTargetWithTimeoutTest3();
	void LockTargetWithTimeoutTest4A();
	void LockTargetWithTimeoutTest4B();
	void LockTargetWithTimeoutTest5A();
	void LockTargetWithTimeoutTest5B();
	void LockTargetWithTimeoutTest6A();
	void LockTargetWithTimeoutTest6B();
	void LockTargetWithTimeoutTest7A();
	void LockTargetWithTimeoutTest7B();
	void LockTargetWithTimeoutTest8();
	void LockTargetWithTimeoutTest9();

	static Test* Suite();

private:
	BHandler *fHandler;
	BLooper *fLooper;
};

#endif	// LOCK_TARGET_WITH_TIMEOUT_TESTER_H

