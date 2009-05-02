#include <cppunit/Portability.h>
#include <typeinfo>
#include <stdexcept>

#include "cppunit/TestCase.h"
#include "cppunit/Exception.h"
#include "cppunit/TestResult.h"


using std::exception;
using std::string;

namespace CppUnit {

/// Create a default TestResult
CppUnit::TestResult*
TestCase::defaultResult()
{
  return new TestResult;
}


/// Run the test and catch any exceptions that are triggered by it
void
TestCase::run( TestResult *result )
{
  result->startTest(this);

  try {
	  setUp();

	  try {
	    runTest();
	  }
	  catch ( Exception &e ) {
	    Exception *copy = e.clone();
	    result->addFailure( this, copy );
	  }
	  catch ( exception &e ) {
	    result->addError( this, new Exception( e.what() ) );
	  }
	  catch (...) {
	    Exception *e = new Exception( "caught unknown exception" );
	    result->addError( this, e );
	  }

	  try {
	    tearDown();
	  }
	  catch (...) {
	    result->addError( this, new Exception( "tearDown() failed" ) );
	  }
  }
  catch (...) {
	  result->addError( this, new Exception( "setUp() failed" ) );
  }

  result->endTest( this );
}


/// A default run method
TestResult *
TestCase::run()
{
  TestResult *result = defaultResult();

  run (result);
  return result;
}


/// All the work for runTest is deferred to subclasses
void
TestCase::runTest()
{
}


/** Constructs a test case.
 *  \param name the name of the TestCase.
 **/
TestCase::TestCase( string name )
    : m_name(name)
{
}


/** Constructs a test case for a suite.
 *  This TestCase is intended for use by the TestCaller and should not
 *  be used by a test case for which run() is called.
 **/
TestCase::TestCase()
    : m_name( "" )
{
}


/// Destructs a test case
TestCase::~TestCase()
{
}


/// Returns a count of all the tests executed
int
TestCase::countTestCases() const
{
  return 1;
}


/// Returns the name of the test case
string
TestCase::getName() const
{
  return m_name;
}


/// Returns the name of the test case instance
string
TestCase::toString() const
{
  string className;

#if CPPUNIT_USE_TYPEINFO_NAME
  const std::type_info& thisClass = typeid( *this );
  className = thisClass.name();
#else
  className = "TestCase";
#endif

  return className + "." + getName();
}


} // namespace CppUnit
