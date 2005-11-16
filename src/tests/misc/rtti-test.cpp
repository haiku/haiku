#include <stdio.h>

#include <string>
#include <typeinfo>

#include <Directory.h>
#include <File.h>

using std::string;

struct A {
	A()	{}
	virtual ~A() {}

	int a;
};

struct B : A {
	B()	{}
	virtual ~B() {}

	int b;
};

struct C : A {
	C()	{}
	virtual ~C() {}

	int c;
};

template<typename A, typename B, typename C>
static void
rtti_test(A *a, const char *inputClass, const char *classNameA,
	const char *classNameB, const char *classNameC)
{
	printf("class %s\n", inputClass);
	printf("  dynamic_cast<%s*>(a): %p\n", classNameA, dynamic_cast<A*>(a));
	printf("  dynamic_cast<%s*>(a): %p\n", classNameB, dynamic_cast<B*>(a));
	printf("  dynamic_cast<%s*>(a): %p\n", classNameC, dynamic_cast<C*>(a));
	const std::type_info *info = &typeid(*a);
	printf("  typeinfo: %p, name: %s\n", info, (info ? info->name() : NULL));
}

int
main()
{
	// test with artificial classes defined in this file
	
	#define RTTI_TEST(obj, className) rtti_test<A, B, C>(obj, className, \
		"A", "B", "C")
	
	A a;
	B b;
	C c;

	printf("A: %p (vtable: %p)\n", &a, *(void**)&a);
	printf("B: %p (vtable: %p)\n", &b, *(void**)&b);
	printf("C: %p (vtable: %p)\n", &c, *(void**)&c);

	RTTI_TEST(&a, "A");
	RTTI_TEST(&b, "B");
	RTTI_TEST(&c, "C");

	
	// test with real classes defined in a library

	#undef RTTI_TEST
	#define RTTI_TEST(obj, className) rtti_test<BNode, BFile, BDirectory>(obj, \
		className, "BNode", "BFile", "BDirectory")
	
	BNode node;
	BFile file;
	BDirectory dir;

	printf("BNode:      %p (vtable: %p)\n", &node, *(void**)&node);
	printf("BFile:      %p (vtable: %p)\n", &file, *(void**)&file);
	printf("BDirectory: %p (vtable: %p)\n", &dir, *(void**)&dir);

	RTTI_TEST(&node, "BNode");
	RTTI_TEST(&file, "BFile");
	RTTI_TEST(&dir, "BDirectory");

	return 0;
}
