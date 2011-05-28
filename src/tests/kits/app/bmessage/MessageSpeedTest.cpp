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

#include "MessageSpeedTest.h"


using namespace std;


#define LOG_TO_FILE
#ifdef LOG_TO_FILE
#define LOG(function, time)													\
	{																		\
		FILE *logfile = fopen("/boot/home/Desktop/messagespeed.log", "a");	\
		fprintf(logfile, "%s:\t%lld\n", function, time);					\
		fclose(logfile);													\
	}
#else
#define LOG(function, time) /* empty */
#endif


#define MESSAGE_SPEED_TEST_CREATE(count, type, typeName, createValue)		\
void																		\
TMessageSpeedTest::MessageSpeedTestCreate##count##type()					\
{																			\
	BMessage message;														\
	bigtime_t length = 0;													\
																			\
	for (int32 i = 0; i < count; i++) {										\
		createValue;														\
		bigtime_t stamp = real_time_clock_usecs();							\
		message.Add##type("data", value);									\
		length += (real_time_clock_usecs() - stamp);						\
	}																		\
																			\
	cout << "Time to add " << count << " " << typeName						\
		<< " in a message = " << length << "usec" << endl;					\
	LOG(__PRETTY_FUNCTION__, length);										\
}

MESSAGE_SPEED_TEST_CREATE(5, Int32, "int32", int32 value = i);
MESSAGE_SPEED_TEST_CREATE(50, Int32, "int32", int32 value = i);
MESSAGE_SPEED_TEST_CREATE(500, Int32, "int32", int32 value = i);
MESSAGE_SPEED_TEST_CREATE(5000, Int32, "int32", int32 value = i);

MESSAGE_SPEED_TEST_CREATE(5, String, "BString", BString value = "item"; value << i);
MESSAGE_SPEED_TEST_CREATE(50, String, "BString", BString value = "item"; value << i);
MESSAGE_SPEED_TEST_CREATE(500, String, "BString", BString value = "item"; value << i);
MESSAGE_SPEED_TEST_CREATE(5000, String, "BString", BString value = "item"; value << i);

#undef MESSAGE_SPEED_TEST_CREATE


#define MESSAGE_SPEED_TEST_LOOKUP(count, type)								\
void																		\
TMessageSpeedTest::MessageSpeedTestLookup##count##type()					\
{																			\
	srand(time(NULL));														\
	BMessage message;														\
																			\
	for (int32 i = 0; i < count; i++) {										\
		BString string;														\
		string << i;														\
		message.AddInt32(string.String(), i);								\
	}																		\
																			\
	BString search;															\
	search << rand() % count;												\
	const char *string = search.String();									\
	int32 res;																\
																			\
	bigtime_t stamp = real_time_clock_usecs();								\
	message.FindInt32(string, &res);										\
	bigtime_t length = real_time_clock_usecs() - stamp;						\
	cout << "Time to find a data in a message containing " << count			\
		<< " items = " << length << "usec" << endl;							\
	LOG(__PRETTY_FUNCTION__, length);										\
}

MESSAGE_SPEED_TEST_LOOKUP(5, Int32);
MESSAGE_SPEED_TEST_LOOKUP(50, Int32);
MESSAGE_SPEED_TEST_LOOKUP(500, Int32);
MESSAGE_SPEED_TEST_LOOKUP(5000, Int32);

#undef MESSAGE_SPEED_TEST_LOOKUP


#define MESSAGE_SPEED_TEST_READ(count, type, typeName, createValue, declareValue)	\
void																		\
TMessageSpeedTest::MessageSpeedTestRead##count##type()						\
{																			\
	srand(time(NULL));														\
	BMessage message;														\
																			\
	for (int32 i = 0; i < count; i++) {										\
		createValue;														\
		message.Add##type("data", value);									\
	}																		\
																			\
	declareValue;															\
	bigtime_t length = 0;													\
	for (int32 i = 0; i < count; i++) {										\
		bigtime_t stamp = real_time_clock_usecs();							\
		message.Find##type("data", i, &value);								\
		length += (real_time_clock_usecs() - stamp);						\
	}																		\
																			\
	cout << "Time to retrieve " << count << " " << typeName					\
		<< " out of a message = " << length << "usec. Giving "				\
		<< length / count << "usec per retrieve." << endl;					\
	LOG(__PRETTY_FUNCTION__, length);										\
}

MESSAGE_SPEED_TEST_READ(5, Int32, "int32", int32 value = i, int32 value);
MESSAGE_SPEED_TEST_READ(50, Int32, "int32", int32 value = i, int32 value);
MESSAGE_SPEED_TEST_READ(500, Int32, "int32", int32 value = i, int32 value);
MESSAGE_SPEED_TEST_READ(5000, Int32, "int32", int32 value = i, int32 value);

