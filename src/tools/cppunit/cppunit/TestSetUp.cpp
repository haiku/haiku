#include <cppunit/extensions/TestSetUp.h>

namespace CppUnit {

TestSetUp::TestSetUp( Test *test ) : TestDecorator( test ) 
{
}


void 
TestSetUp::setUp()
{
}


void 
TestSetUp::tearDown() 
{
}


void
TestSetUp::run( TestResult *result )
{ 
  setUp();
  TestDecorator::run(result);
  tearDown();
}


} //  namespace CppUnit
