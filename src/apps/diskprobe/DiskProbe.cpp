/* 
** Copyright 2004, Axel Dörfler, axeld@pinc-software.de. All rights reserved.
** Distributed under the terms of the OpenBeOS License.
*/


#include "DataEditor.h"

#include <Application.h>
#include <Autolock.h>

#include <stdio.h>
#include <string.h>


static const char *kSignature = "application/x-vnd.OpenBeOS-DiskProbe";


class DiskProbe : public BApplication {
	public:
		DiskProbe();
		~DiskProbe();
};


#define DUMPED_BLOCK_SIZE 16

void
dumpBlock(const uint8 *buffer, int size, const char *prefix)
{
	int i;
	
	for (i = 0; i < size;) {
		int start = i;

		printf(prefix);
		for (; i < start+DUMPED_BLOCK_SIZE; i++) {
			if (!(i % 4))
				printf(" ");

			if (i >= size)
				printf("  ");
			else
				printf("%02x", *(unsigned char *)(buffer + i));
		}
		printf("  ");

		for (i = start; i < start + DUMPED_BLOCK_SIZE; i++) {
			if (i < size) {
				char c = buffer[i];

				if (c < 30)
					printf(".");
				else
					printf("%c", c);
			} else
				break;
		}
		printf("\n");
	}
}


status_t
printBlock(DataEditor &editor, const char *text)
{
	puts(text);

	const uint8 *buffer;
	status_t status = editor.GetViewBuffer(&buffer);
	if (status < B_OK) {
		fprintf(stderr, "Could not get block: %s\n", strerror(status));
		return status;
	}

	dumpBlock(buffer, editor.FileSize() > editor.ViewOffset() + editor.ViewSize() ?
			editor.ViewSize() : editor.FileSize() - editor.ViewOffset(),
		"\t");

	return B_OK;
}


//	#pragma mark -


DiskProbe::DiskProbe()
	: BApplication(kSignature)
{
	DataEditor editor;
	status_t status = editor.SetTo("/boot/beos/apps/DiskProbe");
	if (status < B_OK) {
		fprintf(stderr, "Could not set editor: %s\n", strerror(status));
		return;
	}

	editor.SetBlockSize(512);
	editor.SetViewOffset(0);
	editor.SetViewSize(32);

	BAutolock locker(editor.Locker());

	if (printBlock(editor, "before (location 0):") < B_OK)
		return;

	editor.Replace(5, (const uint8 *)"*REPLACED*", 10);
	if (printBlock(editor, "after (location 0):") < B_OK)
		return;

	editor.SetViewOffset(700);
	if (printBlock(editor, "before (location 700):") < B_OK)
		return;

	editor.Replace(713, (const uint8 *)"**REPLACED**", 12);
	if (printBlock(editor, "after (location 700):") < B_OK)
		return;

	status = editor.Undo();
	if (status < B_OK)
		fprintf(stderr, "Could not undo: %s\n", strerror(status));
	if (printBlock(editor, "undo (location 700):") < B_OK)
		return;

	editor.SetViewOffset(0);
	if (printBlock(editor, "after (location 0):") < B_OK)
		return;

	status = editor.Undo();
	if (status < B_OK)
		fprintf(stderr, "Could not undo: %s\n", strerror(status));
	if (printBlock(editor, "undo (location 0):") < B_OK)
		return;

	status = editor.Redo();
	if (status < B_OK)
		fprintf(stderr, "Could not redo: %s\n", strerror(status));
	if (printBlock(editor, "redo (location 0):") < B_OK)
		return;

	status = editor.Redo();
	editor.SetViewOffset(700);
	if (status < B_OK)
		fprintf(stderr, "Could not redo: %s\n", strerror(status));
	if (printBlock(editor, "redo (location 700):") < B_OK)
		return;

	status = editor.Redo();
	if (status == B_OK)
		fprintf(stderr, "Could apply redo (non-expected behaviour)!\n");

	status = editor.Undo();
	if (status < B_OK)
		fprintf(stderr, "Could not undo: %s\n", strerror(status));
	if (printBlock(editor, "undo (location 700):") < B_OK)
		return;

	PostMessage(B_QUIT_REQUESTED);
}


DiskProbe::~DiskProbe()
{
}


//	#pragma mark -


int 
main(int argc, char **argv)
{
	DiskProbe probe;

	probe.Run();
	return 0;
}
