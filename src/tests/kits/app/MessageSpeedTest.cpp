/*
 * Copyright 2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 *		Olivier Milla <methedras at online dot fr>
 */


#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <String.h>

#include <TestSuiteAddon.h>
#include <cppunit/TestFixture.h>
#include <cppunit/extensions/HelperMacros.h>


// #define LOG_TO_FILE
#ifdef LOG_TO_FILE
#define LOG(function, time) \
	{ \
		FILE* logfile = fopen("/boot/home/Desktop/messagespeed.log", "a"); \
		fprintf(logfile, "%s:\t%lld\n", function, time); \
		fclose(logfile); \
	}
#else
#define LOG(function, time) /* empty */
#endif

#define MESSAGE_SPEED_TEST_LOOKUP(count, type) \
	void MessageSpeedTestLookup##count##type() \
	{ \
		srand(time(NULL)); \
		BMessage message; \
\
		for (int32 i = 0; i < count; i++) { \
			BString string; \
			string << i; \
			message.AddInt32(string.String(), i); \
		} \
\
		BString search; \
		search << rand() % count; \
		const char* string = search.String(); \
		int32 res; \
\
		bigtime_t stamp = real_time_clock_usecs(); \
		message.FindInt32(string, &res); \
		bigtime_t length = real_time_clock_usecs() - stamp; \
		std::cout << "Time to find a data in a message containing " << count \
				  << " items = " << length << "usec" << std::endl; \
		LOG(__PRETTY_FUNCTION__, length); \
	}


#define MESSAGE_SPEED_TEST_CREATE(count, type, typeName, createValue) \
	void MessageSpeedTestCreate##count##type() \
	{ \
		BMessage message; \
		bigtime_t length = 0; \
\
		for (int32 i = 0; i < count; i++) { \
			createValue; \
			bigtime_t stamp = real_time_clock_usecs(); \
			message.Add##type("data", value); \
			length += (real_time_clock_usecs() - stamp); \
		} \
\
		std::cout << "Time to add " << count << " " << typeName \
				  << " in a message = " << length << "usec" << std::endl; \
		LOG(__PRETTY_FUNCTION__, length); \
	}


#define MESSAGE_SPEED_TEST_READ(count, type, typeName, createValue, \
								declareValue) \
	void MessageSpeedTestRead##count##type() \
	{ \
		srand(time(NULL)); \
		BMessage message; \
\
		for (int32 i = 0; i < count; i++) { \
			createValue; \
			message.Add##type("data", value); \
		} \
\
		declareValue; \
		bigtime_t length = 0; \
		for (int32 i = 0; i < count; i++) { \
			bigtime_t stamp = real_time_clock_usecs(); \
			message.Find##type("data", i, &value); \
			length += (real_time_clock_usecs() - stamp); \
		} \
\
		std::cout << "Time to retrieve " << count << " " << typeName \
				  << " out of a message = " << length << "usec. Giving " \
				  << length / count << "usec per retrieve." << std::endl; \
		LOG(__PRETTY_FUNCTION__, length); \
	}

#define MESSAGE_SPEED_TEST_FLATTEN(count, type, typeName, createValue) \
	void MessageSpeedTestFlatten##count##type() \
	{ \
		BMessage message; \
\
		for (int32 i = 0; i < count; i++) { \
			createValue; \
			message.Add##type("data", value); \
		} \
\
		BMallocIO buffer; \
		bigtime_t stamp = real_time_clock_usecs(); \
		message.Flatten(&buffer); \
		bigtime_t length = real_time_clock_usecs() - stamp; \
\
		BString name = "/tmp/MessageSpeedTestFlatten"; \
		name << count << typeName; \
		BEntry entry(name.String()); \
		BFile file(&entry, B_READ_WRITE | B_CREATE_FILE); \
		file.Write(buffer.Buffer(), buffer.BufferLength()); \
\
		std::cout << "Time to flatten a message containing " << count << " " \
				  << typeName << " = " << length << "usec. Giving " \
				  << length / count << "usec per item." << std::endl; \
		LOG(__PRETTY_FUNCTION__, length); \
	}


#define MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(count, type, typeName, \
											  createValue) \
	void MessageSpeedTestFlattenIndividual##count##type() \
	{ \
		BMessage message; \
\
		for (int32 i = 0; i < count; i++) { \
			createValue; \
			BString name = "data"; \
			name << i; \
			message.Add##type(name.String(), value); \
		} \
\
		BMallocIO buffer; \
		bigtime_t stamp = real_time_clock_usecs(); \
		message.Flatten(&buffer); \
		bigtime_t length = real_time_clock_usecs() - stamp; \
\
		BString name = "/tmp/MessageSpeedTestFlattenIndividual"; \
		name << count << typeName; \
		BEntry entry(name.String()); \
		BFile file(&entry, B_READ_WRITE | B_CREATE_FILE); \
		file.Write(buffer.Buffer(), buffer.BufferLength()); \
\
		std::cout << "Time to flatten a message containing " << count \
				  << " individual " << typeName << " fields = " << length \
				  << "usec. Giving " << length / count << "usec per item." \
				  << std::endl; \
		LOG(__PRETTY_FUNCTION__, length); \
	}


