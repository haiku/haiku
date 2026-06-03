/*
 * Copyright 2015, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include "DriverSettingsMessageAdapterTest.h"

#include <stdarg.h>
#include <stdlib.h>

#include <driver_settings.h>
#include <ByteOrder.h>
#include <String.h>

#include <DriverSettingsMessageAdapter.h>

#include <cppunit/TestCaller.h>
#include <cppunit/TestSuite.h>


namespace {


class Settings {
public:
	Settings(const char* string)
	{
		fSettings = parse_driver_settings_string(string);
		CppUnit::Asserter::failIf(fSettings == NULL,
			"Could not parse settings");
	}

	status_t ToMessage(const settings_template* settingsTemplate,
		BMessage& message)
	{
		DriverSettingsMessageAdapter adapter;
		return adapter.ConvertFromDriverSettings(
			*get_driver_settings(fSettings), settingsTemplate, message);
	}

	~Settings()
	{
		unload_driver_settings(fSettings);
	}

private:
	void* fSettings;
};


void
FailWithMessage(const char* fmt, ...)
{
	BString errorMessage;
	va_list args;
	va_start(args, fmt);
	errorMessage.SetToFormatVarArgs(fmt, args);
	va_end(args);

	CppUnit::Asserter::fail(errorMessage.String());
}


void
AssertMessagesEqual(const BMessage& expected, const BMessage& actual)
{
	if (expected.CountNames(B_ANY_TYPE) != actual.CountNames(B_ANY_TYPE)) {
		expected.PrintToStream();
		actual.PrintToStream();
		CppUnit::Asserter::fail("Messages don't match");
	}

	int32 index = 0;
	char* name = NULL;
	type_code type;
	int32 count;
	while (expected.GetInfo(B_ANY_TYPE, index++, &name, &type, &count) == B_OK) {
		type_code actualType;
		int32 actualCount;
		if (actual.GetInfo(name, &actualType, &actualCount) != B_OK)
			FailWithMessage("Couldn't find property %s", name);
		if (actualType != type) {
			int32 expectedCodeType = B_SWAP_INT32(type);
			int32 actualCodeType = B_SWAP_INT32(actualType);
			FailWithMessage("Expected type %.4s for property %s, but found %.4s",
				(char*)&expectedCodeType, name, (char*)&actualCodeType);
		}
		if (actualCount != count) {
			FailWithMessage("Expected %" B_PRId32 " items for property %s, but found %" B_PRId32,
				count, name, actualCount);
		}

		for (int32 valueIndex = 0; valueIndex < count; valueIndex++) {
			switch (type) {
				case B_BOOL_TYPE:
				{
					bool data, actualData;
					expected.FindBool(name, valueIndex, &data);
					actual.FindBool(name, valueIndex, &actualData);
					if (data != actualData) {
						FailWithMessage("Expected %c for property %s, but found %c",
							data ? 'Y' : 'N', name, actualData ? 'Y' : 'N');
					}

					break;
				}

				case B_STRING_TYPE:
				{
					const char* data;
					const char* actualData;
					expected.FindString(name, valueIndex, &data);
					actual.FindString(name, valueIndex, &actualData);
					if (strcmp(data, actualData) != 0) {
						FailWithMessage("Expected '%s' for property %s, but found '%s'",
							data, name, actualData);
					}

					break;
				}

				case B_INT32_TYPE:
				{
					int32 data, actualData;
					expected.FindInt32(name, valueIndex, &data);
					actual.FindInt32(name, valueIndex, &actualData);
					if (data != actualData) {
						FailWithMessage("Expected %" B_PRId32
							" for property %s, but found %" B_PRId32, data, name, actualData);
					}

					break;
				}

				case B_MESSAGE_TYPE:
				{
					BMessage data, actualData;
					expected.FindMessage(name, valueIndex, &data);
					actual.FindMessage(name, valueIndex, &actualData);
					AssertMessagesEqual(data, actualData);

					break;
				}
			}
		}
	}
}


}	// empty namespace


// #pragma mark -


DriverSettingsMessageAdapterTest::DriverSettingsMessageAdapterTest()
{
}


DriverSettingsMessageAdapterTest::~DriverSettingsMessageAdapterTest()
{
}


void
DriverSettingsMessageAdapterTest::TestPrimitivesToMessage()
{
	const settings_template kTemplate[] = {
		{B_BOOL_TYPE, "bool1", NULL},
		{B_BOOL_TYPE, "bool2", NULL},
		{B_BOOL_TYPE, "bool3", NULL},
		{B_BOOL_TYPE, "bool4", NULL},
		{B_BOOL_TYPE, "bool5", NULL},
		{B_BOOL_TYPE, "bool6", NULL},
		{B_BOOL_TYPE, "bool7", NULL},
		{B_BOOL_TYPE, "bool8", NULL},
		{B_BOOL_TYPE, "bool9", NULL},
		{B_BOOL_TYPE, "empty_bool", NULL},
		{B_INT32_TYPE, "int32", NULL},
		{B_INT32_TYPE, "negative_int32", NULL},
		{B_INT32_TYPE, "empty_int32", NULL},
		{B_STRING_TYPE, "string", NULL},
		{B_STRING_TYPE, "long_string", NULL},
		{B_STRING_TYPE, "empty_string", NULL},
		{}
	};
	Settings settings("bool1 true\n"
		"bool2 1\n"
		"bool3 on\n"
		"bool4 yes\n"
		"bool5 enabled\n"
		"bool6 false\n"
		"bool7 off\n"
		"bool8 gurkensalat\n"
		"bool9 0\n"
		"empty_bool\n"
		"int32 42\n"
		"negative_int32 -42\n"
		"empty_int32\n"
		"string Hey\n"
		"long_string \"This is but a test\"\n"
		"empty_string\n");

	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, settings.ToMessage(kTemplate, message));
	CPPUNIT_ASSERT_EQUAL(10, message.CountNames(B_BOOL_TYPE));
	CPPUNIT_ASSERT_EQUAL(2, message.CountNames(B_INT32_TYPE));
	CPPUNIT_ASSERT_EQUAL(2, message.CountNames(B_STRING_TYPE));
	CPPUNIT_ASSERT_EQUAL(14, message.CountNames(B_ANY_TYPE));

	// bool values
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool1", true, message.GetBool("bool1"));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool2", true, message.GetBool("bool2"));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool3", true, message.GetBool("bool3"));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool4", true, message.GetBool("bool4"));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool5", true, message.GetBool("bool5"));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool6", false,
		message.GetBool("bool6", true));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool7", false,
		message.GetBool("bool7", true));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool8", false,
		message.GetBool("bool8", true));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool9", false,
		message.GetBool("bool9", true));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("empty_bool", true,
		message.GetBool("empty_bool"));

	// int32 values
	CPPUNIT_ASSERT_EQUAL(42, message.GetInt32("int32", 0));
	CPPUNIT_ASSERT_EQUAL(-42, message.GetInt32("negative_int32", 0));
	CPPUNIT_ASSERT_EQUAL(false, message.HasInt32("empty_int32"));

	// string values
	CPPUNIT_ASSERT_EQUAL(BString("Hey"), BString(message.GetString("string")));
	CPPUNIT_ASSERT_EQUAL(BString("This is but a test"),
		BString(message.GetString("long_string")));
	CPPUNIT_ASSERT_EQUAL(false, message.HasString("empty_string"));
}


void
DriverSettingsMessageAdapterTest::TestMessage()
{
	const settings_template kSubTemplate[] = {
		{B_BOOL_TYPE, "bool", NULL},
		{}
	};
	const settings_template kTemplate[] = {
		{B_MESSAGE_TYPE, "message", kSubTemplate},
		{}
	};
	Settings settingsA("message {\n"
		"    bool\n"
		"}\n");
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, settingsA.ToMessage(kTemplate, message));
	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("message", &subMessage));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool", true, subMessage.GetBool("bool"));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));

	Settings settingsB("message {\n"
		"}\n");
	CPPUNIT_ASSERT_EQUAL(B_OK, settingsB.ToMessage(kTemplate, message));
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("message", &subMessage));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));

	Settings settingsC("\n");
	CPPUNIT_ASSERT_EQUAL(B_OK, settingsC.ToMessage(kTemplate, message));
	CPPUNIT_ASSERT(message.IsEmpty());
}


void
DriverSettingsMessageAdapterTest::TestFromMessage()
{
	const settings_template kTemplateNoParent[] = {
		{B_STRING_TYPE, NULL, NULL},
		{}
	};
	const settings_template kTemplateName[] = {
		{B_STRING_TYPE, "name", NULL, true},
		{B_STRING_TYPE, NULL, NULL},
		{}
	};
	const settings_template kTemplate[] = {
		{B_STRING_TYPE, NULL, NULL},
		{B_INT32_TYPE, "int+", NULL},
		{B_INT32_TYPE, "int-", NULL},
		{B_BOOL_TYPE, "Yay", NULL},
		{B_BOOL_TYPE, "Nay", NULL},
		{B_MESSAGE_TYPE, "message_no_subparent", kTemplateNoParent},
		{B_MESSAGE_TYPE, "message_subparent", kTemplateName},
		{B_MESSAGE_TYPE, "empty_message_no_subparent", kTemplateNoParent},
		{B_MESSAGE_TYPE, "empty_message_subparent", kTemplateName},
		{}
	};

	BMessage initialMessage;
	initialMessage.AddString("1", "with spaces");
	initialMessage.AddString("2", "array 1");
	initialMessage.AddString("2", "array 2");
	initialMessage.AddString("2", "array {");
	initialMessage.AddString("4", "{");
	initialMessage.AddString("5", "}");
	initialMessage.AddString("6", "how about a newline?\nand stuff");
	initialMessage.AddString("7", "\"qu\"ote\"");
	initialMessage.AddString("8", "'nother'quote'");
	initialMessage.AddString("9", "\\back\\slash\\");
	initialMessage.AddString("10", "#NotAComment");
	initialMessage.AddString("11", "NotA;Separator");
	initialMessage.AddString("12", "v=3");
	initialMessage.AddString("empty", "");
	initialMessage.AddString("space", " ");
	initialMessage.AddString("space", "\f");
	initialMessage.AddString("space", "\n");
	initialMessage.AddString("space", "\r");
	initialMessage.AddString("space", "\t");
	initialMessage.AddString("space", "\v");
	initialMessage.AddString("should check the same in names", "but this is enough");
	initialMessage.AddInt32("int+", 42);
	initialMessage.AddInt32("int-", -54);
	initialMessage.AddBool("Yay", true);
	initialMessage.AddBool("Nay", false);

	BMessage submessage;
	submessage.AddString("name", "with spaces");
	submessage.AddString("close group mark", "}");
	submessage.AddString("a", "value\n");
	submessage.AddString("b", "value\\");
	submessage.AddString("c", "value\n1");
	initialMessage.AddMessage("message_no_subparent", &submessage);
	initialMessage.AddMessage("message_subparent", &submessage);

	submessage.MakeEmpty();
	initialMessage.AddMessage("empty_message_no_subparent", &submessage);
	initialMessage.AddMessage("empty_message_subparent", &submessage);

	submessage.AddString("name", "aName");
	initialMessage.AddMessage("message_subparent", &submessage);

	BString settingsBuffer;
	DriverSettingsMessageAdapter adapter;
	CPPUNIT_ASSERT_EQUAL(B_OK,
		adapter.ConvertToDriverSettings(kTemplate, settingsBuffer, initialMessage));

	Settings settings(settingsBuffer.String());
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, settings.ToMessage(kTemplate, message));

	AssertMessagesEqual(initialMessage, message);
}


void
DriverSettingsMessageAdapterTest::TestParent()
{
	const settings_template kSubTemplate[] = {
		{B_STRING_TYPE, "name", NULL, true},
		{B_BOOL_TYPE, "bool", NULL},
		{}
	};
	const settings_template kTemplate[] = {
		{B_MESSAGE_TYPE, "message", kSubTemplate},
		{}
	};
	Settings settingsA("message first {\n"
		"    bool\n"
		"}\n");
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, settingsA.ToMessage(kTemplate, message));
	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("message", &subMessage));
	CPPUNIT_ASSERT_EQUAL_MESSAGE("bool", true, subMessage.GetBool("bool"));
	CPPUNIT_ASSERT_EQUAL(BString("first"),
		BString(subMessage.GetString("name")));

	Settings settingsB("message second\n");
	CPPUNIT_ASSERT_EQUAL(B_OK, settingsB.ToMessage(kTemplate, message));
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("message", &subMessage));
	CPPUNIT_ASSERT_EQUAL(false, subMessage.HasBool("bool"));
	CPPUNIT_ASSERT_EQUAL(BString("second"),
		BString(subMessage.GetString("name", "-/-")));

	const settings_template kSubSubTemplateC[] = {
		{B_STRING_TYPE, "subname", NULL, true},
		{B_BOOL_TYPE, "bool", NULL},
		{}
	};
	const settings_template kSubTemplateC[] = {
		{B_STRING_TYPE, "name", NULL, true},
		{B_MESSAGE_TYPE, "sub", kSubSubTemplateC},
		{}
	};
	const settings_template kTemplateC[] = {
		{B_MESSAGE_TYPE, "message", kSubTemplateC},
		{}
	};

	Settings settingsC("message other {\n"
		"    sub third {\n"
		"        hun audo\n"
		"    }\n"
		"    sub fourth\n"
		"}");
	CPPUNIT_ASSERT_EQUAL(B_OK, settingsC.ToMessage(kTemplateC, message));
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("message", &subMessage));
	CPPUNIT_ASSERT_EQUAL(false, subMessage.HasBool("bool"));
	CPPUNIT_ASSERT_EQUAL(BString("other"),
		BString(subMessage.GetString("name", "-/-")));
}


void
DriverSettingsMessageAdapterTest::TestConverter()
{
	class HexConverter : public DriverSettingsConverter {
	public:
		status_t ConvertFromDriverSettings(const driver_parameter& parameter,
			const char* name, int32 index, uint32 type, BMessage& target)
		{
			const char* value = parameter.values[index];
			if (value[0] == '0' && value[1] == 'x')
				return target.AddInt32(name, (int32)strtol(value, NULL, 0));
			return B_NOT_SUPPORTED;
		}

		status_t ConvertToDriverSettings(const BMessage& source,
			const char* name, int32 index, uint32 type, BString& value)
		{
			int32 intValue;
			if (index == 0 && source.FindInt32(name, 0, &intValue) == B_OK) {
				BString string;
				string.SetToFormat("0x%" B_PRIu32, intValue);
				value << string;

				return B_OK;
			}
			return B_NOT_SUPPORTED;
		}
	} converter;

	const settings_template kTemplate[] = {
		{B_INT32_TYPE, "test", NULL, false, &converter},
		{}
	};

	Settings settings("test 0x2a 43");
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, settings.ToMessage(kTemplate, message));
	CPPUNIT_ASSERT_EQUAL(42, message.GetInt32("test", 0, 0));
	CPPUNIT_ASSERT_EQUAL(43, message.GetInt32("test", 1, 0));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));
}


void
DriverSettingsMessageAdapterTest::TestWildcard()
{
	const settings_template kTemplateA[] = {
		{B_STRING_TYPE, NULL, NULL},
		{B_INT32_TYPE, "test", NULL},
		{}
	};

	Settings settingsA("this is\n"
		"just a\n"
		"test 42 43");
	BMessage message;
	CPPUNIT_ASSERT_EQUAL(B_OK, settingsA.ToMessage(kTemplateA, message));
	CPPUNIT_ASSERT_EQUAL(42, message.GetInt32("test", 0, 0));
	CPPUNIT_ASSERT_EQUAL(43, message.GetInt32("test", 1, 0));
	CPPUNIT_ASSERT_EQUAL(3, message.CountNames(B_ANY_TYPE));
	CPPUNIT_ASSERT_EQUAL(BString("is"),
		BString(message.GetString("this", "-")));
	CPPUNIT_ASSERT_EQUAL(BString("a"),
		BString(message.GetString("just", "-")));

	const settings_template kSubTemplateB[] = {
		{B_STRING_TYPE, NULL, NULL, true},
		{}
	};
	const settings_template kTemplateB[] = {
		{B_MESSAGE_TYPE, "it", kSubTemplateB},
		{}
	};

	Settings settingsB("it just works\n");
	CPPUNIT_ASSERT_EQUAL(B_OK, settingsB.ToMessage(kTemplateB, message));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));
	BMessage subMessage;
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("it", &subMessage));
	CPPUNIT_ASSERT_EQUAL(BString("just"),
		BString(subMessage.GetString("it", 0, "-")));
	CPPUNIT_ASSERT_EQUAL(BString("works"),
		BString(subMessage.GetString("it", 1, "-")));
	CPPUNIT_ASSERT_EQUAL(1, subMessage.CountNames(B_ANY_TYPE));

	Settings settingsC("it {\n"
		"\tstill works\n"
		"}\n");
	CPPUNIT_ASSERT_EQUAL(B_OK, settingsC.ToMessage(kTemplateB, message));
	CPPUNIT_ASSERT_EQUAL(1, message.CountNames(B_ANY_TYPE));
	CPPUNIT_ASSERT_EQUAL(B_OK, message.FindMessage("it", &subMessage));
	CPPUNIT_ASSERT_EQUAL(BString("works"),
		BString(subMessage.GetString("still", "-")));
	CPPUNIT_ASSERT_EQUAL(1, subMessage.CountNames(B_ANY_TYPE));
}


/*static*/ void
DriverSettingsMessageAdapterTest::AddTests(BTestSuite& parent)
{
	CppUnit::TestSuite& suite = *new CppUnit::TestSuite(
		"DriverSettingsMessageAdapterTest");

	suite.addTest(new CppUnit::TestCaller<DriverSettingsMessageAdapterTest>(
		"DriverSettingsMessageAdapterTest::TestPrimitivesToMessage",
		&DriverSettingsMessageAdapterTest::TestPrimitivesToMessage));
	suite.addTest(new CppUnit::TestCaller<DriverSettingsMessageAdapterTest>(
		"DriverSettingsMessageAdapterTest::TestMessage",
		&DriverSettingsMessageAdapterTest::TestMessage));
	suite.addTest(new CppUnit::TestCaller<DriverSettingsMessageAdapterTest>(
		"DriverSettingsMessageAdapterTest::TestFromMessage",
		&DriverSettingsMessageAdapterTest::TestFromMessage));
	suite.addTest(new CppUnit::TestCaller<DriverSettingsMessageAdapterTest>(
		"DriverSettingsMessageAdapterTest::TestParent",
		&DriverSettingsMessageAdapterTest::TestParent));
	suite.addTest(new CppUnit::TestCaller<DriverSettingsMessageAdapterTest>(
		"DriverSettingsMessageAdapterTest::TestConverter",
		&DriverSettingsMessageAdapterTest::TestConverter));
	suite.addTest(new CppUnit::TestCaller<DriverSettingsMessageAdapterTest>(
		"DriverSettingsMessageAdapterTest::TestWildcard",
		&DriverSettingsMessageAdapterTest::TestWildcard));

	parent.addTest("DriverSettingsMessageAdapterTest", &suite);
}
