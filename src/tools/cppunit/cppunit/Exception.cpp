#include "cppunit/Exception.h"


namespace CppUnit {


#ifdef CPPUNIT_ENABLE_SOURCELINE_DEPRECATED
/*!
 * \deprecated Use SourceLine::isValid() instead.
 */
const string Exception::UNKNOWNFILENAME = "<unknown>";

/*!
 * \deprecated Use SourceLine::isValid() instead.
 */
const long Exception::UNKNOWNLINENUMBER = -1;
#endif


/// Construct the exception
Exception::Exception( const Exception &other ) :
    exception( other )
{
  m_message = other.m_message;
  m_sourceLine = other.m_sourceLine;
}


/*!
 * \deprecated Use other constructor instead.
 */
Exception::Exception( std::string message,
                      SourceLine sourceLine ) :
    m_message( message ),
    m_sourceLine( sourceLine )
{
}


#ifdef CPPUNIT_ENABLE_SOURCELINE_DEPRECATED
/*!
 * \deprecated Use other constructor instead.
 */
Exception::Exception( std::string message,
                      long lineNumber,
                      std::string fileName ) :
    m_message( message ),
    m_sourceLine( fileName, lineNumber )
{
}
#endif


/// Destruct the exception
Exception::~Exception () throw()
{
}


/// Perform an assignment
Exception&
Exception::operator =( const Exception& other )
{
// Don't call superclass operator =(). VC++ STL implementation
// has a bug. It calls the destructor and copy constructor of
// exception() which reset the virtual table to exception.
//  SuperClass::operator =(other);

  if ( &other != this )
  {
    m_message = other.m_message;
    m_sourceLine = other.m_sourceLine;
  }

  return *this;
}


/// Return descriptive message
const char*
Exception::what() const throw()
{
  return m_message.c_str ();
}

/// Location where the error occured
SourceLine
Exception::sourceLine() const
{
  return m_sourceLine;
}


#ifdef CPPUNIT_ENABLE_SOURCELINE_DEPRECATED
/// The line on which the error occurred
long
Exception::lineNumber() const
{
  return m_sourceLine.isValid() ? m_sourceLine.lineNumber() :
                                  UNKNOWNLINENUMBER;
}


/// The file in which the error occurred
string
Exception::fileName() const
{
  return m_sourceLine.isValid() ? m_sourceLine.fileName() :
                                  UNKNOWNFILENAME;
}
#endif


Exception *
Exception::clone() const
{
  return new Exception( *this );
}


bool
Exception::isInstanceOf( const Type &exceptionType ) const
{
  return exceptionType == type();
}


Exception::Type
Exception::type()
{
  return Type( "CppUnit::Exception" );
}


}  // namespace CppUnit
