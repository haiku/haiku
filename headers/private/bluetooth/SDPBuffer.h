/*
 * Copyright 2026, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		mohammedrattia <mohammedrattia@gmail.com>
 */
#ifndef _SDP_BUFFER_H_
#define _SDP_BUFFER_H_


#include <bluetooth/sdp.h>

#include <ByteOrder.h>
#include <DataIO.h>
#include <Message.h>
#include <support/String.h>
#include <stdlib.h>


class SDPDataIO {
public:

	SDPDataIO(BDataIO* data)
		:
		fBuffer(data),
		fReadingStatus(B_OK),
		fNextType(SDP_DATA_NIL)
	{
	}


	~SDPDataIO()
	{
	}


	BDataIO*
	Buffer()
	{
		return fBuffer;
	}


	status_t
	ReadingStatus()
	{
		return fReadingStatus;
	}


	void
	ParseNext(BMessage *message, BString label)
	{
		switch (PeekType())
		{
			case SDP_DATA_NIL:
				break;

			case SDP_DATA_BOOL:
			{
				bool value = ReadNumber<bool>();
				if (fReadingStatus == B_OK)
					message->AddBool(label, value);
			}
			break;

			case SDP_DATA_INT8:
			{
				int8 value = ReadNumber<int8>();
				if (fReadingStatus == B_OK)
					message->AddInt8(label, value);
			}
			break;

			case SDP_DATA_UINT8:
			{
				uint8 value = ReadNumber<uint8>();
				if (fReadingStatus == B_OK)
					message->AddUInt8(label, value);
			}
			break;

			case SDP_DATA_INT16:
			{
				int16 value = ReadNumber<int16>();
				if (fReadingStatus == B_OK)
					message->AddInt16(label, value);
			}
			break;

			case SDP_DATA_UINT16:
			{
				uint16 value = ReadNumber<uint16>();
				if (fReadingStatus == B_OK)
					message->AddUInt16(label, value);
			}
			break;

			case SDP_DATA_INT32:
			{
				int32 value = ReadNumber<int32>();
				if (fReadingStatus == B_OK)
					message->AddInt32(label, value);
			}
			break;

			case SDP_DATA_UINT32:
			{
				uint32 value = ReadNumber<uint32>();
				if (fReadingStatus == B_OK)
					message->AddUInt32(label, value);
			}
			break;

			case SDP_DATA_INT64:
			{
				int64 value = ReadNumber<int64>();
				if (fReadingStatus == B_OK)
					message->AddInt64(label, value);
			}
			break;

			case SDP_DATA_UINT64:
			{
				uint64 value = ReadNumber<uint64>();
				if (fReadingStatus == B_OK)
					message->AddUInt64(label, value);
			}
			break;

			// 128-bit intgers are ignored
			case SDP_DATA_INT128:
			case SDP_DATA_UINT128:
			{
				ReadNumber<int>();
			}
			break;

			case SDP_DATA_UUID16:
			case SDP_DATA_UUID32:
			case SDP_DATA_UUID128:
			{
				sdp_uuid value = ReadUUID();
				if (fReadingStatus == B_OK)
					message->AddData(label, B_RAW_TYPE, &value, sizeof(value));
			}
			break;

			case SDP_DATA_STR8:
			case SDP_DATA_STR16:
			case SDP_DATA_STR32:
			{
				BString value = ReadString();
				if (fReadingStatus == B_OK)
					message->AddString(label, value);
			}
			break;

			case SDP_DATA_ALT8:
			case SDP_DATA_ALT16:
			case SDP_DATA_ALT32:
			case SDP_DATA_SEQ8:
			case SDP_DATA_SEQ16:
			case SDP_DATA_SEQ32:
			{
				size_t size = ReadSeq();
				char buffer[size];
				if (fReadingStatus == B_OK && fBuffer->ReadExactly(buffer, size) == B_OK)
					message->AddData(label, B_RAW_TYPE, buffer, size);
			}
			break;
		}
	}


	template <typename T> T ReadNumber();


	BString
	ReadString()
	{
		size_t strLength = 0;
		status_t status;
		switch (ReadType()) {
			case SDP_DATA_STR8:
			{
				uint8 size;
				status = fBuffer->ReadExactly(&size, sizeof(uint8));
				memcpy(&strLength, &size, sizeof(uint8));
			}
			break;

			case SDP_DATA_STR16:
			{
				uint16 size;
				status = fBuffer->ReadExactly(&size, sizeof(uint16));
				size = B_BENDIAN_TO_HOST_INT16(size);
				memcpy(&strLength, &size, sizeof(uint16));
			}
			break;

			case SDP_DATA_STR32:
			{
				uint32 size;
				status = fBuffer->ReadExactly(&size, sizeof(uint32));
				size = B_BENDIAN_TO_HOST_INT32(size);
				memcpy(&strLength, &size, sizeof(uint32));
			}
			break;

			default:
				status = B_ERROR;
				break;
		}

		if (status != B_OK) {
			fReadingStatus = status;
			return BString("");
		}

		char buffer[strLength];
		if (fBuffer->ReadExactly(buffer, strLength) < 0) {
			fReadingStatus = status;
			return BString("");
		}

		BString value(buffer, strLength);
		return value;
	}


