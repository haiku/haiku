#include <cppunit/TestFailure.h>
#include <cppunit/TextTestProgressListener.h>
#include <iostream>


namespace CppUnit
{


TextTestProgressListener::TextTestProgressListener()
{
}


TextTestProgressListener::~TextTestProgressListener()
{
}


void 
TextTestProgressListener::startTest( Test *test )
{
  std::cerr << ".";
  std::cerr.flush();
}


void 
TextTestProgressListener::addFailure( const TestFailure &failure )
{
  std::cerr << ( failure.isError() ? "E" : "F" );
  std::cerr.flush();
}


void 
TextTestProgressListener::done()
{
  std::cerr  <<  std::endl;
  std::cerr.flush();
}

} //  namespace CppUnit

