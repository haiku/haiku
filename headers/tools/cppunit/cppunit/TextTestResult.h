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

  virtual void addFailure( Test *test, Exception *e );
  virtual void addFailure( const TestFailure &failure );
  virtual void startTest( Test *test );
  virtual void print( ostream &stream );
  virtual void printFailures( ostream &stream );
  virtual void printHeader( ostream &stream );

  virtual void printFailure( TestFailure *failure,
                             int failureNumber,
                             ostream &stream );
  virtual void printFailureListMark( int failureNumber,
                                     ostream &stream );
  virtual void printFailureTestName( TestFailure *failure,
                                     ostream &stream );
  virtual void printFailureType( TestFailure *failure,
                                 ostream &stream );
  virtual void printFailureLocation( SourceLine sourceLine,
                                     ostream &stream );
  virtual void printFailureDetail( Exception *thrownException,
                                   ostream &stream );
  virtual void printFailureWarning( ostream &stream );
  virtual void printStatistics( ostream &stream );
};

/** insertion operator for easy output */
ostream &operator <<( ostream &stream, 
                           TextTestResult &result );

} // namespace CppUnit

#endif // CPPUNIT_TEXTTESTRESULT_H


