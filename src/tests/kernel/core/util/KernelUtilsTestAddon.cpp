#include <TestSuite.h>
#include <TestSuiteAddon.h>

//#include "AVLTreeMapTest.h"
#include "SinglyLinkedListTest.h"
#include "DoublyLinkedListTest.h"
#include "VectorMapTest.h"
#include "VectorSetTest.h"
#include "VectorTest.h"


BTestSuite* getTestSuite() {
	BTestSuite *suite = new BTestSuite("KernelUtils");
//	suite->addTest("AVLTreeMap", AVLTreeMapTest::Suite());
	suite->addTest("SinglyLinkedList", SinglyLinkedListTest::Suite());
	suite->addTest("DoublyLinkedList", DoublyLinkedListTest::Suite());
	suite->addTest("VectorMap", VectorMapTest::Suite());
	suite->addTest("VectorSet", VectorSetTest::Suite());
	suite->addTest("Vector", VectorTest::Suite());
	return suite;
}
