/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#include "Driver.h"

#include "HIDReportItem.h"
#include "HIDReport.h"

#include <string.h>

HIDReportItem::HIDReportItem(HIDReport *report, uint32 bitOffset,
	uint8 bitLength, bool hasData, bool isArray, bool isRelative,
	uint32 minimum, uint32 maximum, uint32 usageMinimum, uint32 usageMaximum)
	:	fReport(report),
		fByteOffset(bitOffset / 8),
		fShift(bitOffset % 8),
		fMask(~(0xffffffff << bitLength)),
		fHasData(hasData),
		fArray(isArray),
		fRelative(isRelative),
		fMinimum(minimum),
		fMaximum(maximum),
		fUsageMinimum(usageMinimum),
		fUsageMaximum(usageMaximum),
		fData(0),
		fValid(false)
{
}


uint16
HIDReportItem::UsagePage()
{
	usage_value value;
	value.u.extended = fUsageMinimum;
	return value.u.s.usage_page;
}


status_t
HIDReportItem::Extract()
{
	// The specs restrict items to span at most across 4 bytes, which means
	// that we can always just byte-align, copy four bytes and then shift and
	// mask as needed.
	uint8 *report = fReport->CurrentReport();
	if (report == NULL)
		return B_NO_INIT;

	memcpy(&fData, report + fByteOffset, sizeof(uint32));
	fData >>= fShift;
	fData &= fMask;

	if (Signed()) {
		// sign extend if needed.
		if ((fData & ~(fMask >> 1)) != 0)
			fData |= ~fMask;

		fValid = (int32)fData >= (int32)fMinimum
			&& (int32)fData <= (int32)fMaximum;
	} else
		fValid = fData >= fMinimum && fData <= fMaximum;

	return B_OK;
}


void
HIDReportItem::PrintToStream(uint32 indentLevel)
{
	char indent[indentLevel + 1];
	memset(indent, '\t', indentLevel);
	indent[indentLevel] = 0;

	TRACE_ALWAYS("%sHIDReportItem %p\n", indent, this);
	TRACE_ALWAYS("%s\tbyte offset: %lu\n", indent, fByteOffset);
	TRACE_ALWAYS("%s\tshift: %u\n", indent, fShift);
	TRACE_ALWAYS("%s\tmask: 0x%08lx\n", indent, fMask);
	TRACE_ALWAYS("%s\thas data: %s\n", indent, fHasData ? "yes" : "no");
	TRACE_ALWAYS("%s\tarray: %s\n", indent, fArray ? "yes" : "no");
	TRACE_ALWAYS("%s\trelative: %s\n", indent, fRelative ? "yes" : "no");
	TRACE_ALWAYS("%s\tminimum: %lu\n", indent, fMinimum);
	TRACE_ALWAYS("%s\tmaximum: %lu\n", indent, fMaximum);
	TRACE_ALWAYS("%s\tusage minimum: 0x%08lx\n", indent, fUsageMinimum);
	TRACE_ALWAYS("%s\tusage maximum: 0x%08lx\n", indent, fUsageMaximum);
}
