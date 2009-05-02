#include <cppunit/Exception.h>
#include <cppunit/NotEqualException.h>
#include <cppunit/Test.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TextTestResult.h>
#include <iostream>


using std::cerr;
using std::endl;
using std::ostream;

namespace CppUnit {


TextTestResult::TextTestResult()
{
  addListener( this );
}


void
TextTestResult::addFailure( Test *test, Exception *e )
{
	TestResult::addFailure( test, e );
}


void
TextTestResult::addFailure( const TestFailure &failure )
{
  TestResultCollector::addFailure( failure );
  cerr << ( failure.isError() ? "E" : "F" );
}


void
TextTestResult::startTest( Test *test )
{
  TestResultCollector::startTest (test);
  cerr << ".";
}


void
TextTestResult::printFailures( ostream &stream )
{
  TestFailures::const_iterator itFailure = failures().begin();
  int failureNumber = 1;
  while ( itFailure != failures().end() )
  {
    stream  <<  endl;
    printFailure( *itFailure++, failureNumber++, stream );
  }
}


void
TextTestResult::printFailure( TestFailure *failure,
                              int failureNumber,
                              ostream &stream )
{
  printFailureListMark( failureNumber, stream );
  stream << ' ';
  printFailureTestName( failure, stream );
  stream << ' ';
  printFailureType( failure, stream );
  stream << ' ';
  printFailureLocation( failure->sourceLine(), stream );
  stream << endl;
  printFailureDetail( failure->thrownException(), stream );
  stream << endl;
}


void
TextTestResult::printFailureListMark( int failureNumber,
                                      ostream &stream )
{
  stream << failureNumber << ")";
}


void
TextTestResult::printFailureTestName( TestFailure *failure,
                                      ostream &stream )
{
  stream << "test: " << failure->failedTest()->getName();
}


void
TextTestResult::printFailureType( TestFailure *failure,
                                  ostream &stream )
{
  stream << "("
         << (failure->isError() ? "E" : "F")
         << ")";
}


void
TextTestResult::printFailureLocation( SourceLine sourceLine,
                                      ostream &stream )
{
  if ( !sourceLine.isValid() )
    return;

  stream << "line: " << sourceLine.lineNumber()
         << ' ' << sourceLine.fileName();
}


void
TextTestResult::printFailureDetail( Exception *thrownException,
                                    ostream &stream )
{
  if ( thrownException->isInstanceOf( NotEqualException::type() ) )
  {
    NotEqualException *e = (NotEqualException*)thrownException;
    stream << "expected: " << e->expectedValue() << endl
           << "but was:  " << e->actualValue();
    if ( !e->additionalMessage().empty() )
    {
      stream  << endl;
      stream  <<  "additional message:"  <<  endl
              <<  e->additionalMessage();
    }
  }
  else
  {
    stream << " \"" << thrownException->what() << "\"";
  }
}


void
TextTestResult::print( ostream& stream )
{
  printHeader( stream );
  stream << endl;
  printFailures( stream );
}


void
TextTestResult::printHeader( ostream &stream )
{
  if (wasSuccessful ())
    stream << endl << "OK (" << runTests () << " tests)"
           << endl;
  else
  {
    stream << endl;
    printFailureWarning( stream );
    printStatistics( stream );
  }
}


void
TextTestResult::printFailureWarning( ostream &stream )
{
  stream  << "!!!FAILURES!!!" << endl;
}


void
TextTestResult::printStatistics( ostream &stream )
{
  stream  << "Test Results:" << endl;

  stream  <<  "Run:  "  <<  runTests()
          <<  "   Failures: "  <<  testFailures()
          <<  "   Errors: "  <<  testErrors()
          <<  endl;
}


ostream &
operator <<( ostream &stream,
             TextTestResult &result )
{
  result.print (stream); return stream;
}


} // namespace CppUnit
