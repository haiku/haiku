/*
 * Copyright 2008 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Julun, <host.haiku@gmx.de
 */
#ifndef _PRINTER_H_
#define _PRINTER_H_


#include <Directory.h>
#include <Entry.h>
#include <image.h>
#include <Message.h>
#include <Node.h>
#include <Path.h>
#include <String.h>


namespace BPrivate {
	namespace Print {


class BPrinter {
public:
							BPrinter();
							BPrinter(const BEntry& entry);
							BPrinter(const BPrinter& printer);
							BPrinter(const node_ref& nodeRef);
							BPrinter(const entry_ref& entryRef);
							BPrinter(const BDirectory& directory);
							~BPrinter();

			status_t		SetTo(const BEntry& entry);
			status_t		SetTo(const node_ref& nodeRef);
			status_t		SetTo(const entry_ref& entryRef);
			status_t		SetTo(const BDirectory& directory);
			void			Unset();

			bool			IsValid() const;
			status_t		InitCheck() const;

			bool			IsFree() const;
			bool			IsDefault() const;
			bool			IsShareable() const;

			BString			Name() const;
			BString			State() const;
			BString			Driver() const;
			BString			Comments() const;
			BString			Transport() const;
			BString			TransportAddress() const;
			status_t		DefaultSettings(BMessage& settings);

			status_t		StartWatching(const BMessenger& listener);
			void			StopWatching();

			BPrinter&		operator=(const BPrinter& printer);
			bool			operator==(const BPrinter& printer) const;
			bool			operator!=(const BPrinter& printer) const;

private:
			status_t		_Configure() const;
			status_t		_ConfigureJob(BMessage& settings);
			status_t		_ConfigurePage(BMessage& settings);

			BPath			_DriverPath() const;
			image_id		_LoadDriver() const;
			void			_AddPrinterName(BMessage& settings);
			BString			_ReadAttribute(const char* attribute) const;

private:
			BMessenger*		fListener;
			entry_ref		fPrinterEntryRef;
};


	}	// namespace Print
}	// namespace BPrivate


#endif	// _PRINTER_H_
