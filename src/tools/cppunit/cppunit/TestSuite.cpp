#include "cppunit/TestSuite.h"
#include "cppunit/TestResult.h"

namespace CppUnit {

/// Default constructor
TestSuite::TestSuite( string name )
    : m_name( name )
{
}


/// Destructor
TestSuite::~TestSuite()
{ 
  deleteContents(); 
}


/// Deletes all tests in the suite.
void 
TestSuite::deleteContents()
{
  for ( vector<Test *>::iterator it = m_tests.begin();
        it != m_tests.end();
        ++it)
    delete *it;
  m_tests.clear();
}


/// Runs the tests and collects their result in a TestResult.
void 
TestSuite::run( TestResult *result )
{
  for ( vector<Test *>::iterator it = m_tests.begin();
        it != m_tests.end();
        ++it )
  {
    if ( result->shouldStop() )
        break;

    Test *test = *it;
    test->run( result );
  }
}


/// Counts the number of test cases that will be run by this test.
int 
TestSuite::countTestCases() const
{
  int count = 0;

  for ( vector<Test *>::const_iterator it = m_tests.begin();
        it != m_tests.end();
        ++it )
    count += (*it)->countTestCases();

  return count;
}


/// Adds a test to the suite. 
void 
TestSuite::addTest( Test *test )
{ 
  m_tests.push_back( test ); 
}


/// Returns a string representation of the test suite.
string 
TestSuite::toString() const
{ 
  return "suite " + getName();
}


/// Returns the name of the test suite.
string 
TestSuite::getName() const
{ 
  return m_name; 
}


const vector<Test *> &
TestSuite::getTests() const
{
  return m_tests;
}


} // namespace CppUnit

