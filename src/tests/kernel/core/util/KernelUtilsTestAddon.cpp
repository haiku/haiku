#include <TestSuite.h>
#include <TestSuiteAddon.h>
#include <SinglyLinkedListTest.h>

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("KernelUtils");
	suite->addTest("SinglyLinkedList", SinglyLinkedListTest::Suite());
	return suite;
}
