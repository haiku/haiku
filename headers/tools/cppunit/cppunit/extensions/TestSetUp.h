#ifndef CPPUNIT_EXTENSIONS_TESTSETUP_H
#define CPPUNIT_EXTENSIONS_TESTSETUP_H

#include <cppunit/extensions/TestDecorator.h>

namespace CppUnit {

class Test;
class TestResult;


class CPPUNIT_API TestSetUp : public TestDecorator 
{
public:
  TestSetUp( Test *test );

  void run( TestResult *result );

protected:
  virtual void setUp();
  virtual void tearDown();

private:
  TestSetUp( const TestSetUp & );
  void operator =( const TestSetUp & );
};


} //  namespace CppUnit

#endif // CPPUNIT_EXTENSIONS_TESTSETUP_H

