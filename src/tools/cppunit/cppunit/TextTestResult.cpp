#include <cppunit/Exception.h>
#include <cppunit/NotEqualException.h>
#include <cppunit/Test.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TextTestResult.h>
#include <iostream>


namespace CppUnit {


TextTestResult::TextTestResult()
{
  addListener( this );
}


void 
TextTestResult::addFailure( const TestFailure &failure )
{
  TestResultCollector::addFailure( failure );
  std::cerr << ( failure.isError() ? "E" : "F" );
}


void 
TextTestResult::startTest( Test *test )
{
  TestResultCollector::startTest (test);
  std::cerr << ".";
}


void 
TextTestResult::printFailures( std::ostream &stream )
{
  TestFailures::const_iterator itFailure = failures().begin();
  int failureNumber = 1;
  while ( itFailure != failures().end() ) 
  {
    stream  <<  std::endl;
    printFailure( *itFailure++, failureNumber++, stream );
  }
}


void 
TextTestResult::printFailure( TestFailure *failure,
                              int failureNumber,
                              std::ostream &stream )
{
  printFailureListMark( failureNumber, stream );
  stream << ' ';
  printFailureTestName( failure, stream );
  stream << ' ';
  printFailureType( failure, stream );
  stream << ' ';
  printFailureLocation( failure->sourceLine(), stream );
  stream << std::endl;
  printFailureDetail( failure->thrownException(), stream );
  stream << std::endl;
}


void 
TextTestResult::printFailureListMark( int failureNumber,
                                      std::ostream &stream )
{
  stream << failureNumber << ")";
}


void 
TextTestResult::printFailureTestName( TestFailure *failure,
                                      std::ostream &stream )
{
  stream << "test: " << failure->failedTest()->getName();
}


void 
TextTestResult::printFailureType( TestFailure *failure,
                                  std::ostream &stream )
{
  stream << "("
         << (failure->isError() ? "E" : "F")
         << ")";
}


void 
TextTestResult::printFailureLocation( SourceLine sourceLine,
                                      std::ostream &stream )
{
  if ( !sourceLine.isValid() )
    return;

  stream << "line: " << sourceLine.lineNumber()
         << ' ' << sourceLine.fileName();
}


void 
TextTestResult::printFailureDetail( Exception *thrownException,
                                    std::ostream &stream )
{
  if ( thrownException->isInstanceOf( NotEqualException::type() ) )
  {
    NotEqualException *e = (NotEqualException*)thrownException;
    stream << "expected: " << e->expectedValue() << std::endl
           << "but was:  " << e->actualValue();
    if ( !e->additionalMessage().empty() )
    {
      stream  << std::endl;
      stream  <<  "additional message:"  <<  std::endl
              <<  e->additionalMessage();
    }
  }
  else
  {
    stream << " \"" << thrownException->what() << "\"";
  }
}


void 
TextTestResult::print( std::ostream& stream ) 
{
  printHeader( stream );
  stream << std::endl;
  printFailures( stream );
}


void 
TextTestResult::printHeader( std::ostream &stream )
{
  if (wasSuccessful ())
    stream << std::endl << "OK (" << runTests () << " tests)" 
           << std::endl;
  else
  {
    stream << std::endl;
    printFailureWarning( stream );
    printStatistics( stream );
  }
}


void 
TextTestResult::printFailureWarning( std::ostream &stream )
{
  stream  << "!!!FAILURES!!!" << std::endl;
}


void 
TextTestResult::printStatistics( std::ostream &stream )
{
  stream  << "Test Results:" << std::endl;

  stream  <<  "Run:  "  <<  runTests()
          <<  "   Failures: "  <<  testFailures()
          <<  "   Errors: "  <<  testErrors()
          <<  std::endl;
}


std::ostream &
operator <<( std::ostream &stream, 
             TextTestResult &result )
{ 
  result.print (stream); return stream; 
}


} // namespace CppUnit
