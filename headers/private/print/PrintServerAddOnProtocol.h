/*
 * Copyright 2010 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Pfeiffer
 */
#ifndef PRINT_SERVER_ADD_ON_PROTOCOL_H
#define PRINT_SERVER_ADD_ON_PROTOCOL_H

extern const char* kPrintServerAddOnApplicationSignature;

extern const char* kPrintServerAddOnStatusAttribute;
extern const char* kPrinterDriverAttribute;
extern const char* kPrinterNameAttribute;
extern const char* kPrinterFolderAttribute;
extern const char* kPrintJobFileAttribute;
extern const char* kPrintSettingsAttribute;

enum {
	// message constants for the five corresponding
	// printer driver add-on hook functions
	kMessageAddPrinter = 'PSad',
		// Request:
		// 		BString kPrinterDriverAttribute
		// 		BString kPrinterNameAttribute
		// Reply:
		// 		int32 kPrintServerAddOnStatusAttribute

	kMessageConfigPage = 'PScp',
		// Request:
		// 		BString kPrinterDriverAttribute
		// 		BString kPrinterFolderAttribute
		// 		BMessage kPrintSettingsAttribute
		// Reply:
		// 		int32 kPrintServerAddOnStatusAttribute
		// 		BMessage kPrintSettingsAttribute (if status is B_OK)

	kMessageConfigJob = 'PScj',
		// Request:
		// 		BString kPrinterDriverAttribute
		// 		BString kPrinterFolderAttribute
		// 		BMessage kPrintSettingsAttribute
		// Reply:
		// 		int32 kPrintServerAddOnStatusAttribute
		// 		BMessage kPrintSettingsAttribute (if status is B_OK)

	kMessageDefaultSettings = 'PSds',
		// Request:
		// 		BString kPrinterDriverAttribute
		// 		BString kPrinterFolderAttribute
		// Reply:
		// 		int32 kPrintServerAddOnStatusAttribute
		// 		BMessage kPrintSettingsAttribute (if status is B_OK)

	kMessageTakeJob = 'PStj',
		// Request:
		// 		BString kPrinterDriverAttribute
		// 		BString kPrintJobFileAttribute
		// 		BString kPrinterFolderAttribute
		// Reply:
		// 		int32 kPrintServerAddOnStatusAttribute
};

#endif
