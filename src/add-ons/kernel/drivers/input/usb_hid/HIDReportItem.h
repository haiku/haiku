/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef HID_REPORT_ITEM_H
#define HID_REPORT_ITEM_H

#include <SupportDefs.h>

class HIDReport;

class HIDReportItem {
public:
								HIDReportItem(HIDReport *report,
									uint32 bitOffset, uint8 bitLength,
									bool hasData, bool isArray, bool isRelative,
									uint32 minimum, uint32 maximum,
									uint32 usageMinimum, uint32 usageMaximum);

		bool					HasData() { return fHasData; };
		bool					Relative() { return fRelative; };
		bool					Array() { return fArray; };
		bool					Signed() { return fMinimum > fMaximum; };

		uint32					UsageMinimum() { return fUsageMinimum; };
		uint32					UsageMaximum() { return fUsageMaximum; };

		status_t				Extract();
		bool					Valid() { return fValid; };
		uint32					Data() { return fData; };

private:
		HIDReport *				fReport;
		uint32					fByteOffset;
		uint8					fShift;
		uint32					fMask;
		bool					fHasData;
		bool					fArray;
		bool					fRelative;
		uint32					fMinimum;
		uint32					fMaximum;
		uint32					fUsageMinimum;
		uint32					fUsageMaximum;

		uint32					fData;
		bool					fValid;
};

#endif // HID_REPORT_ITEM_H
