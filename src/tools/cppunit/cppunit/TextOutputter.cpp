#include <cppunit/NotEqualException.h>
#include <cppunit/TestFailure.h>
#include <cppunit/SourceLine.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/TextOutputter.h>


using std::endl;
using std::ostream;

namespace CppUnit
{


TextOutputter::TextOutputter( TestResultCollector *result,
                              ostream &stream )
    : m_result( result )
    , m_stream( stream )
{
}


TextOutputter::~TextOutputter()
{
}


void
TextOutputter::write()
{
  printHeader();
  m_stream << endl;
  printFailures();
  m_stream << endl;
}


void
TextOutputter::printFailures()
{
  TestResultCollector::TestFailures::const_iterator itFailure = m_result->failures().begin();
  int failureNumber = 1;
  while ( itFailure != m_result->failures().end() )
  {
    m_stream  <<  endl;
    printFailure( *itFailure++, failureNumber++ );
  }
}


void
TextOutputter::printFailure( TestFailure *failure,
                             int failureNumber )
{
  printFailureListMark( failureNumber );
  m_stream << ' ';
  printFailureTestName( failure );
  m_stream << ' ';
  printFailureType( failure );
  m_stream << ' ';
  printFailureLocation( failure->sourceLine() );
  m_stream << endl;
  printFailureDetail( failure->thrownException() );
  m_stream << endl;
}


void
TextOutputter::printFailureListMark( int failureNumber )
{
  m_stream << failureNumber << ")";
}


void
TextOutputter::printFailureTestName( TestFailure *failure )
{
  m_stream << "test: " << failure->failedTestName();
}


void
TextOutputter::printFailureType( TestFailure *failure )
{
  m_stream << "("
           << (failure->isError() ? "E" : "F")
           << ")";
}


void
TextOutputter::printFailureLocation( SourceLine sourceLine )
{
  if ( !sourceLine.isValid() )
    return;

  m_stream << "line: " << sourceLine.lineNumber()
           << ' ' << sourceLine.fileName();
}


void
TextOutputter::printFailureDetail( Exception *thrownException )
{
  if ( thrownException->isInstanceOf( NotEqualException::type() ) )
  {
    NotEqualException *e = (NotEqualException*)thrownException;
    m_stream << "expected: " << e->expectedValue() << endl
             << "but was:  " << e->actualValue();
    if ( !e->additionalMessage().empty() )
    {
      m_stream  << endl;
      m_stream  <<  "additional message:"  <<  endl
                <<  e->additionalMessage();
    }
  }
  else
  {
    m_stream << " \"" << thrownException->what() << "\"";
  }
}


void
TextOutputter::printHeader()
{
  if ( m_result->wasSuccessful() )
    m_stream << endl << "OK (" << m_result->runTests () << " tests)"
             << endl;
  else
  {
    m_stream << endl;
    printFailureWarning();
    printStatistics();
  }
}


void
TextOutputter::printFailureWarning()
{
  m_stream  << "!!!FAILURES!!!" << endl;
}


void
TextOutputter::printStatistics()
{
  m_stream  << "Test Results:" << endl;

  m_stream  <<  "Run:  "  <<  m_result->runTests()
            <<  "   Failures: "  <<  m_result->testFailures()
            <<  "   Errors: "  <<  m_result->testErrors()
            <<  endl;
}


} //  namespace CppUnit

