#ifndef _DOUBLY_LINKED_LIST_TEST_H_
#define _DOUBLY_LINKED_LIST_TEST_H_

#include <TestCase.h>

class DoublyLinkedListTest : public BTestCase {
	public:
		DoublyLinkedListTest(std::string name = "");

		static CppUnit::Test *Suite();

		void WithoutOffsetTest();
		void WithOffsetTest();	
		void VirtualWithoutOffsetTest();
		void VirtualWithOffsetTest();	

	private:
		template <typename Item> void TestList();
};

#endif	/* _DOUBLY_LINKED_LIST_TEST_H_ */
