#ifndef _avl_tree_map_test_h_
#define _avl_tree_map_test_h_

#include <TestCase.h>

class AVLTreeMapTest : public BTestCase {
public:
	AVLTreeMapTest(std::string name = "");

	static CppUnit::Test* Suite();
	
	void Test1();

private:
	template <class List>
	void TestList(List &list, typename List::ValueType *values, int valueCount);
};

#endif // _avl_tree_map_test_h_