MESSAGE_SPEED_TEST_READ(5, String, "BString", BString value = "item"; value << i, BString value);
MESSAGE_SPEED_TEST_READ(50, String, "BString", BString value = "item"; value << i, BString value);
MESSAGE_SPEED_TEST_READ(500, String, "BString", BString value = "item"; value << i, BString value);
MESSAGE_SPEED_TEST_READ(5000, String, "BString", BString value = "item"; value << i, BString value);

#undef MESSAGE_SPEED_TEST_READ


#define MESSAGE_SPEED_TEST_FLATTEN(count, type, typeName, createValue)		\
void																		\
TMessageSpeedTest::MessageSpeedTestFlatten##count##type()					\
{																			\
	BMessage message;														\
																			\
	for (int32 i = 0; i < count; i++) { 									\
		createValue;														\
		message.Add##type("data", value);									\
	}																		\
																			\
	BMallocIO buffer;														\
	bigtime_t stamp = real_time_clock_usecs();								\
	message.Flatten(&buffer);												\
	bigtime_t length = real_time_clock_usecs() - stamp;						\
																			\
	BString name = "/tmp/MessageSpeedTestFlatten";							\
	name << count << typeName;												\
	BEntry entry(name.String());											\
	BFile file(&entry, B_READ_WRITE | B_CREATE_FILE);						\
	file.Write(buffer.Buffer(), buffer.BufferLength());						\
																			\
	cout << "Time to flatten a message containing " << count << " "			\
		<< typeName << " = " << length << "usec. Giving " << length / count	\
		<< "usec per item." << endl;										\
	LOG(__PRETTY_FUNCTION__, length);										\
}

MESSAGE_SPEED_TEST_FLATTEN(5, Int32, "int32", int32 value = i);
MESSAGE_SPEED_TEST_FLATTEN(50, Int32, "int32", int32 value = i);
MESSAGE_SPEED_TEST_FLATTEN(500, Int32, "int32", int32 value = i);
MESSAGE_SPEED_TEST_FLATTEN(5000, Int32, "int32", int32 value = i);

MESSAGE_SPEED_TEST_FLATTEN(5, String, "BString", BString value = "item"; value << i);
MESSAGE_SPEED_TEST_FLATTEN(50, String, "BString", BString value = "item"; value << i);
MESSAGE_SPEED_TEST_FLATTEN(500, String, "BString", BString value = "item"; value << i);
MESSAGE_SPEED_TEST_FLATTEN(5000, String, "BString", BString value = "item"; value << i);

#undef MESSAGE_SPEED_TEST_FLATTEN


#define MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(count, type, typeName, createValue)	\
void																		\
TMessageSpeedTest::MessageSpeedTestFlattenIndividual##count##type()			\
{																			\
	BMessage message;														\
																			\
	for (int32 i = 0; i < count; i++) {										\
		createValue;														\
		BString name = "data";												\
		name << i;															\
		message.Add##type(name.String(), value);							\
	}																		\
																			\
	BMallocIO buffer;														\
	bigtime_t stamp = real_time_clock_usecs();								\
	message.Flatten(&buffer);												\
	bigtime_t length = real_time_clock_usecs() - stamp;						\
																			\
	BString name = "/tmp/MessageSpeedTestFlattenIndividual";				\
	name << count << typeName;												\
	BEntry entry(name.String());											\
	BFile file(&entry, B_READ_WRITE | B_CREATE_FILE);						\
	file.Write(buffer.Buffer(), buffer.BufferLength());						\
																			\
	cout << "Time to flatten a message containing " << count				\
		<< " individual " << typeName << " fields = " << length				\
		<< "usec. Giving " << length / count << "usec per item." << endl;	\
	LOG(__PRETTY_FUNCTION__, length);										\
}

MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(5, Int32, "int32", int32 value = i);
MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(50, Int32, "int32", int32 value = i);
MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(500, Int32, "int32", int32 value = i);
MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(5000, Int32, "int32", int32 value = i);

MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(5, String, "BString", BString value = "item"; value << i);
MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(50, String, "BString", BString value = "item"; value << i);
MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(500, String, "BString", BString value = "item"; value << i);
MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL(5000, String, "BString", BString value = "item"; value << i);

#undef MESSAGE_SPEED_TEST_FLATTEN_INDIVIDUAL


