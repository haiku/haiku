/*
 * Copyright 2009, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

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
