#ifndef CPPUNIT_OUTPUTTER_H
#define CPPUNIT_OUTPUTTER_H

#include <cppunit/Portability.h>


namespace CppUnit
{

/*! \brief Abstract outputter to print test result summary.
 * \ingroup WritingTestResult
 */
class CPPUNIT_API Outputter
{
public:
  /// Destructor.
  virtual ~Outputter() {}

  virtual void write() =0;
};



// Inlines methods for Outputter:
// ------------------------------


} //  namespace CppUnit


#endif  // CPPUNIT_OUTPUTTER_H
