#include <cppunit/SourceLine.h>


using std::string;

namespace CppUnit
{

SourceLine::SourceLine() :
    m_lineNumber( -1 )
{
}


SourceLine::SourceLine( const string &fileName,
                        int lineNumber ) :
    m_fileName( fileName ),
    m_lineNumber( lineNumber )
{
}


SourceLine::~SourceLine()
{
}


bool
SourceLine::isValid() const
{
  return !m_fileName.empty();
}


int
SourceLine::lineNumber() const
{
  return m_lineNumber;
}


string
SourceLine::fileName() const
{
  return m_fileName;
}


bool
SourceLine::operator ==( const SourceLine &other ) const
{
  return m_fileName == other.m_fileName  &&
         m_lineNumber == other.m_lineNumber;
}


bool
SourceLine::operator !=( const SourceLine &other ) const
{
  return !( *this == other );
}


} // namespace CppUnit
