//------------------------------------------------------------------------------
//	common.h
//
//------------------------------------------------------------------------------

#ifndef COMMON_H
#define COMMON_H

// Standard Includes -----------------------------------------------------------
#include <posix/string.h>
#include <errno.h>

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include "cppunit/TestCaller.h"
#include "cppunit/TestCase.h"
#include "cppunit/TestResult.h"
#include "cppunit/TestSuite.h"
#include "cppunit/TextTestResult.h"

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
#define assert_err(condition)	\
    (this->assertImplementation ((condition), std::string((#condition)) +	\
    	strerror(condition),\
        __LINE__, __FILE__))

#define ADD_TEST4(classbeingtested, suitename, classname, funcname)				\
	(suitename)->addTest(new TestCaller<classname>((#classbeingtested "::" #funcname),	\
						 &classname::funcname));

#define ADD_TEST ADD_TEST4

#define CHECK_ERRNO														\
	cout << endl << "errno == \"" << strerror(errno) << "\" (" << errno	\
		 << ") in " << __PRETTY_FUNCTION__ << endl

#define CHECK_STATUS(status__)											\
	cout << endl << "status_t == \"" << strerror((status__)) << "\" ("	\
		 << (status__) << ") in " << __PRETTY_FUNCTION__ << endl

#ifdef USE_DEBUGGER_TESTS
#define DEBUGGER_ESCAPE
#else
#define DEBUGGER_ESCAPE	return
#endif

// Globals ---------------------------------------------------------------------

using namespace CppUnit;

#endif	//COMMON_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