#define MESSAGE_SPEED_TEST_UNFLATTEN(count, type, typeName) \
	void MessageSpeedTestUnflatten##count##type() \
	{ \
		BString name = "/tmp/MessageSpeedTestFlatten"; \
		name << count << typeName; \
		BEntry entry(name.String()); \
		BFile file(&entry, B_READ_ONLY); \
\
		off_t size = 0; \
		file.GetSize(&size); \
		char* buffer = (char*)malloc(size); \
		file.Read(buffer, size); \
		BMemoryIO stream(buffer, size); \
\
		BMessage rebuilt; \
		bigtime_t stamp = real_time_clock_usecs(); \
		rebuilt.Unflatten(&stream); \
		bigtime_t length = real_time_clock_usecs() - stamp; \
\
		std::cout << "Time to unflatten a message containing " << count << " " \
				  << typeName << " = " << length << "usec. Giving " \
				  << length / count << "usec per item." << std::endl; \
		LOG(__PRETTY_FUNCTION__, length); \
\
		file.Unset(); \
		entry.Remove(); \
	}


#define MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(count, type, typeName) \
	void MessageSpeedTestUnflattenIndividual##count##type() \
	{ \
		BString name = "/tmp/MessageSpeedTestFlattenIndividual"; \
		name << count << typeName; \
		BEntry entry(name.String()); \
		BFile file(&entry, B_READ_ONLY); \
\
		off_t size = 0; \
		file.GetSize(&size); \
		char* buffer = (char*)malloc(size); \
		file.Read(buffer, size); \
		BMemoryIO stream(buffer, size); \
\
		BMessage rebuilt; \
		bigtime_t stamp = real_time_clock_usecs(); \
		rebuilt.Unflatten(&stream); \
		bigtime_t length = real_time_clock_usecs() - stamp; \
\
		std::cout << "Time to unflatten a message containing " << count \
				  << " individual " << typeName << " fields = " << length \
				  << "usec. Giving " << length / count << "usec per item." \
				  << std::endl; \
		LOG(__PRETTY_FUNCTION__, length); \
\
		file.Unset(); \
		entry.Remove(); \
	}


