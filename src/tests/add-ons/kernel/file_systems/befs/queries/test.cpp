#include <Message.h>
#include <Entry.h>
#include <File.h>
#include <NodeMonitor.h>
#include <kernel/fs_info.h>
#include <kernel/fs_query.h>

#include <stdio.h>
#include <string.h>

#ifndef B_BAD_DATA
#	define B_BAD_DATA B_ERROR
#endif

#define DUMPED_BLOCK_SIZE 16
#define Print printf

void
dumpBlock(const char *buffer,int size)
{
	for(int i = 0;i < size;) {
		int start = i;

		for(;i < start+DUMPED_BLOCK_SIZE;i++) {
			if (!(i % 4))
				Print(" ");

			if (i >= size)
				Print("  ");
			else
				Print("%02x",*(unsigned char *)(buffer+i));
		}
		Print("  ");

		for(i = start;i < start + DUMPED_BLOCK_SIZE;i++) {
			if (i < size) {
				char c = *(buffer+i);

				if (c < 30)
					Print(".");
				else
					Print("%c",c);
			}
			else
				break;
		}
		Print("\n");
	}
}


void
createFile(int32 num)
{
	char name[B_FILE_NAME_LENGTH];
	sprintf(name,"./_query_test_%ld",num);

	BFile file(name,B_CREATE_FILE | B_WRITE_ONLY);
}


BEntry *
getEntry(int32 num)
{
	char name[B_FILE_NAME_LENGTH];
	sprintf(name,"./_query_test_%ld",num);

	return new BEntry(name);
}


status_t
waitForMessage(port_id port,const char *string,int32 op,char *name)
{
	puts(string);

	int32 msg;
	char buffer[1024];
	ssize_t bytes = read_port_etc(port,&msg,buffer,sizeof(buffer),B_TIMEOUT,1000000);
	if (op == B_TIMED_OUT && bytes == B_TIMED_OUT) {
		puts("  passed, timed out!\n");
		return bytes;
	}
	if (bytes < B_OK) {
		printf("-> %s\n\n", strerror(bytes));
		return bytes;
	}

	BMessage message;
	if (message.Unflatten(buffer) < B_OK) {
		printf("could not unflatten message:\n");
		dumpBlock(buffer, bytes);
		return B_BAD_DATA;
	}

	if (message.what != B_QUERY_UPDATE
		|| message.FindInt32("opcode") != op) {
		printf("Expected what = %x, opcode = %ld, got:", B_QUERY_UPDATE, op);
		message.PrintToStream();
		putchar('\n');
		return message.FindInt32("opcode");
	}
	puts("  passed!\n");
	return op;
}


int
main(int argc,char **argv)
{
	port_id port = create_port(100, "query port");
	printf("port id = %ld\n", port);
	printf("  B_ENTRY_REMOVED = %d, B_ENTRY_CREATED = %d\n\n", B_ENTRY_REMOVED, B_ENTRY_CREATED);

	dev_t device = dev_for_path(".");
	DIR *query = fs_open_live_query(device, "name=_query_test_*", B_LIVE_QUERY, port, 12345);

	createFile(1);
	waitForMessage(port, "File 1 created:", B_ENTRY_CREATED, "_query_test_1");

	createFile(2);
	waitForMessage(port, "File 2 created:", B_ENTRY_CREATED, "_query_test_2");

	BEntry *entry = getEntry(2);
	if (entry->InitCheck() < B_OK) {
		fprintf(stderr, "Could not get entry for file 2\n");
		fs_close_query(query);
		return -1;
	}
	entry->Rename("_some_other_name_");
	waitForMessage(port,"File 2 renamed (should fall out of query):", B_ENTRY_REMOVED, NULL);

	entry->Rename("_some_other_");
	waitForMessage(port,"File 2 renamed again (should time out):", B_TIMED_OUT, NULL);

	entry->Rename("_query_test_2");
	waitForMessage(port,"File 2 renamed back (should be added to query):",
		B_ENTRY_CREATED, "_query_test_2");

	entry->Rename("_query_test_2_and_more");
	status_t status = waitForMessage(port,"File 2 renamed (should stay in query, time out):",
							B_TIMED_OUT, "_query_test_2_and_more");
	if (status == B_ENTRY_REMOVED) {
		waitForMessage(port,"Received B_ENTRY_REMOVED, now expecting file 2 to be added again:",
			B_ENTRY_CREATED, NULL);
	}

	entry->Remove();
	delete entry;
	waitForMessage(port,"File 2 removed:", B_ENTRY_REMOVED, NULL);

	entry = getEntry(1);
	if (entry->InitCheck() < B_OK) {
		fprintf(stderr, "Could not get entry for file 1\n");
		fs_close_query(query);
		return -1;
	}

	entry->Rename("_some_other_name_");
	waitForMessage(port, "File 1 renamed (should fall out of query):", B_ENTRY_REMOVED, NULL);

	entry->Remove();
	delete entry;
	waitForMessage(port, "File 1 removed (should time out):", B_TIMED_OUT, NULL);

	fs_close_query(query);

	return 0;
}

