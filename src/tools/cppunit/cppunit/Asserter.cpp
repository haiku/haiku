#include <cppunit/Asserter.h>
#include <cppunit/NotEqualException.h>


using std::string;

namespace CppUnit
{


namespace Asserter
{


void
fail( string message,
      SourceLine sourceLine )
{
  throw Exception( message, sourceLine );
}


void
failIf( bool shouldFail,
        string message,
        SourceLine location )
{
  if ( shouldFail )
    fail( message, location );
}


void
failNotEqual( string expected,
              string actual,
              SourceLine sourceLine,
              string additionalMessage )
{
  throw NotEqualException( expected,
                           actual,
                           sourceLine,
                           additionalMessage );
}


void
failNotEqualIf( bool shouldFail,
                string expected,
                string actual,
                SourceLine sourceLine,
                string additionalMessage )
{
  if ( shouldFail )
    failNotEqual( expected, actual, sourceLine, additionalMessage );
}


} // namespace Asserter
} // namespace CppUnit