class MessageSpeedTest : public CppUnit::TestFixture {
	CPPUNIT_TEST_SUITE(MessageSpeedTest);
	CPPUNIT_TEST(MessageSpeedTestCreate5Int32);
	CPPUNIT_TEST(MessageSpeedTestCreate50Int32);
	CPPUNIT_TEST(MessageSpeedTestCreate500Int32);
	CPPUNIT_TEST(MessageSpeedTestCreate5000Int32);
	CPPUNIT_TEST(MessageSpeedTestCreate5String);
	CPPUNIT_TEST(MessageSpeedTestCreate50String);
	CPPUNIT_TEST(MessageSpeedTestCreate500String);
	CPPUNIT_TEST(MessageSpeedTestCreate5000String);
	CPPUNIT_TEST(MessageSpeedTestLookup5Int32);
	CPPUNIT_TEST(MessageSpeedTestLookup50Int32);
	CPPUNIT_TEST(MessageSpeedTestLookup500Int32);
	CPPUNIT_TEST(MessageSpeedTestLookup5000Int32);
	CPPUNIT_TEST(MessageSpeedTestRead5Int32);
	CPPUNIT_TEST(MessageSpeedTestRead50Int32);
	CPPUNIT_TEST(MessageSpeedTestRead500Int32);
	CPPUNIT_TEST(MessageSpeedTestRead5000Int32);
	CPPUNIT_TEST(MessageSpeedTestRead5String);
	CPPUNIT_TEST(MessageSpeedTestRead50String);
	CPPUNIT_TEST(MessageSpeedTestRead500String);
	CPPUNIT_TEST(MessageSpeedTestRead5000String);
	CPPUNIT_TEST(MessageSpeedTestFlatten5Int32);
	CPPUNIT_TEST(MessageSpeedTestFlatten50Int32);
	CPPUNIT_TEST(MessageSpeedTestFlatten500Int32);
	CPPUNIT_TEST(MessageSpeedTestFlatten5000Int32);
	CPPUNIT_TEST(MessageSpeedTestFlatten5String);
	CPPUNIT_TEST(MessageSpeedTestFlatten50String);
	CPPUNIT_TEST(MessageSpeedTestFlatten500String);
	CPPUNIT_TEST(MessageSpeedTestFlatten5000String);
	CPPUNIT_TEST(MessageSpeedTestFlattenIndividual5Int32);
	CPPUNIT_TEST(MessageSpeedTestFlattenIndividual50Int32);
	CPPUNIT_TEST(MessageSpeedTestFlattenIndividual500Int32);
	CPPUNIT_TEST(MessageSpeedTestFlattenIndividual5000Int32);
	CPPUNIT_TEST(MessageSpeedTestFlattenIndividual5String);
	CPPUNIT_TEST(MessageSpeedTestFlattenIndividual50String);
	CPPUNIT_TEST(MessageSpeedTestFlattenIndividual500String);
	CPPUNIT_TEST(MessageSpeedTestFlattenIndividual5000String);
	CPPUNIT_TEST(MessageSpeedTestUnflatten5Int32);
	CPPUNIT_TEST(MessageSpeedTestUnflatten50Int32);
	CPPUNIT_TEST(MessageSpeedTestUnflatten500Int32);
	CPPUNIT_TEST(MessageSpeedTestUnflatten5000Int32);
	CPPUNIT_TEST(MessageSpeedTestUnflatten5String);
	CPPUNIT_TEST(MessageSpeedTestUnflatten50String);
	CPPUNIT_TEST(MessageSpeedTestUnflatten500String);
	CPPUNIT_TEST(MessageSpeedTestUnflatten5000String);
	CPPUNIT_TEST(MessageSpeedTestUnflattenIndividual5Int32);
	CPPUNIT_TEST(MessageSpeedTestUnflattenIndividual50Int32);
	CPPUNIT_TEST(MessageSpeedTestUnflattenIndividual500Int32);
	CPPUNIT_TEST(MessageSpeedTestUnflattenIndividual5000Int32);
	CPPUNIT_TEST(MessageSpeedTestUnflattenIndividual5String);
	CPPUNIT_TEST(MessageSpeedTestUnflattenIndividual50String);
	CPPUNIT_TEST(MessageSpeedTestUnflattenIndividual500String);
	CPPUNIT_TEST(MessageSpeedTestUnflattenIndividual5000String);
	CPPUNIT_TEST_SUITE_END();

public:
	MESSAGE_SPEED_TEST_CREATE(5, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_CREATE(50, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_CREATE(500, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_CREATE(5000, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_CREATE(5, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_CREATE(50, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_CREATE(500, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_CREATE(5000, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_LOOKUP(5, Int32)
	MESSAGE_SPEED_TEST_LOOKUP(50, Int32)
	MESSAGE_SPEED_TEST_LOOKUP(500, Int32)
	MESSAGE_SPEED_TEST_LOOKUP(5000, Int32)
	MESSAGE_SPEED_TEST_READ(5, Int32, "int32", int32 value = i, int32 value)
	MESSAGE_SPEED_TEST_READ(50, Int32, "int32", int32 value = i, int32 value)
	MESSAGE_SPEED_TEST_READ(500, Int32, "int32", int32 value = i, int32 value)
	MESSAGE_SPEED_TEST_READ(5000, Int32, "int32", int32 value = i, int32 value)
	MESSAGE_SPEED_TEST_READ(5, String, "BString", BString value = "item"; value << i, BString value)
	MESSAGE_SPEED_TEST_READ(50, String, "BString", BString value = "item"; value << i, BString value)
	MESSAGE_SPEED_TEST_READ(500, String, "BString", BString value = "item"; value << i, BString value)
	MESSAGE_SPEED_TEST_READ(5000, String, "BString", BString value = "item"; value << i, BString value)
	MESSAGE_SPEED_TEST_FLATTEN(5, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_FLATTEN(50, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_FLATTEN(500, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_FLATTEN(5000, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_FLATTEN(5, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_FLATTEN(50, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_FLATTEN(500, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_FLATTEN(5000, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(5, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(50, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(500, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(5000, Int32, "int32", int32 value = i)
	MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(5, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(50, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(500, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(5000, String, "BString", BString value = "item"; value << i)
	MESSAGE_SPEED_TEST_UNFLATTEN(5, Int32, "int32")
	MESSAGE_SPEED_TEST_UNFLATTEN(50, Int32, "int32")
	MESSAGE_SPEED_TEST_UNFLATTEN(500, Int32, "int32")
	MESSAGE_SPEED_TEST_UNFLATTEN(5000, Int32, "int32")
	MESSAGE_SPEED_TEST_UNFLATTEN(5, String, "BString")
	MESSAGE_SPEED_TEST_UNFLATTEN(50, String, "BString")
	MESSAGE_SPEED_TEST_UNFLATTEN(500, String, "BString")
	MESSAGE_SPEED_TEST_UNFLATTEN(5000, String, "BString")
	MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(5, Int32, "int32")
	MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(50, Int32, "int32")
	MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(500, Int32, "int32")
	MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(5000, Int32, "int32")
	MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(5, String, "BString")
	MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(50, String, "BString")
	MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(500, String, "BString")
	MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(5000, String, "BString")
};


CPPUNIT_TEST_SUITE_NAMED_REGISTRATION(MessageSpeedTest, getTestSuiteName());
