#ifndef _vector_test_h_
#define _vector_test_h_

#include <TestCase.h>

class VectorTest : public BTestCase {
public:
	VectorTest(std::string name = "");

	static CppUnit::Test* Suite();
	
	void ConstructorTest1();
	void ConstructorTest2();
	void PushPopFrontTest();
	void PushPopBackTest();
	void InsertTest();
	void RemoveTest();
	void EraseTest();
	void MakeEmptyTest();
	void IndexAccessTest();
	void FindTest();
	void IteratorTest();

private:
	template <class List>
	void TestList(List &list, typename List::ValueType *values, int valueCount);
};

#endif // _vector_test_h_
