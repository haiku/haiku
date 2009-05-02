#include <cppunit/Portability.h>

#if CPPUNIT_USE_TYPEINFO_NAME

#include <string>
#include <cppunit/extensions/TypeInfoHelper.h>


using std::string;
using std::type_info;

namespace CppUnit {

string
TypeInfoHelper::getClassName( const type_info &info )
{
    static const string classPrefix( "class " );
    string name( info.name() );

    bool has_class_prefix = 0 ==
#if CPPUNIT_FUNC_STRING_COMPARE_STRING_FIRST
	name.compare( classPrefix, 0, classPrefix.length() );
#else
    name.compare( 0, classPrefix.length(), classPrefix );
#endif

    return has_class_prefix ? name.substr( classPrefix.length() ) : name;
}


} //  namespace CppUnit

#endif
