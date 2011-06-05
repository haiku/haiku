/*
 * Copyright 2011, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */
#ifndef HID_WRITER_H
#define HID_WRITER_H

#include "HIDDataTypes.h"


class HIDWriter {
public:
								HIDWriter(size_t blockSize = 20);
								~HIDWriter();

			// High Level

			status_t			DefineInputPadding(uint8 reportID, uint8 count,
									uint8 bitLength);
			status_t			DefineInputData(uint8 reportID, uint8 count,
									uint8 bitLength, main_item_data data,
									uint32 logicalMinimum,
									uint32 logicalMaximum, uint16 usagePage,
									uint16 usageMinimum,
									uint16 usageMaximum = 0xffff);

			status_t			BeginCollection(uint8 collectionType,
									uint16 usagePage, uint16 usageID);
			status_t			EndCollection();

			// Low Level

			status_t			SetUsagePage(uint16 usagePage);
			status_t			SetLogicalMinimum(uint32 logicalMinimum);
			status_t			SetLogicalMaximum(uint32 logicalMaximum);
			status_t			SetReportSize(uint8 reportSize);
			status_t			SetReportID(uint8 reportID);
			status_t			SetReportCount(uint8 reportCount);

			status_t			LocalSetUsageID(uint16 usageID);
			status_t			LocalSetUsageMinimum(uint16 usageMinimum);
			status_t			LocalSetUsageMaximum(uint16 usageMaximum);

			status_t			BeginCollection(uint8 collectionType);

			status_t			Input(main_item_data data);
			status_t			Output(main_item_data data);
			status_t			Feature(main_item_data data);

			// Generic

			status_t			WriteShortItem(uint8 type, uint8 tag,
									uint32 value);
			status_t			Write(const void *data, size_t length);

			size_t				BufferLength() { return fBufferUsed; };
			const uint8 *		Buffer() { return fBuffer; };

			void				Reset();

private:
			size_t				fBlockSize;
			size_t				fBufferAllocated;
			size_t				fBufferUsed;
			uint8 *				fBuffer;
			status_t			fStatus;
};

#endif // HID_WRITER_H
