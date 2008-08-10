/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun, <host.haiku@gmx.de
 */
#ifndef _PRINTER_ROSTER_H_
#define _PRINTER_ROSTER_H_


#include <Directory.h>
#include <Messenger.h>
#include <Node.h>
#include <String.h>


namespace BPrivate {
	namespace Print {


class BPrinter;


class BPrinterRoster {
public:
							BPrinterRoster();
							~BPrinterRoster();

		int32				CountPrinters();

		status_t			GetNextPrinter(BPrinter* printer);
		status_t			GetDefaultPrinter(BPrinter* printer);
		status_t			FindPrinter(const BString& name, BPrinter* printer);

		status_t			Rewind();

		status_t			StartWatching(const BMessenger& listener);
		void				StopWatching();

private:
							BPrinterRoster(const BPrinterRoster& printerRoster);
		BPrinterRoster&		operator=(const BPrinterRoster& printerRoster);

private:
		BMessenger*			fListener;

		node_ref			fUserPrintersNodRef;
		BDirectory			fUserPrintersDirectory;
};


	}	// namespace Print
}	// namespace BPrivate


#endif	// _PRINTER_ROSTER_H_
