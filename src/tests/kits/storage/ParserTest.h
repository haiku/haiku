// ParserTest.h

#ifndef __sk_parser_test_h__
#define __sk_parser_test_h__

#include <TestCase.h>

class ParserTest : public BTestCase {
public:
	static CppUnit::Test* Suite();
	
	//------------------------------------------------------------
	// Test functions
	//------------------------------------------------------------
	void ScannerTest();

	//------------------------------------------------------------
	// Helper functions
	//------------------------------------------------------------

};


#endif	// __sk_parser_test_h__
