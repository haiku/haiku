//------------------------------------------------------------------------------

#ifndef COMMON_H
#define COMMON_H

// Standard Includes -----------------------------------------------------------
#include <posix/string.h>
#include <errno.h>

// System Includes -------------------------------------------------------------

// Project Includes ------------------------------------------------------------
#include "cppunit/TestCaller.h"
#include "TestCase.h"
//#include "TestResult.h"
#include "cppunit/TestSuite.h"

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
#define assert_err(condition)	\
    (this->assertImplementation ((condition), std::string((#condition)) +	\
    	strerror(condition),\
        __LINE__, __FILE__))

#define ADD_TEST(suitename, classname, funcname)				\
	(suitename)->addTest(new CppUnit::TestCaller<classname>(std::string("BArchivable::") + \
				std::string((#funcname)), &classname::funcname));

#define ADD_TEST4(classbeingtested, suitename, classname, funcname)				\
	(suitename)->addTest(new TestCaller<classname>((#classbeingtested "::" #funcname),	\
						 &classname::funcname));
#define CHECK_ERRNO														\
	cout << endl << "errno == \"" << strerror(errno) << "\" (" << errno	\
		 << ") in " << __PRETTY_FUNCTION__ << endl

#define CHECK_STATUS(status__)											\
	cout << endl << "status_t == \"" << strerror((status__)) << "\" ("	\
		 << (status__) << ") in " << __PRETTY_FUNCTION__ << endl

// Globals ---------------------------------------------------------------------


#endif	//COMMON_H

/*
 * $Log $
 *
 * $Id  $
 *
 */

