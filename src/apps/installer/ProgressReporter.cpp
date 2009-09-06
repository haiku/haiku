/*
 * Copyright 2009, Stephan Aßmus <superstippi@gmx.de>
 *  All rights reserved. Distributed under the terms of the MIT License.
 */

#include "ProgressReporter.h"

#include <new>

#include <math.h>
#include <stdio.h>
#include <string.h>


using std::nothrow;


ProgressReporter::ProgressReporter(const BMessenger& messenger,
		BMessage* message)
	:
	fBytesRead(0),
	fItemsCopied(0),
	fTimeRead(0),

	fBytesWritten(0),
	fTimeWritten(0),

	fBytesToCopy(0),
	fItemsToCopy(0),

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
	fBytesRead = 0;
	fItemsCopied = 0;
	fTimeRead = 0;

	fBytesWritten = 0;
	fTimeWritten = 0;

	fBytesToCopy = 0;
	fItemsToCopy = 0;

	if (fMessage) {
		BMessage message(*fMessage);
		message.AddString("status", "Collecting copy information.");
		fMessenger.SendMessage(&message);
	}
}


void
ProgressReporter::AddItems(uint64 count, off_t bytes)
{
	// TODO ...
}


void
ProgressReporter::StartTimer()
{
	// TODO ...
}


void
ProgressReporter::ItemsCopied(uint64 items, off_t bytes, const char* itemName,
	const char* targetFolder)
{
	// TODO ...
}


void
ProgressReporter::_UpdateProgress(const char* itemName,
	const char* targetFolder)
{
	if (fMessage != NULL) {
		BMessage message(*fMessage);
		float progress = 100.0 * fBytesRead / fBytesToCopy;
		message.AddFloat("progress", progress);
		message.AddInt32("current", fItemsCopied);
		message.AddInt32("maximum", fItemsToCopy);
		message.AddString("item", itemName);
		message.AddString("folder", targetFolder);
		fMessenger.SendMessage(&message);
	}
}
