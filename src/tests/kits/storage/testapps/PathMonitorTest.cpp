/*
 * Copyright 2007, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Axel Dörfler, axeld@pinc-software.de
 */


#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <Looper.h>
#include <Path.h>
#include <PathMonitor.h>

#include <stdio.h>
#include <string.h>

using BPrivate::BPathMonitor;


class Looper : public BLooper {
	public:
		Looper(const char* path);
		virtual ~Looper();

		virtual void MessageReceived(BMessage* message);

	private:
		BPath	fPath;
};


Looper::Looper(const char* path)
	: BLooper("path monitor test"),
	fPath(path)
{
	status_t status = BPathMonitor::StartWatching(path, B_WATCH_ALL, this);
	if (status != B_OK) {
		fprintf(stderr, "Could not watch path \"%s\": %s.\n", path, strerror(status));
		PostMessage(B_QUIT_REQUESTED);
	}
}


Looper::~Looper()
{
	status_t status = BPathMonitor::StopWatching(fPath.Path(), this);
	if (status != B_OK)
		fprintf(stderr, "Could not stop watching: %s\n", strerror(status));
}


void
Looper::MessageReceived(BMessage* message)
{
	if (message->what == B_PATH_MONITOR)
		message->PrintToStream();
}


//	#pragma mark -


void
create_file(const char* path)
{
	printf("******* create file %s *******\n", path);
	BFile file;
	status_t status = file.SetTo(path, B_CREATE_FILE | B_READ_WRITE);
	if (status != B_OK)
		fprintf(stderr, "could not create watch test.\n");
	file.Write("test", 4);
}


void
remove_file(const char* path)
{
	printf("******* remove file %s *******\n", path);
	remove(path);
}


void
create_directory(const char* path)
{
	printf("******* create directory %s *******\n", path);
	create_directory(path, 0755);
}


void
remove_directory(const char* path)
{
	printf("******* remove directory %s *******\n", path);
	rmdir(path);
}


//	#pragma mark -


void
test_a()
{
	puts("******************* test A ********************");

	create_directory("/tmp/a");
	create_directory("/tmp/ab");
	create_directory("/tmp/a/b");
	create_directory("/tmp/a/bc");
	create_directory("/tmp/a/b/c");
	create_directory("/tmp/a/b/cd");

	create_file("/tmp/a/b/c/d");
	snooze(100000);
	remove_file("/tmp/a/b/c/d");

	remove_directory("/tmp/a/b/cd");
	remove_directory("/tmp/a/bc");
	remove_directory("/tmp/ab");

	snooze(100000);
}


void
test_b()
{
	puts("******************* test B ********************");
	remove_directory("/tmp/a/b/c");
	snooze(100000);
	create_file("/tmp/a/b/c");
	snooze(100000);
	remove_file("/tmp/a/b/c");
}


void
test_c()
{
	puts("******************* test C ********************");
	create_directory("/tmp/a/b/c");
	create_directory("/tmp/a/b/c/d");
	snooze(100000);
	remove_directory("/tmp/a/b/c/d");
	remove_directory("/tmp/a/b/c");
}


void
test_d()
{
	puts("******************* test D ********************");
	remove_directory("/tmp/a/b");
	remove_directory("/tmp/a");
}


int
main()
{
	// start looper
	Looper& looper = *new Looper("/tmp/a/b/c");
	looper.Run();

	// run tests
	test_a();
	test_b();
	test_c();
	test_d();

#if 0
	char b[2100];
	gets(b);
#endif

	snooze(500000LL);

	// quit looper
	looper.PostMessage(B_QUIT_REQUESTED);
	status_t status;
	wait_for_thread(looper.Thread(), &status);

	return 0;
}
