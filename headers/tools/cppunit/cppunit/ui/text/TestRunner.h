#ifndef CPPUNITUI_TEXT_TESTRUNNER_H
#define CPPUNITUI_TEXT_TESTRUNNER_H

#include <cppunit/Portability.h>
#include <string>
#include <vector>

namespace CppUnit {

class Outputter;
class Test;
class TestSuite;
class TextOutputter;
class TestResult;
class TestResultCollector;

namespace TextUi
{

/*!
 * \brief A text mode test runner.
 * \ingroup WritingTestResult
 * \ingroup ExecutingTest
 *
 * The test runner manage the life cycle of the added tests.
 *
 * The test runner can run only one of the added tests or all the tests. 
 *
 * TestRunner prints out a trace as the tests are executed followed by a
 * summary at the end. The trace and summary print are optional.
 *
 * Here is an example of use:
 *
 * \code
 * CppUnit::TextUi::TestRunner runner;
 * runner.addTest( ExampleTestCase::suite() );
 * runner.run( "", true );    // Run all tests and wait
 * \endcode
 *
 * The trace is printed using a TextTestProgressListener. The summary is printed
 * using a TextOutputter. 
 *
 * You can specify an alternate Outputter at construction
 * or later with setOutputter(). 
 *
 * After construction, you can register additional TestListener to eventManager(),
 * for a custom progress trace, for example.
 *
 * \code
 * CppUnit::TextUi::TestRunner runner;
 * runner.addTest( ExampleTestCase::suite() );
 * runner.setOutputter( CppUnit::CompilerOutputter::defaultOutputter( 
 *                          &runner.result(),
 *                          std::cerr ) );
 * MyCustomProgressTestListener progress;
 * runner.eventManager().addListener( &progress );
 * runner.run( "", true );    // Run all tests and wait
 * \endcode
 *
 * \see CompilerOutputter, XmlOutputter, TextOutputter.
 */
class CPPUNIT_API TestRunner
{
public:
  TestRunner( Outputter *outputter =NULL );

  virtual ~TestRunner();

  bool run( std::string testName ="",
            bool doWait = false,
            bool doPrintResult = true,
            bool doPrintProgress = true );

  void addTest( Test *test );

  void setOutputter( Outputter *outputter );

  TestResultCollector &result() const;

  TestResult &eventManager() const;

protected:
  virtual bool runTest( Test *test,
                        bool doPrintProgress );
  virtual bool runTestByName( std::string testName,
                              bool printProgress );
  virtual void wait( bool doWait );
  virtual void printResult( bool doPrintResult );

  virtual Test *findTestByName( std::string name ) const;

  TestSuite *m_suite;
  TestResultCollector *m_result;
  TestResult *m_eventManager;
  Outputter *m_outputter;
};


} // namespace TextUi

} // namespace CppUnit

#endif  // CPPUNITUI_TEXT_TESTRUNNER_H