	sdp_uuid
	ReadUUID()
	{
		sdp_uuid value = SDP_BT_BASE_UUID;
		status_t status;

		switch (ReadType()) {
			case SDP_DATA_UUID16:
			{
				value = SDP_BT_BASE_UUID;

				uint16 rawValue;
				status = fBuffer->ReadExactly(&rawValue, sizeof(uint16));
				*((uint16*)value.uuid + 6) = B_BENDIAN_TO_HOST_INT16(rawValue);

				value.type = SDP_UUID_16;
			}
			break;

			case SDP_DATA_UUID32:
			{
				value = SDP_BT_BASE_UUID;

				uint32 rawValue;
				status = fBuffer->ReadExactly(&rawValue, sizeof(uint32));
				*((uint32*)value.uuid + 3) = B_BENDIAN_TO_HOST_INT32(rawValue);

				value.type = SDP_UUID_32;
			}
			break;

			case SDP_DATA_UUID128:
			{
				for (int32 i = 15; i >= 0; --i) {
					status = fBuffer->ReadExactly(&value.uuid[i], sizeof(uint8));
					if (status != B_OK)
						break;
				}
				if (status != B_OK)
					break;

				// 16-bit and 32-bit UUIDs can be represented as 128-bit when send in SDP packets
				if (memcmp(value.uuid, SDP_BT_BASE_UUID.uuid, 12) != 0)
					value.type = SDP_UUID_128;
				else if (value.uuid[14] == 0 && value.uuid[15] == 0)
					value.type = SDP_UUID_16;
				else
					value.type = SDP_UUID_32;
			}
			break;

			default:
				status = B_ERROR;
				break;
		}

		if (status != B_OK) {
			fReadingStatus = status;
			value = SDP_BT_BASE_UUID;
			return value;
		}

		return value;
	}


	size_t
	ReadSeq()
	{
		size_t seqSize = 0;
		status_t status;
		switch (ReadType()) {
			case SDP_DATA_ALT8:
			case SDP_DATA_SEQ8:
			{
				uint8 size = 0;
				status = fBuffer->ReadExactly(&size, sizeof(uint8));
				seqSize = size;
			}
			break;

			case SDP_DATA_ALT16:
			case SDP_DATA_SEQ16:
			{
				uint16 size = 0;
				status = fBuffer->ReadExactly(&size, sizeof(uint16));
				size = B_BENDIAN_TO_HOST_INT16(size);
				seqSize = size;
			}
			break;

			case SDP_DATA_ALT32:
			case SDP_DATA_SEQ32:
			{
				uint32 size = 0;
				status = fBuffer->ReadExactly(&size, sizeof(uint32));
				size = B_BENDIAN_TO_HOST_INT32(size);
				seqSize = size;
			}
			break;

			default:
				status = B_ERROR;
				break;
		}

		if (status != B_OK) {
			fReadingStatus = status;
			return 0;
		}

		return seqSize;
	}


	void
	AddDataSeq(uint32 size)
	{
		if (size <= UINT8_MAX) {
			AddType(SDP_DATA_SEQ8);
			uint8 temp = (uint8)size;
			fBuffer->WriteExactly(&temp, sizeof(uint8));
		} else if (size <= UINT16_MAX) {
			AddType(SDP_DATA_SEQ16);
			uint16 temp = B_HOST_TO_BENDIAN_INT16((uint16)size);
			fBuffer->WriteExactly(&temp, sizeof(uint16));
		} else {
			AddType(SDP_DATA_SEQ32);
			uint32 temp = B_HOST_TO_BENDIAN_INT32((uint32)size);
			fBuffer->WriteExactly(&temp, sizeof(uint32));
		}
	}


	status_t
	AddUInt8(uint8 data)
	{
		AddType(SDP_DATA_UINT8);
		return fBuffer->WriteExactly(&data, sizeof(uint8));
	}


	status_t
	AddUInt16(uint16 data)
	{
		AddType(SDP_DATA_UINT16);
		uint16 value = B_BENDIAN_TO_HOST_INT16(data);
		return fBuffer->WriteExactly(&value, sizeof(uint16));
	}


