#include "../common.h"

#include <Application.h>
#include <String.h>
#include <TextView.h>

class TextViewTestcase: public TestCase {
public:
	void
	SizeTest()
	{
		CPPUNIT_ASSERT_EQUAL(356, sizeof(BTextView));
	}

	void
	GetTextTest()
	{
		BApplication app("application/x-vnd.Haiku-interfacekit-textviewtest");
		BRect textRect(0, 0, 100, 100);
		BTextView* v = new BTextView(textRect, "test", textRect, 0, 0);
		v->SetText("Initial text");
		v->Insert(8, "(inserted) ", 10);
		char buffer[12];
		v->GetText(2, 11, buffer);
		CPPUNIT_ASSERT_EQUAL(BString("itial (inse"), buffer);
	}
};


Test*
TextViewTestSuite()
{
	TestSuite *testSuite = new TestSuite();

	testSuite->addTest(new CppUnit::TestCaller<TextViewTestcase>(
		"BTextView_Size", &TextViewTestcase::SizeTest));
	testSuite->addTest(new CppUnit::TestCaller<TextViewTestcase>(
		"BTextView_GetText", &TextViewTestcase::GetTextTest));

	return testSuite;
}
