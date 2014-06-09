#ifndef _single_linked_list_test_h_
#define _single_linked_list_test_h_

#include <TestCase.h>

class SinglyLinkedListTest : public BTestCase {
public:
	SinglyLinkedListTest(std::string name = "");

	static CppUnit::Test* Suite();
	
	void UserDefaultTest();	
	void UserCustomTest();
private:
	template <class List, class Element>
	void TestList(List &list, Element *values, int valueCount);
};

#endif // _single_linked_list_test_h_
