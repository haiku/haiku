#include <algorithm>
#include <cppunit/NotEqualException.h>
#include <cppunit/SourceLine.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/CompilerOutputter.h>


using std::endl;
using std::ostream;
using std::string;

namespace CppUnit
{

CompilerOutputter::CompilerOutputter( TestResultCollector *result,
                                      ostream &stream ) :
    m_result( result ),
    m_stream( stream )
{
}


CompilerOutputter::~CompilerOutputter()
{
}


CompilerOutputter *
CompilerOutputter::defaultOutputter( TestResultCollector *result,
                                     ostream &stream )
{
  return new CompilerOutputter( result, stream );
// For automatic adpatation...
//  return new CPPUNIT_DEFAULT_OUTPUTTER( result, stream );
}


void
CompilerOutputter::write()
{
  if ( m_result->wasSuccessful() )
    printSucess();
  else
    printFailureReport();
}


void
CompilerOutputter::printSucess()
{
  m_stream  << "OK (" << m_result->runTests()  << ")"
            <<  endl;
}


void
CompilerOutputter::printFailureReport()
{
  printFailuresList();
  printStatistics();
}


void
CompilerOutputter::printFailuresList()
{
  for ( int index =0; index < m_result->testFailuresTotal(); ++index)
  {
    printFailureDetail( m_result->failures()[ index ] );
  }
}


void
CompilerOutputter::printFailureDetail( TestFailure *failure )
{
  printFailureLocation( failure->sourceLine() );
  printFailureType( failure );
  printFailedTestName( failure );
  printFailureMessage( failure );
}


void
CompilerOutputter::printFailureLocation( SourceLine sourceLine )
{
  if ( sourceLine.isValid() )
    m_stream  <<  sourceLine.fileName()
              <<  "("  << sourceLine.lineNumber()  << ") : ";
  else
    m_stream  <<  "##Failure Location unknown## : ";
}


void
CompilerOutputter::printFailureType( TestFailure *failure )
{
  m_stream  <<  (failure->isError() ? "Error" : "Assertion");
}


void
CompilerOutputter::printFailedTestName( TestFailure *failure )
{
  m_stream  <<  endl;
  m_stream  <<  "Test name: "  <<  failure->failedTestName();
}


void
CompilerOutputter::printFailureMessage( TestFailure *failure )
{
  m_stream  <<  endl;
  Exception *thrownException = failure->thrownException();
  if ( thrownException->isInstanceOf( NotEqualException::type() ) )
    printNotEqualMessage( thrownException );
  else
    printDefaultMessage( thrownException );
  m_stream  <<  endl;
}


void
CompilerOutputter::printNotEqualMessage( Exception *thrownException )
{
  NotEqualException *e = (NotEqualException *)thrownException;
  m_stream  <<  wrap( "- Expected : " + e->expectedValue() );
  m_stream  <<  endl;
  m_stream  <<  wrap( "- Actual   : " + e->actualValue() );
  m_stream  <<  endl;
  if ( !e->additionalMessage().empty() )
  {
    m_stream  <<  wrap( e->additionalMessage() );
    m_stream  <<  endl;
  }
}


void
CompilerOutputter::printDefaultMessage( Exception *thrownException )
{
  string wrappedMessage = wrap( thrownException->what() );
  m_stream  <<  wrappedMessage  << endl;
}


void
CompilerOutputter::printStatistics()
{
  m_stream  <<  "Failures !!!"  <<  endl;
  m_stream  <<  "Run: "  <<  m_result->runTests()  << "   "
            <<  "Failure total: "  <<  m_result->testFailuresTotal()  << "   "
            <<  "Failures: "  <<  m_result->testFailures()  << "   "
            <<  "Errors: "  <<  m_result->testErrors()
            <<  endl;
}


string
CompilerOutputter::wrap( string message )
{
  Lines lines = splitMessageIntoLines( message );
  string wrapped;
  for ( Lines::iterator it = lines.begin(); it != lines.end(); ++it )
  {
    string line( *it );
    const int maxLineLength = 80;
    int index =0;
    while ( index < (int)line.length() )
    {
      string line( line.substr( index, maxLineLength ) );
      wrapped += line;
      index += maxLineLength;
      if ( index < (int)line.length() )
        wrapped += "\n";
    }
    wrapped += '\n';
  }
  return wrapped;
}


CompilerOutputter::Lines
CompilerOutputter::splitMessageIntoLines( string message )
{
  Lines lines;

  string::iterator itStart = message.begin();
  while ( true )
  {
    string::iterator itEol = find( itStart,
                                             message.end(),
                                             '\n' );
    lines.push_back( message.substr( itStart - message.begin(),
                                     itEol - itStart ) );
    if ( itEol == message.end() )
      break;
    itStart = itEol +1;
  }
  return lines;
}



}  // namespace CppUnit
