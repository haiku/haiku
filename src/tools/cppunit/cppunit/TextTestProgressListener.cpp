#include <cppunit/TestFailure.h>
#include <cppunit/TextTestProgressListener.h>
#include <iostream>


using std::cerr;
using std::endl;

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
  cerr << ".";
  cerr.flush();
}


void
TextTestProgressListener::addFailure( const TestFailure &failure )
{
  cerr << ( failure.isError() ? "E" : "F" );
  cerr.flush();
}


void
TextTestProgressListener::done()
{
  cerr  <<  endl;
  cerr.flush();
}

} //  namespace CppUnit

