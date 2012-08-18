/*
 * Copyright 2009-2010, Stephan Aßmus <superstippi@gmx.de>
 * All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ProgressReporter.h"

#include <stdio.h>

#include <Catalog.h>
#include <Locale.h>


#undef B_TRANSLATION_CONTEXT
#define B_TRANSLATION_CONTEXT "InstallProgress"


ProgressReporter::ProgressReporter(const BMessenger& messenger,
		BMessage* message)
	:
	fStartTime(0),

	fBytesToWrite(0),
	fBytesWritten(0),

	fItemsToWrite(0),
	fItemsWritten(0),

	fMessenger(messenger),
	fMessage(message)
{
}


ProgressReporter::~ProgressReporter()
{
	delete fMessage;
}


void
ProgressReporter::Reset()
{
	fBytesToWrite = 0;
	fBytesWritten = 0;

	fItemsToWrite = 0;
	fItemsWritten = 0;

	if (fMessage) {
		BMessage message(*fMessage);
		message.AddString("status",
			B_TRANSLATE("Collecting copy information."));
		fMessenger.SendMessage(&message);
	}
}


void
ProgressReporter::AddItems(uint64 count, off_t bytes)
{
	fBytesToWrite += bytes;
	fItemsToWrite += count;
}


void
ProgressReporter::StartTimer()
{
	fStartTime = system_time();

	printf("%" B_PRIdOFF " bytes to write in %" B_PRId64 " files\n",
		fBytesToWrite, fItemsToWrite);

	if (fMessage) {
		BMessage message(*fMessage);
		message.AddString("status", B_TRANSLATE("Performing installation."));
		fMessenger.SendMessage(&message);
	}
}


void
ProgressReporter::ItemsWritten(uint64 items, off_t bytes,
	const char* itemName, const char* targetFolder)
{
	fItemsWritten += items;
	fBytesWritten += bytes;

	_UpdateProgress(itemName, targetFolder);
}


void
ProgressReporter::_UpdateProgress(const char* itemName,
	const char* targetFolder)
{
	if (fMessage == NULL)
		return;

	// TODO: Could add time to finish calculation here...

	BMessage message(*fMessage);
	float progress = 100.0 * fBytesWritten / fBytesToWrite;
	message.AddFloat("progress", progress);
	message.AddInt32("current", fItemsWritten);
	message.AddInt32("maximum", fItemsToWrite);
	message.AddString("item", itemName);
	message.AddString("folder", targetFolder);
	fMessenger.SendMessage(&message);
}
