#include <cppunit/Asserter.h>
#include <cppunit/NotEqualException.h>


namespace CppUnit
{


namespace Asserter
{


void 
fail( std::string message, 
      SourceLine sourceLine )
{
  throw Exception( message, sourceLine );
}


void 
failIf( bool shouldFail, 
        std::string message, 
        SourceLine location )
{
  if ( shouldFail )
    fail( message, location );
}


void 
failNotEqual( std::string expected, 
              std::string actual, 
              SourceLine sourceLine,
              std::string additionalMessage )
{
  throw NotEqualException( expected, 
                           actual, 
                           sourceLine, 
                           additionalMessage );
}


void 
failNotEqualIf( bool shouldFail,
                std::string expected, 
                std::string actual, 
                SourceLine sourceLine,
                std::string additionalMessage )
{
  if ( shouldFail )
    failNotEqual( expected, actual, sourceLine, additionalMessage );
}


} // namespace Asserter
} // namespace CppUnit