#define MESSAGE_SPEED_TEST_UNFLATTEN(count, type, typeName)					\
void																		\
TMessageSpeedTest::MessageSpeedTestUnflatten##count##type()					\
{																			\
	BString name = "/tmp/MessageSpeedTestFlatten";							\
	name << count << typeName;												\
	BEntry entry(name.String());											\
	BFile file(&entry, B_READ_ONLY);										\
																			\
	off_t size = 0;															\
	file.GetSize(&size);													\
	char *buffer = (char *)malloc(size);									\
	file.Read(buffer, size);												\
	BMemoryIO stream(buffer, size);											\
																			\
	BMessage rebuilt;														\
	bigtime_t stamp = real_time_clock_usecs();								\
	rebuilt.Unflatten(&stream);												\
	bigtime_t length = real_time_clock_usecs() - stamp;						\
																			\
	cout << "Time to unflatten a message containing " << count << " "		\
		<< typeName	<< " = " << length << "usec. Giving " << length / count	\
		<< "usec per item." << endl;										\
	LOG(__PRETTY_FUNCTION__, length);										\
																			\
	file.Unset();															\
	entry.Remove();															\
}

MESSAGE_SPEED_TEST_UNFLATTEN(5, Int32, "int32");
MESSAGE_SPEED_TEST_UNFLATTEN(50, Int32, "int32");
MESSAGE_SPEED_TEST_UNFLATTEN(500, Int32, "int32");
MESSAGE_SPEED_TEST_UNFLATTEN(5000, Int32, "int32");

MESSAGE_SPEED_TEST_UNFLATTEN(5, String, "BString");
MESSAGE_SPEED_TEST_UNFLATTEN(50, String, "BString");
MESSAGE_SPEED_TEST_UNFLATTEN(500, String, "BString");
MESSAGE_SPEED_TEST_UNFLATTEN(5000, String, "BString");

#undef MESSAGE_SPEED_TEST_UNFLATTEN


#define MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(count, type, typeName)		\
void																		\
TMessageSpeedTest::MessageSpeedTestUnflattenIndividual##count##type()		\
{																			\
	BString name = "/tmp/MessageSpeedTestFlattenIndividual";				\
	name << count << typeName;												\
	BEntry entry(name.String());											\
	BFile file(&entry, B_READ_ONLY);										\
																			\
	off_t size = 0;															\
	file.GetSize(&size);													\
	char *buffer = (char *)malloc(size);									\
	file.Read(buffer, size);												\
	BMemoryIO stream(buffer, size);											\
																			\
	BMessage rebuilt;														\
	bigtime_t stamp = real_time_clock_usecs();								\
	rebuilt.Unflatten(&stream);												\
	bigtime_t length = real_time_clock_usecs() - stamp;						\
																			\
	cout << "Time to unflatten a message containing " << count				\
		<< " individual " << typeName << " fields = " << length				\
		<< "usec. Giving " << length / count << "usec per item." << endl;	\
	LOG(__PRETTY_FUNCTION__, length);										\
																			\
	file.Unset();															\
	entry.Remove();															\
}

MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(5, Int32, "int32");
MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(50, Int32, "int32");
MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(500, Int32, "int32");
MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(5000, Int32, "int32");

MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(5, String, "BString");
MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(50, String, "BString");
MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(500, String, "BString");
MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL(5000, String, "BString");

#undef MESSAGE_SPEED_TEST_UNFLATTEN_INDIVIDUAL


TestSuite* TMessageSpeedTest::Suite()
{
	TestSuite* suite = new TestSuite("BMessage::Test of Performance");

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestCreate5Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestCreate50Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestCreate500Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestCreate5000Int32);

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestCreate5String);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestCreate50String);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestCreate500String);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestCreate5000String);

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestLookup5Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestLookup50Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestLookup500Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestLookup5000Int32);

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestRead5Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestRead50Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestRead500Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestRead5000Int32);	

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestRead5String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestRead50String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestRead500String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestRead5000String);	

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten5Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten50Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten500Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten5000Int32);	

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten5String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten50String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten500String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten5000String);	

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlattenIndividual5Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlattenIndividual50Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlattenIndividual500Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlattenIndividual5000Int32);	

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlattenIndividual5String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlattenIndividual50String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlattenIndividual500String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlattenIndividual5000String);	

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten5Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten50Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten500Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten5000Int32);	

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten5String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten50String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten500String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten5000String);	

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflattenIndividual5Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflattenIndividual50Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflattenIndividual500Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflattenIndividual5000Int32);	

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflattenIndividual5String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflattenIndividual50String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflattenIndividual500String);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflattenIndividual5000String);	

	return suite;
}
