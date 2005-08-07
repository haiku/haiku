//------------------------------------------------------------------------------
//	TMessageSpeedTest.cpp
//  Written on 04 - 13 - 2005 by Olivier Milla (methedras at online dot fr)
//------------------------------------------------------------------------------

// Standard Includes -----------------------------------------------------------
#include <iostream>
#include <time.h>

// System Includes -------------------------------------------------------------
#include <Entry.h>
#include <File.h>
#include <Message.h>
#include <String.h>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------
#include "MessageSpeedTest.h"

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------

//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestCreate5Int32()
{
	BMessage message;
	
	bigtime_t length = 0;
	
	for (int32 i=0; i<5; i++){
		bigtime_t stamp = real_time_clock_usecs();
		message.AddInt32("data",i);
		length += (real_time_clock_usecs() - stamp);	
	}
	cout << "TMessageSpeedTest::Time to add 5 int32 in a message = " 
		 << length << "usec" << endl;	
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestLookup5Int32()
{
	srand(time(NULL));
	
	BMessage message;
	
	for (int32 i=0; i<5; i++){
		BString string;
		string << i;
		message.AddInt32(string.String(),i);
	}
	BString search;
	search << rand()%5;
	const char *string = search.String();
	int32 res;
	
	bigtime_t stamp = real_time_clock_usecs();
	message.FindInt32(string,&res);
	bigtime_t length = real_time_clock_usecs() - stamp;	
	cout << "TMessageSpeedTest::Time to find a data in a message containing 5 datas = " 
		 << length << "usec" << endl;
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestLookup50Int32()
{
	srand(time(NULL));
	
	BMessage message;
	
	for (int32 i=0; i<50; i++){
		BString string;
		string << i;
		message.AddInt32(string.String(),i);
	}
	BString search;
	search << rand()%50;
	const char *string = search.String();
	int32 res;
	
	bigtime_t stamp = real_time_clock_usecs();
	message.FindInt32(string,&res);
	bigtime_t length = real_time_clock_usecs() - stamp;	
	cout << "TMessageSpeedTest::Time to find a data in a message containing 50 datas = " 
		 << length << "usec" << endl;
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestLookup500Int32()
{
	srand(time(NULL));
	
	BMessage message;
	
	for (int32 i=0; i<500; i++){
		BString string;
		string << i;
		message.AddInt32(string.String(),i);
	}
	BString search;
	search << rand()%500;
	const char *string = search.String();
	int32 res;
	
	bigtime_t stamp = real_time_clock_usecs();
	message.FindInt32(string,&res);
	bigtime_t length = real_time_clock_usecs() - stamp;	
	cout << "TMessageSpeedTest::Time to find a data in a message containing 500 datas = " 
		 << length << "usec" << endl;
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestLookup5000Int32()
{
	srand(time(NULL));
	
	BMessage message;
	
	for (int32 i=0; i<5000; i++){
		BString string;
		string << i;
		message.AddInt32(string.String(),i);
	}
	BString search;
	search << rand()%5000;
	const char *string = search.String();
	int32 res;
	
	bigtime_t stamp = real_time_clock_usecs();
	message.FindInt32(string,&res);
	bigtime_t length = real_time_clock_usecs() - stamp;	
	cout << "TMessageSpeedTest::Time to find a data in a message containing 5000 datas = " 
		 << length << "usec" << endl;
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestRead500Int32()
{
	srand(time(NULL));
	
	BMessage message;
	
	for (int32 i=0; i<500; i++)
		message.AddInt32("data",i);

	int32 res;	
	bigtime_t length = 0;
	for (int32 i=0; i<500; i++){
		bigtime_t stamp = real_time_clock_usecs();	
		message.FindInt32("data",i,&res);
		length += (real_time_clock_usecs() - stamp);	
	}
	cout << "TMessageSpeedTest::Time to retrieve 500 Int32 out of a message = " 
		 << length << "usec. Giving " << length/500 << "usec per retrieve" << endl;
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestFlatten5Int32()
{
	BMessage message;
	
	for (int32 i=0; i<5; i++)
		message.AddInt32("data",i);

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	bigtime_t stamp = real_time_clock_usecs();
	message.Flatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to flatten a message containing 5 Int32 = " 
		 << length << "usec. Giving " << length/5 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}

//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestFlatten50Int32()
{
	BMessage message;
	
	for (int32 i=0; i<50; i++)
		message.AddInt32("data",i);

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	bigtime_t stamp = real_time_clock_usecs();
	message.Flatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to flatten a message containing 50 Int32 = " 
		 << length << "usec. Giving " << length/50 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestFlatten500Int32()
{
	BMessage message;
	
	for (int32 i=0; i<500; i++)
		message.AddInt32("data",i);

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	bigtime_t stamp = real_time_clock_usecs();
	message.Flatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to flatten a message containing 500 Int32 = " 
		 << length << "usec. Giving " << length/500 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestFlatten5000Int32()
{
	BMessage message;
	
	for (int32 i=0; i<5000; i++)
		message.AddInt32("data",i);

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	bigtime_t stamp = real_time_clock_usecs();
	message.Flatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to flatten a message containing 5000 Int32 = " 
		 << length << "usec. Giving " << length/5000 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestFlatten5IndividualInt32()
{
	BMessage message;
	
	for (int32 i=0; i<5; i++) {
		BString name = "data";
		name << i;
		message.AddInt32(name.String(),i);
	}

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	bigtime_t stamp = real_time_clock_usecs();
	message.Flatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to flatten a message containing 5 individual Int32 fields = " 
		 << length << "usec. Giving " << length/5 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestFlatten50IndividualInt32()
{
	BMessage message;
	
	for (int32 i=0; i<50; i++) {
		BString name = "data";
		name << i;
		message.AddInt32(name.String(),i);
	}

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	bigtime_t stamp = real_time_clock_usecs();
	message.Flatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to flatten a message containing 50 individual Int32 fields = " 
		 << length << "usec. Giving " << length/50 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestFlatten500IndividualInt32()
{
	BMessage message;
	
	for (int32 i=0; i<500; i++) {
		BString name = "data";
		name << i;
		message.AddInt32(name.String(),i);
	}

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	bigtime_t stamp = real_time_clock_usecs();
	message.Flatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to flatten a message containing 500 individual Int32 fields = " 
		 << length << "usec. Giving " << length/500 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestFlatten5000IndividualInt32()
{
	BMessage message;
	
	for (int32 i=0; i<5000; i++) {
		BString name = "data";
		name << i;
		message.AddInt32(name.String(),i);
	}

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	bigtime_t stamp = real_time_clock_usecs();
	message.Flatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to flatten a message containing 5000 individual Int32 fields = " 
		 << length << "usec. Giving " << length/5000 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestUnflatten5Int32()
{
	BMessage message;
	
	for (int32 i=0; i<5; i++)
		message.AddInt32("data",i);

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	message.Flatten(&file);

	BMessage rebuilt;
	bigtime_t stamp = real_time_clock_usecs();
	rebuilt.Unflatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to unflatten a message containing 5 Int32 = " 
		 << length << "usec. Giving " << length/5 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestUnflatten50Int32()
{
	BMessage message;
	
	for (int32 i=0; i<50; i++)
		message.AddInt32("data",i);

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	message.Flatten(&file);

	BMessage rebuilt;
	bigtime_t stamp = real_time_clock_usecs();
	rebuilt.Unflatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to unflatten a message containing 50 Int32 = " 
		 << length << "usec. Giving " << length/50 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestUnflatten500Int32()
{
	BMessage message;
	
	for (int32 i=0; i<500; i++)
		message.AddInt32("data",i);

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	message.Flatten(&file);

	BMessage rebuilt;
	bigtime_t stamp = real_time_clock_usecs();
	rebuilt.Unflatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to unflatten a message containing 500 Int32 = " 
		 << length << "usec. Giving " << length/500 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestUnflatten5000Int32()
{
	BMessage message;
	
	for (int32 i=0; i<5000; i++)
		message.AddInt32("data",i);

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	message.Flatten(&file);

	BMessage rebuilt;
	bigtime_t stamp = real_time_clock_usecs();
	rebuilt.Unflatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to unflatten a message containing 5000 Int32 = " 
		 << length << "usec. Giving " << length/5000 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestUnflatten5IndividualInt32()
{
	BMessage message;
	
	for (int32 i=0; i<5; i++) {
		BString name = "data";
		name << i;
		message.AddInt32(name.String(),i);
	}

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	message.Flatten(&file);

	BMessage rebuilt;
	bigtime_t stamp = real_time_clock_usecs();
	rebuilt.Unflatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to unflatten a message containing 5 individual Int32 fields = " 
		 << length << "usec. Giving " << length/50 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestUnflatten50IndividualInt32()
{
	BMessage message;
	
	for (int32 i=0; i<50; i++) {
		BString name = "data";
		name << i;
		message.AddInt32(name.String(),i);
	}

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	message.Flatten(&file);

	BMessage rebuilt;
	bigtime_t stamp = real_time_clock_usecs();
	rebuilt.Unflatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to unflatten a message containing 50 individual Int32 fields = " 
		 << length << "usec. Giving " << length/50 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestUnflatten500IndividualInt32()
{
	BMessage message;
	
	for (int32 i=0; i<500; i++) {
		BString name = "data";
		name << i;
		message.AddInt32(name.String(),i);
	}

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	message.Flatten(&file);

	BMessage rebuilt;
	bigtime_t stamp = real_time_clock_usecs();
	rebuilt.Unflatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to unflatten a message containing 500 individual Int32 fields = " 
		 << length << "usec. Giving " << length/500 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
void TMessageSpeedTest::MessageSpeedTestUnflatten5000IndividualInt32()
{
	BMessage message;
	
	for (int32 i=0; i<5000; i++) {
		BString name = "data";
		name << i;
		message.AddInt32(name.String(),i);
	}

	BEntry entry("/tmp/bmessagetest");
	BFile file(&entry,B_READ_WRITE|B_CREATE_FILE);
	message.Flatten(&file);

	BMessage rebuilt;
	bigtime_t stamp = real_time_clock_usecs();
	rebuilt.Unflatten(&file);
	bigtime_t length = real_time_clock_usecs() - stamp;
	cout << "TMessageSpeedTest::Time to unflatten a message containing 5000 individual Int32 fields = " 
		 << length << "usec. Giving " << length/5000 << "usec per data" << endl;
	
	//delete the file
	file.Unset();
	entry.Remove();
}
//------------------------------------------------------------------------------
TestSuite* TMessageSpeedTest::Suite()
{
	TestSuite* suite = new TestSuite("BMessage::Test of Performance");

	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestCreate5Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestLookup5Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestLookup50Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestLookup500Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestLookup5000Int32);
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestRead500Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten5Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten50Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten500Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten5000Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten5IndividualInt32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten50IndividualInt32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten500IndividualInt32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestFlatten5000IndividualInt32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten5Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten50Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten500Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten5000Int32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten5IndividualInt32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten50IndividualInt32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten500IndividualInt32);	
	ADD_TEST4(BMessage, suite, TMessageSpeedTest, MessageSpeedTestUnflatten5000IndividualInt32);	

	return suite;
}
//------------------------------------------------------------------------------


/*
 * $Log $
 *
 * $Id  $
 *
 */

