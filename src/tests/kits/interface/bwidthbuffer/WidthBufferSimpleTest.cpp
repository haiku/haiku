#include "WidthBufferSimpleTest.h"
#include "cppunit/TestCaller.h"

#include <Application.h>
#include <Font.h>

#include <stdio.h>

// Needed because it isn't available in beos headers
double round(float n)
{	
	int32 tmp = (int32)floor(n + 0.5);
	return (double)tmp;
}


WidthBufferSimpleTest::WidthBufferSimpleTest(std::string name) :
		BTestCase(name)
{
	fWidthBuffer = new _BWidthBuffer_;
}

 

WidthBufferSimpleTest::~WidthBufferSimpleTest()
{
	delete fWidthBuffer;
}


void 
WidthBufferSimpleTest::PerformTest(void)
{
	const char *str = "Something";
	const char *str2 = "Some Text";
	const char *longString = "ABCDEFGHIJKLMNOPQRSTUVYXWZ abcdefghijklmnopqrstuvwyxz";
	
	// Some charachters which takes more than one byte
	const char *str3 = "30u8g 2342 42nfrç°S£^GF°W§|£^?£ç$£!1ààà'ì";
	
	const BFont *font = be_plain_font;
	
	// We use round() here because the width returned by
	// _BWidthBuffer_::StringWidth() isn't _exactly_ the same
	// as the one returned by BFont::StringWidth();
	float width = fWidthBuffer->StringWidth(str, 0, strlen(str), font);
	CPPUNIT_ASSERT(round(width) == font->StringWidth(str)); 
	
	width = fWidthBuffer->StringWidth(str2, 0, strlen(str2), font);
	CPPUNIT_ASSERT(round(width) == font->StringWidth(str2));
	
	width = fWidthBuffer->StringWidth(longString, 0, strlen(longString), font);
	CPPUNIT_ASSERT(round(width) == font->StringWidth(longString));
	
	// pass the first string again, it should be completely cached
	// in _BWidthBuffer_'s hash table
	width = fWidthBuffer->StringWidth(str, 0, strlen(str), font);
	CPPUNIT_ASSERT(round(width) == font->StringWidth(str)); 
	
	width = fWidthBuffer->StringWidth(str3, 0, strlen(str3), font);
	CPPUNIT_ASSERT(round(width) == font->StringWidth(str3)); 
	
	// choose a different font and ask the same strings as before
	font = be_fixed_font;
	width = fWidthBuffer->StringWidth(longString, 0, strlen(longString), font);
	CPPUNIT_ASSERT(round(width) == font->StringWidth(longString));
	
	width = fWidthBuffer->StringWidth(str, 0, strlen(str), font);
	CPPUNIT_ASSERT(round(width) == font->StringWidth(str)); 
	
	width = fWidthBuffer->StringWidth(str2, 0, strlen(str2), font);
	CPPUNIT_ASSERT(round(width) == font->StringWidth(str2));
}


CppUnit::Test *WidthBufferSimpleTest::suite(void)
{	
	typedef CppUnit::TestCaller<WidthBufferSimpleTest>
		WidthBufferSimpleTestCaller;
		
	return(new WidthBufferSimpleTestCaller("_BWidthBuffer_::Simple Test", &WidthBufferSimpleTest::PerformTest));
}
