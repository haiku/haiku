#include <cppunit/TestFailure.h>
#include <cppunit/TestListener.h>
#include <cppunit/TestResult.h>
#include <algorithm>

namespace CppUnit {

/// Construct a TestResult
TestResult::TestResult( SynchronizationObject *syncObject )
    : SynchronizedObject( syncObject )
{ 
  reset();
}


/// Destroys a test result
TestResult::~TestResult()
{
}


/** Resets the result for a new run.
 *
 * Clear the previous run result.
 */
void 
TestResult::reset()
{
  ExclusiveZone zone( m_syncObject ); 
  m_stop = false;
}


/** Adds an error to the list of errors. 
 *  The passed in exception
 *  caused the error
 */
void 
TestResult::addError( Test *test, 
                      Exception *e )
{ 
  addFailure( TestFailure( test, e, true ) );
}


/** Adds a failure to the list of failures. The passed in exception
 * caused the failure.
 */
void 
TestResult::addFailure( Test *test, Exception *e )
{ 
  addFailure( TestFailure( test, e, false ) );
}


/** Called to add a failure to the list of failures.
 */
void 
TestResult::addFailure( const TestFailure &failure )
{
  ExclusiveZone zone( m_syncObject ); 
  for ( TestListeners::iterator it = m_listeners.begin();
        it != m_listeners.end(); 
        ++it )
    (*it)->addFailure( failure );
}


/// Informs the result that a test will be started.
void 
TestResult::startTest( Test *test )
{ 
  ExclusiveZone zone( m_syncObject ); 
  for ( TestListeners::iterator it = m_listeners.begin();
        it != m_listeners.end(); 
        ++it )
    (*it)->startTest( test );
}

  
/// Informs the result that a test was completed.
void 
TestResult::endTest( Test *test )
{ 
  ExclusiveZone zone( m_syncObject ); 
  for ( TestListeners::iterator it = m_listeners.begin();
        it != m_listeners.end(); 
        ++it )
    (*it)->endTest( test );
}


/// Returns whether testing should be stopped
bool 
TestResult::shouldStop() const
{ 
  ExclusiveZone zone( m_syncObject );
  return m_stop; 
}


/// Stop testing
void 
TestResult::stop()
{ 
  ExclusiveZone zone( m_syncObject );
  m_stop = true; 
}


void 
TestResult::addListener( TestListener *listener )
{
  ExclusiveZone zone( m_syncObject ); 
  m_listeners.push_back( listener );
}


void 
TestResult::removeListener ( TestListener *listener )
{
  ExclusiveZone zone( m_syncObject ); 
  m_listeners.erase( remove( m_listeners.begin(), 
                                  m_listeners.end(), 
                                  listener ),
                     m_listeners.end());
}

} // namespace CppUnit
