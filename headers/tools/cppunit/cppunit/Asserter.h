#ifndef CPPUNIT_ASSERTER_H
#define CPPUNIT_ASSERTER_H

#include <cppunit/Portability.h>
#include <cppunit/SourceLine.h>
#include <string>

namespace CppUnit
{

/*! \brief A set of functions to help writing assertion macros.
 * \ingroup CreatingNewAssertions
 *
 * Here is an example of assertion, a simplified version of the
 * actual assertion implemented in examples/cppunittest/XmlUniformiser.h:
 * \code
 * #include <cppunit/SourceLine.h>
 * #include <cppunit/TestAssert.h>
 * 
 * void 
 * checkXmlEqual( std::string expectedXml,
 *                std::string actualXml,
 *                CppUnit::SourceLine sourceLine )
 * {
 *   std::string expected = XmlUniformiser( expectedXml ).stripped();
 *   std::string actual = XmlUniformiser( actualXml ).stripped();
 * 
 *   if ( expected == actual )
 *     return;
 * 
 *   ::CppUnit::Asserter::failNotEqual( expected,
 *                                      actual,
 *                                      sourceLine );
 * }
 * 
 * /// Asserts that two XML strings are equivalent.
 * #define CPPUNITTEST_ASSERT_XML_EQUAL( expected, actual ) \
 *     checkXmlEqual( expected, actual,                     \
 *                    CPPUNIT_SOURCELINE() )
 * \endcode
 */
namespace Asserter
{

  /*! Throws a Exception with the specified message and location.
   */
  void CPPUNIT_API fail( std::string message, 
                         SourceLine sourceLine = SourceLine() );

  /*! Throws a Exception with the specified message and location.
   * \param shouldFail if \c true then the exception is thrown. Otherwise
   *                   nothing happen.
   * \param message Message explaining the assertion failiure.
   * \param sourceLine Location of the assertion.
   */
  void CPPUNIT_API failIf( bool shouldFail, 
                           std::string message, 
                           SourceLine sourceLine = SourceLine() );

  /*! Throws a NotEqualException with the specified message and location.
   * \param expected Text describing the expected value.
   * \param actual Text describing the actual value.
   * \param additionalMessage Additional message. Usually used to report
   *                          where the "difference" is located.
   * \param sourceLine Location of the assertion.
   */
  void CPPUNIT_API failNotEqual( std::string expected, 
                                 std::string actual, 
                                 SourceLine sourceLine = SourceLine(),
                                 std::string additionalMessage ="" );

  /*! Throws a NotEqualException with the specified message and location.
   * \param shouldFail if \c true then the exception is thrown. Otherwise
   *                   nothing happen.
   * \param expected Text describing the expected value.
   * \param actual Text describing the actual value.
   * \param additionalMessage Additional message. Usually used to report
   *                          where the "difference" is located.
   * \param sourceLine Location of the assertion.
   */
  void CPPUNIT_API failNotEqualIf( bool shouldFail,
                                   std::string expected, 
                                   std::string actual, 
                                   SourceLine sourceLine = SourceLine(),
                                   std::string additionalMessage ="" );

} // namespace Asserter
} // namespace CppUnit


#endif  // CPPUNIT_ASSERTER_H
