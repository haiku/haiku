#ifndef CPPUNIT_COMPILERTESTRESULTOUTPUTTER_H
#define CPPUNIT_COMPILERTESTRESULTOUTPUTTER_H

#include <cppunit/Portability.h>
#include <cppunit/Outputter.h>
#include <vector>
#include <iostream>

namespace CppUnit
{

class Exception;
class SourceLine;
class Test;
class TestFailure;
class TestResultCollector;

/*! 
 * \brief Outputs a TestResultCollector in a compiler compatible format.
 * \ingroup WritingTestResult
 *
 * Printing the test results in a compiler compatible format (assertion
 * location has the same format as compiler error), allow you to use your
 * IDE to jump to the assertion failure.
 *
 * For example, when running the test in a post-build with VC++, if an assertion
 * fails, you can jump to the assertion by pressing F4 (jump to next error).
 *
 * You should use defaultOutputter() to create an instance.
 *
 * Heres is an example of usage (from examples/cppunittest/CppUnitTestMain.cpp):
 * \code
 * int main( int argc, char* argv[] ) {
 *   // if command line contains "-selftest" then this is the post build check
 *   // => the output must be in the compiler error format.
 *   bool selfTest = (argc > 1)  &&  
 *                   (std::string("-selftest") == argv[1]);
 *
 *   CppUnit::TextUi::TestRunner runner;
 *   runner.addTest( CppUnitTest::suite() );   // Add the top suite to the test runner
 * 
 *  if ( selfTest )
 *   { // Change the default outputter to a compiler error format outputter
 *     // The test runner owns the new outputter.
 *     runner.setOutputter( CppUnit::CompilerOutputter::defaultOutputter( 
 *                                                        &runner.result(),
 *                                                         std::cerr ) );
 *   }
 * 
 *  // Run the test and don't wait a key if post build check.
 *   bool wasSucessful = runner.run( "", !selfTest );
 * 
 *   // Return error code 1 if the one of test failed.
 *   return wasSucessful ? 0 : 1;
 * }
 * \endcode
 */
class CPPUNIT_API CompilerOutputter : public Outputter
{
public:
  /*! Constructs a CompilerOutputter object.
   */
  CompilerOutputter( TestResultCollector *result,
                     std::ostream &stream );

  /// Destructor.
  virtual ~CompilerOutputter();

  /*! Creates an instance of an outputter that matches your current compiler.
   */
  static CompilerOutputter *defaultOutputter( TestResultCollector *result,
                                              std::ostream &stream );

  void write();

  virtual void printSucess();
  virtual void printFailureReport();
  virtual void printFailuresList();
  virtual void printStatistics();
  virtual void printFailureDetail( TestFailure *failure );
  virtual void printFailureLocation( SourceLine sourceLine );
  virtual void printFailureType( TestFailure *failure );
  virtual void printFailedTestName( TestFailure *failure );
  virtual void printFailureMessage( TestFailure *failure );
  virtual void printNotEqualMessage( Exception *thrownException );
  virtual void printDefaultMessage( Exception *thrownException );
  virtual std::string wrap( std::string message );

private:
  /// Prevents the use of the copy constructor.
  CompilerOutputter( const CompilerOutputter &copy );

  /// Prevents the use of the copy operator.
  void operator =( const CompilerOutputter &copy );

  typedef std::vector<std::string> Lines;
  static Lines splitMessageIntoLines( std::string message );

private:
  TestResultCollector *m_result;
  std::ostream &m_stream;
};


}  // namespace CppUnit



#endif  // CPPUNIT_COMPILERTESTRESULTOUTPUTTER_H
