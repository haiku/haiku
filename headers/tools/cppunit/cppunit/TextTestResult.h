#ifndef CPPUNIT_TEXTTESTRESULT_H
#define CPPUNIT_TEXTTESTRESULT_H

#include <cppunit/TestResult.h>
#include <cppunit/TestResultCollector.h>
#include <iostream>

namespace CppUnit {

class SourceLine;
class Exception;
class Test;

/*! \brief Holds printable test result (DEPRECATED).
 * \ingroup TrackingTestExecution
 * 
 * deprecated Use class TextTestProgressListener and TextOutputter instead.
 */
class CPPUNIT_API TextTestResult : public TestResult,
                                   public TestResultCollector
{
public:
  TextTestResult();

  virtual void addFailure( const TestFailure &failure );
  virtual void startTest( Test *test );
  virtual void print( std::ostream &stream );
  virtual void printFailures( std::ostream &stream );
  virtual void printHeader( std::ostream &stream );

  virtual void printFailure( TestFailure *failure,
                             int failureNumber,
                             std::ostream &stream );
  virtual void printFailureListMark( int failureNumber,
                                     std::ostream &stream );
  virtual void printFailureTestName( TestFailure *failure,
                                     std::ostream &stream );
  virtual void printFailureType( TestFailure *failure,
                                 std::ostream &stream );
  virtual void printFailureLocation( SourceLine sourceLine,
                                     std::ostream &stream );
  virtual void printFailureDetail( Exception *thrownException,
                                   std::ostream &stream );
  virtual void printFailureWarning( std::ostream &stream );
  virtual void printStatistics( std::ostream &stream );
};

/** insertion operator for easy output */
std::ostream &operator <<( std::ostream &stream, 
                           TextTestResult &result );

} // namespace CppUnit

#endif // CPPUNIT_TEXTTESTRESULT_H


