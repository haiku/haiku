#include <TestSuite.h>
#include <TestSuiteAddon.h>
//#include <AVLTreeMapTest.h>
#include <SinglyLinkedListTest.h>

BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("KernelUtils");
	suite->addTest("SinglyLinkedList", SinglyLinkedListTest::Suite());
//	suite->addTest("AVLTreeMap", AVLTreeMapTest::Suite());
	return suite;
}
