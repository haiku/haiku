#include <cppunit/NotEqualException.h>

namespace CppUnit {


NotEqualException::NotEqualException( std::string expected,
                                      std::string actual, 
                                      SourceLine sourceLine ,
                                      std::string additionalMessage ) :
    Exception( "Expected: " + expected + 
                   ", but was: " + actual + 
                   "." + additionalMessage ,
               sourceLine),
    m_expected( expected ),
    m_actual( actual ),
    m_additionalMessage( additionalMessage )
{
}


#ifdef CPPUNIT_ENABLE_SOURCELINE_DEPRECATED
/*!
 * \deprecated Use other constructor instead.
 */
NotEqualException::NotEqualException( std::string expected,
                                      std::string actual,
                                      long lineNumber, 
                                      std::string fileName ) : 
    Exception( "Expected: " + expected + ", but was: " + actual,
               lineNumber,
               fileName ),
    m_expected( expected ),
    m_actual( actual )
{
}
#endif


NotEqualException::NotEqualException( const NotEqualException &other ) : 
    Exception( other ),
    m_expected( other.m_expected ),
    m_actual( other.m_actual ),
    m_additionalMessage( other.m_additionalMessage )
{
}


NotEqualException::~NotEqualException() throw()
{
}


NotEqualException &
NotEqualException::operator =( const NotEqualException &other )
{
  Exception::operator =( other );

  if ( &other != this )
  {
    m_expected = other.m_expected;
    m_actual = other.m_actual;
    m_additionalMessage = other.m_additionalMessage;
  }
  return *this;
}


Exception *
NotEqualException::clone() const
{
  return new NotEqualException( *this );
}


bool 
NotEqualException::isInstanceOf( const Type &exceptionType ) const
{
  return exceptionType == type()  ||
         Exception::isInstanceOf( exceptionType );
}


Exception::Type
NotEqualException::type()
{
  return Type( "CppUnit::NotEqualException" );
}


std::string 
NotEqualException::expectedValue() const
{
  return m_expected;
}


std::string 
NotEqualException::actualValue() const
{
  return m_actual;
}


std::string 
NotEqualException::additionalMessage() const
{
  return m_additionalMessage;
}


}  //  namespace CppUnit