	status_t
	AddUInt32(uint32 data)
	{
		AddType(SDP_DATA_UINT32);
		uint32 value = B_BENDIAN_TO_HOST_INT32(data);
		return fBuffer->WriteExactly(&value, sizeof(uint32));
	}


	status_t
	AddUInt64(uint64 data)
	{
		AddType(SDP_DATA_UINT64);
		uint64 value = B_BENDIAN_TO_HOST_INT64(data);
		return fBuffer->WriteExactly(&value, sizeof(uint64));
	}


	status_t
	AddUUID16(sdp_uuid data)
	{
		AddType(SDP_DATA_UUID16);
		uint16 hostUUID = B_HOST_TO_BENDIAN_INT16(*(uint16*)(data.uuid + 12));
		return fBuffer->WriteExactly(&hostUUID, sizeof(uint16));
	}


	status_t
	AddUUID16(uint16 data)
	{
		AddType(SDP_DATA_UUID16);
		uint16 value = B_BENDIAN_TO_HOST_INT16(data);
		return fBuffer->WriteExactly(&value, sizeof(uint16));
	}


	status_t
	AddUUID32(sdp_uuid data)
	{
		AddType(SDP_DATA_UUID32);
		uint32 hostUUID = B_HOST_TO_BENDIAN_INT32(*(uint32*)(data.uuid + 12));
		return fBuffer->WriteExactly(&hostUUID, sizeof(uint32));
	}


	status_t
	AddUUID32(uint32 data)
	{
		AddType(SDP_DATA_UUID32);
		uint32 value = B_BENDIAN_TO_HOST_INT32(data);
		return fBuffer->WriteExactly(&value, sizeof(uint32));
	}


	status_t
	AddUUID128(sdp_uuid data)
	{
		AddType(SDP_DATA_UUID128);
		for (int32 i = 15; i >= 0; --i) {
			if (fBuffer->WriteExactly(&data.uuid[i], sizeof(uint8)) != B_OK)
				return B_ERROR;
		}
		return B_OK;
	}


	uint8
	PeekType()
	{
		if (fNextType == SDP_DATA_NIL)
			fBuffer->ReadExactly(&fNextType, sizeof(uint8));
		return fNextType;
	}


private:

	status_t
	AddType(uint8 type)
	{
		return fBuffer->WriteExactly(&type, sizeof(uint8));
	}


	uint8
	ReadType()
	{
		if (fNextType == SDP_DATA_NIL)
			PeekType();

		uint8 type = fNextType;
		fNextType = SDP_DATA_NIL;
		return type;
	}


	BDataIO*	fBuffer;
	status_t	fReadingStatus;
	uint8 		fNextType;

};


template <typename T> T
SDPDataIO::ReadNumber()
{
	T result = 0;
	status_t status;
	switch (ReadType()) {
		case SDP_DATA_BOOL:
		{
			if (sizeof(T) != sizeof(bool)) {
				status = B_ERROR;
				break;
			}
			status = fBuffer->ReadExactly(&result, sizeof(T));
		}
		break;

		case SDP_DATA_UINT8:
		case SDP_DATA_INT8:
		{
			if (sizeof(T) != sizeof(uint8)) {
				status = B_ERROR;
				break;
			}
			status = fBuffer->ReadExactly(&result, sizeof(T));
		}
		break;

		case SDP_DATA_UINT16:
		case SDP_DATA_INT16:
		{
			if (sizeof(T) != sizeof(uint16)) {
				status = B_ERROR;
				break;
			}
			status = fBuffer->ReadExactly(&result, sizeof(T));
			result = B_BENDIAN_TO_HOST_INT16(result);
		}
		break;

		case SDP_DATA_UINT32:
		case SDP_DATA_INT32:
		{
			if (sizeof(T) != sizeof(uint32)) {
				status = B_ERROR;
				break;
			}
			status = fBuffer->ReadExactly(&result, sizeof(T));
			result = B_BENDIAN_TO_HOST_INT32(result);
		}
		break;

		case SDP_DATA_UINT64:
		case SDP_DATA_INT64:
		{
			if (sizeof(T) != sizeof(uint64)) {
				status = B_ERROR;
				break;
			}
			status = fBuffer->ReadExactly(&result, sizeof(T));
			result = B_BENDIAN_TO_HOST_INT64(result);
		}
		break;

		// 128-bit intgers are ignored
		case SDP_DATA_UINT128:
		case SDP_DATA_INT128:
		{
			uint8 dump[16];
			status = fBuffer->ReadExactly(&dump, sizeof(dump));
		}
		break;

		default:
			status = B_ERROR;
			break;
	}

	if (status != B_OK) {
		fReadingStatus = status;
		return 0;
	}

	return result;
}


#endif //_SDP_BUFFER_H_
