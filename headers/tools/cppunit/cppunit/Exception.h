#ifndef CPPUNIT_EXCEPTION_H
#define CPPUNIT_EXCEPTION_H

#include <cppunit/Portability.h>
#include <cppunit/SourceLine.h>
#include <exception>
#include <string>

namespace CppUnit {

/*! \brief Exceptions thrown by failed assertions.
 * \ingroup BrowsingCollectedTestResult
 *
 * Exception is an exception that serves
 * descriptive strings through its what() method
 */
class CPPUNIT_API Exception : public exception
{
public:

    class Type
    {
    public:
        Type( string type ) : m_type ( type ) {}

        bool operator ==( const Type &other ) const
        {
	    return m_type == other.m_type;
        }
    private:
        const string m_type;
    };


    Exception( string  message = "", 
	       SourceLine sourceLine = SourceLine() );

#ifdef CPPUNIT_ENABLE_SOURCELINE_DEPRECATED
    Exception( string  message, 
	       long lineNumber, 
	       string fileName );
#endif

    Exception (const Exception& other);

    virtual ~Exception () throw();

    Exception& operator= (const Exception& other);

    const char *what() const throw ();

    SourceLine sourceLine() const;

#ifdef CPPUNIT_ENABLE_SOURCELINE_DEPRECATED
    long lineNumber() const;
    string fileName() const;

    static const string UNKNOWNFILENAME;
    static const long UNKNOWNLINENUMBER;
#endif

    virtual Exception *clone() const;
    
    virtual bool isInstanceOf( const Type &type ) const;

    static Type type();

private:
    // VC++ does not recognize call to parent class when prefixed
    // with a namespace. This is a workaround.
    typedef exception SuperClass;

    string m_message;
    SourceLine m_sourceLine;
};


} // namespace CppUnit

#endif // CPPUNIT_EXCEPTION_H

