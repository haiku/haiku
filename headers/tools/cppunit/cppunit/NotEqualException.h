#ifndef NOTEQUALEXCEPTION_H
#define NOTEQUALEXCEPTION_H

#include <cppunit/Exception.h>


namespace CppUnit {

/*! \brief Exception thrown by failed equality assertions.
 * \ingroup BrowsingCollectedTestResult
 */
class CPPUNIT_API NotEqualException : public Exception
{
public:
  /*! Constructs the exception.
   * \param expected Text that represents the expected value.
   * \param actual Text that represents the actual value.
   * \param sourceLine Location of the assertion.
   * \param additionalMessage Additionnal information provided to further qualify
   *                          the inequality.
   */
  NotEqualException( string expected,
                     string actual, 
                     SourceLine sourceLine = SourceLine(),
                     string additionalMessage = "" );

#ifdef CPPUNIT_ENABLE_SOURCELINE_DEPRECATED
  NotEqualException( string expected,
                     string actual, 
                     long lineNumber, 
                     string fileName );
#endif

  NotEqualException( const NotEqualException &other );


  virtual ~NotEqualException() throw();

  string expectedValue() const;

  string actualValue() const;

  string additionalMessage() const;

  /*! Copy operator.
   * @param other Object to copy.
   * @return Reference on this object.
   */
  NotEqualException &operator =( const NotEqualException &other );

  Exception *clone() const;

  bool isInstanceOf( const Type &type ) const;

  static Type type();

private:
  string m_expected;
  string m_actual;
  string m_additionalMessage;
};

}  // namespace CppUnit

#endif  // NOTEQUALEXCEPTION_H
