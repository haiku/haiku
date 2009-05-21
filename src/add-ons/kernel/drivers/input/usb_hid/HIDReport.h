/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef HID_REPORT_H
#define HID_REPORT_H

#include "HIDParser.h"

#include <condition_variable.h>

#define HID_REPORT_TYPE_INPUT		0x01
#define HID_REPORT_TYPE_OUTPUT		0x02
#define HID_REPORT_TYPE_FEATURE		0x04
#define HID_REPORT_TYPE_ANY			0x07

class HIDCollection;
class HIDReportItem;

class HIDReport {
public:
								HIDReport(HIDParser *parser, uint8 type,
									uint8 id);
								~HIDReport();

		uint8					Type() { return fType; };
		uint8					ID() { return fReportID; };
		size_t					ReportSize() { return (fReportSize + 7) / 8; };

		HIDParser *				Parser() { return fParser; };
		HIDDevice *				Device() { return fParser->Device(); };

		void					AddMainItem(global_item_state &globalState,
									local_item_state &localState,
									main_item_data &mainData,
									HIDCollection *collection);

		void					SetReport(status_t status, uint8 *report,
									size_t length);
		uint8 *					CurrentReport() { return fCurrentReport; };

		uint32					CountItems() { return fItemsUsed; };
		HIDReportItem *			ItemAt(uint32 index);

		status_t				WaitForReport(bigtime_t timeout);
		void					DoneProcessing();

		void					PrintToStream();

private:
		void					_SignExtend(uint32 &minimum, uint32 &maximum);

		HIDParser *				fParser;

		uint8					fType;
		uint8					fReportID;
		uint32					fReportSize;

		uint32					fItemsUsed;
		uint32					fItemsAllocated;
		HIDReportItem **		fItems;

		status_t				fReportStatus;
		uint8 *					fCurrentReport;
		int32					fBusyCount;
		ConditionVariable		fConditionVariable;
};

#endif // HID_REPORT_H
