/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun, <host.haiku@gmx.de
 */

#include <PrinterRoster.h>

#include <FindDirectory.h>
#include <Node.h>
#include <NodeMonitor.h>
#include <Path.h>
#include <Printer.h>


#include <new>


namespace BPrivate {
	namespace Print {


BPrinterRoster::BPrinterRoster()
	: fListener(NULL)
{
	BPath path;
	find_directory(B_USER_PRINTERS_DIRECTORY, &path, true);
	BNode(path.Path()).GetNodeRef(&fUserPrintersNodRef);

	fUserPrintersDirectory.SetTo(&fUserPrintersNodRef);
}


BPrinterRoster::~BPrinterRoster()
{
	StopWatching();
}


int32
BPrinterRoster::CountPrinters()
{
	Rewind();

	int32 i = 0;
	BPrinter printer;
	while (GetNextPrinter(&printer) == B_OK)
		i++;

	Rewind();
	return i;
}


status_t
BPrinterRoster::GetNextPrinter(BPrinter* printer)
{
	if (!printer)
		return B_BAD_VALUE;

	status_t status = fUserPrintersDirectory.InitCheck();
	if (!status == B_OK)
		return status;

	BEntry entry;
	bool next = true;
	while (status == B_OK && next) {
		status = fUserPrintersDirectory.GetNextEntry(&entry);
		if (status == B_OK) {
			printer->SetTo(entry);
			next = !printer->IsValid();
		} else {
			printer->Unset();
		}
	}
	return status;
}


status_t
BPrinterRoster::GetDefaultPrinter(BPrinter* printer)
{
	if (!printer)
		return B_BAD_VALUE;

	BDirectory dir(&fUserPrintersNodRef);
	status_t status = dir.InitCheck();
	if (!status == B_OK)
		return status;

	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		if (!entry.IsDirectory())
			continue;

		printer->SetTo(entry);
		if (printer->IsValid() && printer->IsDefault())
			return B_OK;
	}
	printer->Unset();
	return B_ERROR;
}


status_t
BPrinterRoster::FindPrinter(const BString& name, BPrinter* printer)
{
	if (name.Length() <= 0 || !printer)
		return B_BAD_VALUE;

	BDirectory dir(&fUserPrintersNodRef);
	status_t status = dir.InitCheck();
	if (!status == B_OK)
		return status;

	BEntry entry;
	while (dir.GetNextEntry(&entry) == B_OK) {
		if (!entry.IsDirectory())
			continue;

		printer->SetTo(entry);
		if (printer->IsValid() && printer->Name() == name)
			return B_OK;
	}
	printer->Unset();
	return B_ERROR;

}


status_t
BPrinterRoster::Rewind()
{
	return fUserPrintersDirectory.Rewind();
}


status_t
BPrinterRoster::StartWatching(const BMessenger& listener)
{
	StopWatching();

	if (!listener.IsValid())
		return B_BAD_VALUE;

	fListener = new(std::nothrow) BMessenger(listener);
	if (!fListener)
		return B_NO_MEMORY;

	return watch_node(&fUserPrintersNodRef, B_WATCH_DIRECTORY, *fListener);
}


void
BPrinterRoster::StopWatching()
{
	if (fListener) {
		stop_watching(*fListener);
		delete fListener;
		fListener = NULL;
	}
}


	}	// namespace Print
}	// namespace BPrivate
