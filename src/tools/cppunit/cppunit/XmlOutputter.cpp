#include <cppunit/Exception.h>
#include <cppunit/Test.h>
#include <cppunit/TestFailure.h>
#include <cppunit/TestResultCollector.h>
#include <cppunit/XmlOutputter.h>
#include <map>
#include <stdlib.h>


using std::endl;
using std::ostream;
using std::pair;
using std::string;


namespace CppUnit
{

// XmlOutputter::Node
// //////////////////////////////////////////////////////////////////


XmlOutputter::Node::Node( string elementName,
                          string content ) :
    m_name( elementName ),
    m_content( content )
{
}


XmlOutputter::Node::Node( string elementName,
                          int numericContent ) :
    m_name( elementName )
{
  m_content = asString( numericContent );
}


XmlOutputter::Node::~Node()
{
  Nodes::iterator itNode = m_nodes.begin();
  while ( itNode != m_nodes.end() )
    delete *itNode++;
}


void
XmlOutputter::Node::addAttribute( string attributeName,
                                  string value  )
{
  m_attributes.push_back( Attribute( attributeName, value ) );
}


void
XmlOutputter::Node::addAttribute( string attributeName,
                                  int numericValue )
{
  addAttribute( attributeName, asString( numericValue ) );
}


void
XmlOutputter::Node::addNode( Node *node )
{
  m_nodes.push_back( node );
}


string
XmlOutputter::Node::toString() const
{
  string element = "<";
  element += m_name;
  element += " ";
  element += attributesAsString();
  element += " >\n";

  Nodes::const_iterator itNode = m_nodes.begin();
  while ( itNode != m_nodes.end() )
  {
    const Node *node = *itNode++;
    element += node->toString();
  }

  element += m_content;

  element += "</";
  element += m_name;
  element += ">\n";

  return element;
}


string
XmlOutputter::Node::attributesAsString() const
{
  string attributes;
  Attributes::const_iterator itAttribute = m_attributes.begin();
  while ( itAttribute != m_attributes.end() )
  {
    const Attribute &attribute = *itAttribute++;
    attributes += attribute.first;
    attributes += "=\"";
    attributes += escape( attribute.second );
    attributes += "\"";
  }
  return attributes;
}


string
XmlOutputter::Node::escape( string value ) const
{
  string escaped;
  for ( int index =0; index < (int)value.length(); ++index )
  {
    char c = value[index ];
    switch ( c )    // escape all predefined XML entity (safe?)
    {
    case '<':
      escaped += "&lt;";
      break;
    case '>':
      escaped += "&gt;";
      break;
    case '&':
      escaped += "&amp;";
      break;
    case '\'':
      escaped += "&apos;";
      break;
    case '"':
      escaped += "&quot;";
      break;
    default:
      escaped += c;
    }
  }

  return escaped;
}

// should be somewhere else... Future CppUnit::String ?
string
XmlOutputter::Node::asString( int value )
{
  OStringStream stream;
  stream << value;
  return stream.str();
}




// XmlOutputter
// //////////////////////////////////////////////////////////////////

XmlOutputter::XmlOutputter( TestResultCollector *result,
                            ostream &stream,
                            string encoding ) :
    m_result( result ),
    m_stream( stream ),
    m_encoding( encoding )
{
}


XmlOutputter::~XmlOutputter()
{
}


void
XmlOutputter::write()
{
  writeProlog();
  writeTestsResult();
}


void
XmlOutputter::writeProlog()
{
  m_stream  <<  "<?xml version=\"1.0\" "
                "encoding='"  <<  m_encoding  << "' standalone='yes' ?>"
            <<  endl;
}


void
XmlOutputter::writeTestsResult()
{
  Node *rootNode = makeRootNode();
  m_stream  <<  rootNode->toString();
  delete rootNode;
}


XmlOutputter::Node *
XmlOutputter::makeRootNode()
{
  Node *rootNode = new Node( "TestRun" );

  FailedTests failedTests;
  fillFailedTestsMap( failedTests );

  addFailedTests( failedTests, rootNode );
  addSucessfulTests( failedTests, rootNode );
  addStatistics( rootNode );

  return rootNode;
}


void
XmlOutputter::fillFailedTestsMap( FailedTests &failedTests )
{
  const TestResultCollector::TestFailures &failures = m_result->failures();
  TestResultCollector::TestFailures::const_iterator itFailure = failures.begin();
  while ( itFailure != failures.end() )
  {
    TestFailure *failure = *itFailure++;
    failedTests.insert(
    	pair< CppUnit::Test* const, CppUnit::TestFailure*
    	>(
    		failure->failedTest(), failure
    	)
    );
  }
}


void
XmlOutputter::addFailedTests( FailedTests &failedTests,
                                        Node *rootNode )
{
  Node *testsNode = new Node( "FailedTests" );
  rootNode->addNode( testsNode );

  const TestResultCollector::Tests &tests = m_result->tests();
  for ( int testNumber = 0; testNumber < (int)tests.size(); ++testNumber )
  {
    Test *test = tests[testNumber];
    if ( failedTests.find( test ) != failedTests.end() )
      addFailedTest( test, failedTests[test], testNumber+1, testsNode );
  }
}


void
XmlOutputter::addSucessfulTests( FailedTests &failedTests,
                                           Node *rootNode )
{
  Node *testsNode = new Node( "SucessfulTests" );
  rootNode->addNode( testsNode );

  const TestResultCollector::Tests &tests = m_result->tests();
  for ( int testNumber = 0; testNumber < (int)tests.size(); ++testNumber )
  {
    Test *test = tests[testNumber];
    if ( failedTests.find( test ) == failedTests.end() )
      addSucessfulTest( test, testNumber+1, testsNode );
  }
}


void
XmlOutputter::addStatistics( Node *rootNode )
{
  Node *statisticsNode = new Node( "Statistics" );
  rootNode->addNode( statisticsNode );
  statisticsNode->addNode( new Node( "Tests", m_result->runTests() ) );
  statisticsNode->addNode( new Node( "FailuresTotal",
                                     m_result->testFailuresTotal() ) );
  statisticsNode->addNode( new Node( "Errors", m_result->testErrors() ) );
  statisticsNode->addNode( new Node( "Failures", m_result->testFailures() ) );
}


void
XmlOutputter::addFailedTest( Test *test,
                                       TestFailure *failure,
                                       int testNumber,
                                       Node *testsNode )
{
  Exception *thrownException = failure->thrownException();

  Node *testNode = new Node( "FailedTest", thrownException->what() );
  testsNode->addNode( testNode );
  testNode->addAttribute( "id", testNumber );
  testNode->addNode( new Node( "Name", test->getName() ) );
  testNode->addNode( new Node( "FailureType",
                               failure->isError() ? "Error" : "Assertion" ) );

  if ( failure->sourceLine().isValid() )
    addFailureLocation( failure, testNode );
}


void
XmlOutputter::addFailureLocation( TestFailure *failure,
                                            Node *testNode )
{
  Node *locationNode = new Node( "Location" );
  testNode->addNode( locationNode );
  SourceLine sourceLine = failure->sourceLine();
  locationNode->addNode( new Node( "File", sourceLine.fileName() ) );
  locationNode->addNode( new Node( "Line", sourceLine.lineNumber() ) );
}


void
XmlOutputter::addSucessfulTest( Test *test,
                                          int testNumber,
                                          Node *testsNode )
{
  Node *testNode = new Node( "Test" );
  testsNode->addNode( testNode );
  testNode->addAttribute( "id", testNumber );
  testNode->addNode( new Node( "Name", test->getName() ) );
}


}  // namespace CppUnit
