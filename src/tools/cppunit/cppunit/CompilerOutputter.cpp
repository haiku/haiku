#include <algorithm>
#include <cppunit/NotEqualException.h>
#include <cppunit/SourceLine.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/CompilerOutputter.h>


namespace CppUnit
{

CompilerOutputter::CompilerOutputter( TestResultCollector *result,
                                      std::ostream &stream ) :
    m_result( result ),
    m_stream( stream )
{
}


CompilerOutputter::~CompilerOutputter()
{
}


CompilerOutputter *
CompilerOutputter::defaultOutputter( TestResultCollector *result,
                                     std::ostream &stream )
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
            <<  std::endl;
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
  m_stream  <<  std::endl;
  m_stream  <<  "Test name: "  <<  failure->failedTestName();
}


void 
CompilerOutputter::printFailureMessage( TestFailure *failure )
{
  m_stream  <<  std::endl;
  Exception *thrownException = failure->thrownException();
  if ( thrownException->isInstanceOf( NotEqualException::type() ) )
    printNotEqualMessage( thrownException );
  else
    printDefaultMessage( thrownException );
  m_stream  <<  std::endl;
}


void 
CompilerOutputter::printNotEqualMessage( Exception *thrownException )
{
  NotEqualException *e = (NotEqualException *)thrownException;
  m_stream  <<  wrap( "- Expected : " + e->expectedValue() );
  m_stream  <<  std::endl;
  m_stream  <<  wrap( "- Actual   : " + e->actualValue() );
  m_stream  <<  std::endl;
  if ( !e->additionalMessage().empty() )
  {
    m_stream  <<  wrap( e->additionalMessage() );
    m_stream  <<  std::endl;
  }
}


void 
CompilerOutputter::printDefaultMessage( Exception *thrownException )
{
  std::string wrappedMessage = wrap( thrownException->what() );
  m_stream  <<  wrappedMessage  << std::endl;
}


void 
CompilerOutputter::printStatistics()
{
  m_stream  <<  "Failures !!!"  <<  std::endl;
  m_stream  <<  "Run: "  <<  m_result->runTests()  << "   "
            <<  "Failure total: "  <<  m_result->testFailuresTotal()  << "   "
            <<  "Failures: "  <<  m_result->testFailures()  << "   "
            <<  "Errors: "  <<  m_result->testErrors()
            <<  std::endl;
}


std::string
CompilerOutputter::wrap( std::string message )
{
  Lines lines = splitMessageIntoLines( message );
  std::string wrapped;
  for ( Lines::iterator it = lines.begin(); it != lines.end(); ++it )
  {
    std::string line( *it );
    const int maxLineLength = 80;
    int index =0;
    while ( index < line.length() )
    {
      std::string line( line.substr( index, maxLineLength ) );
      wrapped += line;
      index += maxLineLength;
      if ( index < line.length() )
        wrapped += "\n";
    }
    wrapped += '\n';
  }
  return wrapped;
}


CompilerOutputter::Lines 
CompilerOutputter::splitMessageIntoLines( std::string message )
{
  Lines lines;

  std::string::iterator itStart = message.begin();
  while ( true )
  {
    std::string::iterator itEol = std::find( itStart, 
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
