#include <cppunit/extensions/RepeatedTest.h>
#include <cppunit/TestResult.h>

namespace CppUnit {



// Counts the number of test cases that will be run by this test.
int
RepeatedTest::countTestCases() const
{ 
  return TestDecorator::countTestCases () * m_timesRepeat; 
}


// Returns the name of the test instance. 
std::string 
RepeatedTest::toString() const
{ 
  return TestDecorator::toString () + " (repeated)"; 
}

// Runs a repeated test
void 
RepeatedTest::run( TestResult *result )
{
  for ( int n = 0; n < m_timesRepeat; n++ ) 
  {
    if ( result->shouldStop() )
        break;

    TestDecorator::run( result );
  }
}


} // namespace TestAssert
