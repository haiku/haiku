#include "../common.h"

#include <Application.h>
#include <String.h>
#include <TextControl.h>

class TextControlTestcase: public TestCase {
public:
	void
	SizeTest()
	{
		CPPUNIT_ASSERT_EQUAL(312, sizeof(BTextControl));
	}

	void
	GetTextTest()
	{
		BApplication app("application/x-vnd.Haiku-interfacekit-textcontroltest");
		BRect textRect(0, 0, 100, 100);
		BTextControl* v = new BTextControl(textRect, "test", 0, 0, 0);
		v->SetText("Initial text");
		v->TextView()->Insert(8, "(inserted) ", 10);
		CPPUNIT_ASSERT_EQUAL(BString("Initial (inserted)text"), v->Text());
	}
};


Test*
TextControlTestSuite()
{
	TestSuite *testSuite = new TestSuite();

	testSuite->addTest(new CppUnit::TestCaller<TextControlTestcase>(
		"BTextControl_Size", &TextControlTestcase::SizeTest));
	testSuite->addTest(new CppUnit::TestCaller<TextControlTestcase>(
		"BTextControl_GetText", &TextControlTestcase::GetTextTest));

	return testSuite;
}
