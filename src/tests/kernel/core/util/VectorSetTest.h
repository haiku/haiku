#ifndef _vector_set_test_h_
#define _vector_set_test_h_

#include <TestCase.h>

class VectorSetTest : public BTestCase {
public:
	VectorSetTest(std::string name = "");

	static CppUnit::Test* Suite();
	
	void ConstructorTest();
	void InsertTest();
	void RemoveTest();
	void EraseTest();
	void MakeEmptyTest();
	void IndexAccessTest();
	void FindTest();
	void FindCloseTest();
	void IteratorTest();

private:
	template <class List>
	void TestList(List &list, typename List::ValueType *values, int valueCount);
};

#endif // _vector_set_test_h_
