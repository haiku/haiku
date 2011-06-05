/*
 * Copyright 2011, Michael Lotz, mmlr@mlotz.ch.
 * Distributed under the terms of the MIT License.
 */

#ifndef USERLAND_HID
#include "Driver.h"
#else
#include "UserlandHID.h"
#endif

#include "HIDWriter.h"

#include "HIDDataTypes.h"

#include <stdlib.h>
#include <string.h>


HIDWriter::HIDWriter(size_t blockSize)
	:	fBlockSize(blockSize),
		fBufferAllocated(0),
		fBufferUsed(0),
		fBuffer(NULL),
		fStatus(B_OK)
{
}


HIDWriter::~HIDWriter()
{
	free(fBuffer);
}


// #pragma mark - High Level


status_t
HIDWriter::DefineInputPadding(uint8 reportID, uint8 count, uint8 bitLength)
{
	SetReportID(reportID);
	SetReportSize(bitLength);
	SetReportCount(count);

	main_item_data data;
	data.data_constant = 1;
	return Input(data);
}


status_t
HIDWriter::DefineInputData(uint8 reportID, uint8 count, uint8 bitLength,
	main_item_data data, uint32 logicalMinimum, uint32 logicalMaximum,
	uint16 usagePage, uint16 usageMinimum, uint16 usageMaximum)
{
	SetReportID(reportID);
	SetReportSize(bitLength);
	SetReportCount(count);

	SetLogicalMinimum(logicalMinimum);
	SetLogicalMaximum(logicalMaximum);

	SetUsagePage(usagePage);
	LocalSetUsageMinimum(usageMinimum);
	LocalSetUsageMaximum(
		usageMaximum == 0xffff ? usageMinimum + count - 1 : usageMaximum);
	return Input(data);
}


status_t
HIDWriter::BeginCollection(uint8 collectionType, uint16 usagePage,
	uint16 usageID)
{
	SetUsagePage(usagePage);
	LocalSetUsageID(usageID);
	return BeginCollection(collectionType);
}


status_t
HIDWriter::EndCollection()
{
	return WriteShortItem(ITEM_TYPE_MAIN, ITEM_TAG_MAIN_END_COLLECTION, 0);
}


// #pragma mark - Low Level


status_t
HIDWriter::SetUsagePage(uint16 usagePage)
{
	return WriteShortItem(ITEM_TYPE_GLOBAL, ITEM_TAG_GLOBAL_USAGE_PAGE,
		usagePage);
}


status_t
HIDWriter::SetLogicalMinimum(uint32 logicalMinimum)
{
	return WriteShortItem(ITEM_TYPE_GLOBAL, ITEM_TAG_GLOBAL_LOGICAL_MINIMUM,
		logicalMinimum);
}


status_t
HIDWriter::SetLogicalMaximum(uint32 logicalMaximum)
{
	return WriteShortItem(ITEM_TYPE_GLOBAL, ITEM_TAG_GLOBAL_LOGICAL_MAXIMUM,
		logicalMaximum);
}


status_t
HIDWriter::SetReportSize(uint8 reportSize)
{
	return WriteShortItem(ITEM_TYPE_GLOBAL, ITEM_TAG_GLOBAL_REPORT_SIZE,
		reportSize);
}


status_t
HIDWriter::SetReportID(uint8 reportID)
{
	return WriteShortItem(ITEM_TYPE_GLOBAL, ITEM_TAG_GLOBAL_REPORT_ID,
		reportID);
}


status_t
HIDWriter::SetReportCount(uint8 reportCount)
{
	return WriteShortItem(ITEM_TYPE_GLOBAL, ITEM_TAG_GLOBAL_REPORT_COUNT,
		reportCount);
}


status_t
HIDWriter::LocalSetUsageID(uint16 usageID)
{
	return WriteShortItem(ITEM_TYPE_LOCAL, ITEM_TAG_LOCAL_USAGE, usageID);
}


status_t
HIDWriter::LocalSetUsageMinimum(uint16 usageMinimum)
{
	return WriteShortItem(ITEM_TYPE_LOCAL, ITEM_TAG_LOCAL_USAGE_MINIMUM,
		usageMinimum);
}


status_t
HIDWriter::LocalSetUsageMaximum(uint16 usageMaximum)
{
	return WriteShortItem(ITEM_TYPE_LOCAL, ITEM_TAG_LOCAL_USAGE_MAXIMUM,
		usageMaximum);
}


status_t
HIDWriter::BeginCollection(uint8 collectionType)
{
	return WriteShortItem(ITEM_TYPE_MAIN, ITEM_TAG_MAIN_COLLECTION,
		collectionType);
}


status_t
HIDWriter::Input(main_item_data data)
{
	main_item_data_converter converter;
	converter.main_data = data;
	return WriteShortItem(ITEM_TYPE_MAIN, ITEM_TAG_MAIN_INPUT,
		converter.flat_data);
}


status_t
HIDWriter::Output(main_item_data data)
{
	main_item_data_converter converter;
	converter.main_data = data;
	return WriteShortItem(ITEM_TYPE_MAIN, ITEM_TAG_MAIN_OUTPUT,
		converter.flat_data);
}


status_t
HIDWriter::Feature(main_item_data data)
{
	main_item_data_converter converter;
	converter.main_data = data;
	return WriteShortItem(ITEM_TYPE_MAIN, ITEM_TAG_MAIN_FEATURE,
		converter.flat_data);
}


// #pragma mark - Generic


status_t
HIDWriter::WriteShortItem(uint8 type, uint8 tag, uint32 value)
{
	short_item item;
	item.prefix.size = 0;

	if (value > 0) {
		if (value <= 0xff)
			item.prefix.size = 1;
		else if (value <= 0xffff)
			item.prefix.size = 2;
		else
			item.prefix.size = 3; // actually means 4
	}

	item.prefix.type = type;
	item.prefix.tag = tag;

	switch (item.prefix.size) {
		case 0:
			return Write(&item, sizeof(item_prefix));
		case 1:
			item.data.as_uint8[0] = value;
			return Write(&item, sizeof(item_prefix) + sizeof(uint8));
		case 2:
			item.data.as_uint16[0] = value;
			return Write(&item, sizeof(item_prefix) + sizeof(uint16));
		case 3:
			item.data.as_uint32 = value;
			return Write(&item, sizeof(item_prefix) + sizeof(uint32));
	}

	return B_OK;
}


status_t
HIDWriter::Write(const void *data, size_t length)
{
	if (fStatus != B_OK)
		return fStatus;

	size_t available = fBufferAllocated - fBufferUsed;
	if (length > available) {
		fBufferAllocated += length > fBlockSize ?  length : fBlockSize;
		uint8 *newBuffer = (uint8 *)realloc(fBuffer, fBufferAllocated);
		if (newBuffer == NULL) {
			fBufferAllocated -= fBlockSize;
			fStatus = B_NO_MEMORY;
			return fStatus;
		}

		fBuffer = newBuffer;
	}

	memcpy(fBuffer + fBufferUsed, data, length);
	fBufferUsed += length;
	return B_OK;
}


void
HIDWriter::Reset()
{
	free(fBuffer);
	fBuffer = NULL;
	fBufferUsed = 0;
	fBufferAllocated = 0;
	fStatus = B_OK;
}
