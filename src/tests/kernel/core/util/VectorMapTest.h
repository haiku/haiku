#ifndef _vector_map_test_h_
#define _vector_map_test_h_

#include <TestCase.h>

class VectorMapTest : public BTestCase {
public:
	VectorMapTest(std::string name = "");

	static CppUnit::Test* Suite();
	
	void ConstructorTest();
	void InsertTest();
	void PutTest();
	void GetTest();
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

#endif // _vector_map_test_h_
