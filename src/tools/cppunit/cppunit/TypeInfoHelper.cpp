#include <cppunit/Portability.h>

#if CPPUNIT_USE_TYPEINFO_NAME

#include <string>
#include <cppunit/extensions/TypeInfoHelper.h>


namespace CppUnit {

std::string 
TypeInfoHelper::getClassName( const std::type_info &info )
{
    static std::string classPrefix( "class " );
    std::string name( info.name() );

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
