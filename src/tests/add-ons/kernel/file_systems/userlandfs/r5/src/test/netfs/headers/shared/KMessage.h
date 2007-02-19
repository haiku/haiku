// KMessage.h

#ifndef KMESSAGE_H
#define KMESSAGE_H

#include <SupportDefs.h>
#include <TypeConstants.h>

namespace BPrivate {

class KMessageField;

// KMessage
class KMessage {
public:
	enum {
		KMESSAGE_OWNS_BUFFER		= 0x01,
		KMESSAGE_INIT_FROM_BUFFER	= 0x02,
		KMESSAGE_READ_ONLY			= 0x04,

		KMESSAGE_FLAG_MASK			= 0x07,
	};

	KMessage();
	KMessage(uint32 what);
	~KMessage();

	status_t SetTo(uint32 what, uint32 flags = 0);
	status_t SetTo(void *buffer, int32 bufferSize, uint32 what,
		uint32 flags = 0);
	status_t SetTo(const void *buffer, int32 bufferSize);
	void Unset();

	void SetWhat(uint32 what);
	uint32 What() const;

	const void *Buffer() const;
	int32 BufferCapacity() const;
	int32 ContentSize() const;

	status_t AddField(const char *name, type_code type, int32 elementSize = -1,
		KMessageField *field = NULL);
	status_t FindField(const char *name, KMessageField *field) const;
	status_t FindField(const char *name, type_code type,
		KMessageField *field) const;
	status_t GetNextField(KMessageField *field) const;

	status_t AddData(const char *name, type_code type, const void *data,
		int32 numBytes, bool isFixedSize = true);
	status_t AddArray(const char *name, type_code type, const void *data,
		int32 elementSize, int32 elementCount);
	inline status_t AddBool(const char *name, bool value);
	inline status_t AddInt8(const char *name, int8 value);
	inline status_t AddInt16(const char *name, int16 value);
	inline status_t AddInt32(const char *name, int32 value);
	inline status_t AddInt64(const char *name, int64 value);
	inline status_t AddString(const char *name, const char *value);

	status_t FindData(const char *name, type_code type,
		const void **data, int32 *numBytes) const;
	status_t FindData(const char *name, type_code type, int32 index,
		const void **data, int32 *numBytes) const;
	inline status_t FindBool(const char *name, bool *value) const;
	inline status_t FindBool(const char *name, int32 index, bool *value) const;
	inline status_t FindInt8(const char *name, int8 *value) const;
	inline status_t FindInt8(const char *name, int32 index, int8 *value) const;
	inline status_t FindInt16(const char *name, int16 *value) const;
	inline status_t FindInt16(const char *name, int32 index, int16 *value) const;
	inline status_t FindInt32(const char *name, int32 *value) const;
	inline status_t FindInt32(const char *name, int32 index, int32 *value) const;
	inline status_t FindInt64(const char *name, int64 *value) const;
	inline status_t FindInt64(const char *name, int32 index, int64 *value) const;
	inline status_t FindString(const char *name, const char **value) const;
	inline status_t FindString(const char *name, int32 index,
		const char **value) const;

	// message delivery
	team_id Sender() const;
	int32 TargetToken() const;
	port_id ReplyPort() const;
	int32 ReplyToken() const;

	status_t SendTo(port_id targetPort, int32 targetToken = -1,
		port_id replyPort = -1, int32 replyToken = -1, bigtime_t timeout = -1,
		team_id senderTeam = -1);
	status_t SendTo(port_id targetPort, int32 targetToken,
		KMessage* reply, bigtime_t deliveryTimeout = -1,
		bigtime_t replyTimeout = -1, team_id senderTeam = -1);
	status_t SendReply(KMessage* message, port_id replyPort = -1,
		int32 replyToken = -1, bigtime_t timeout = -1, team_id senderTeam = -1);
	status_t SendReply(KMessage* message, KMessage* reply,
		bigtime_t deliveryTimeout = -1, bigtime_t replyTimeout = -1,
		team_id senderTeam = -1);
	status_t ReceiveFrom(port_id fromPort, bigtime_t timeout = -1);

private:
	friend class KMessageField;

	struct Header {
		uint32		magic;
		int32		size;
		uint32		what;
		team_id		sender;
		int32		targetToken;
		port_id		replyPort;
		int32		replyToken;
	};

	struct FieldHeader;
	struct FieldValueHeader;

	Header *_Header() const;
	int32 _BufferOffsetFor(const void* data) const;
	FieldHeader *_FirstFieldHeader() const;
	FieldHeader *_LastFieldHeader() const;
	FieldHeader *_FieldHeaderForOffset(int32 offset) const;
//	FieldHeader *_NextFieldHeader(FieldHeader *fieldHeader) const;
	status_t _AddField(const char *name, type_code type, int32 elementSize,
		KMessageField *field);
	status_t _AddFieldData(KMessageField *field, const void *data,
		int32 elementSize, int32 elementCount);

	status_t _InitFromBuffer();
	void _InitBuffer(uint32 what);

	void _CheckBuffer();	// debugging only

	status_t _AllocateSpace(int32 size, bool alignAddress, bool alignSize,
		void **address, int32 *alignedSize);
	int32 _CapacityFor(int32 size);
	template<typename T> status_t _FindType(const char* name,
		type_code type, int32 index, T *value) const;

	Header			fHeader;	// pointed to by fBuffer, if nothing is
								// allocated
	void*			fBuffer;
	int32			fBufferCapacity;
	uint32			fFlags;
	int32			fLastFieldOffset;
};

// KMessageField
class KMessageField {
public:
	KMessageField();

	void Unset();

	KMessage *Message() const;

	const char *Name() const;
	type_code TypeCode() const;
	bool HasFixedElementSize() const;
	int32 ElementSize() const;		// if HasFixedElementSize()
	
	status_t AddElement(const void *data, int32 size = -1);
	status_t AddElements(const void *data, int32 count, int32 elementSize = -1);
	const void *ElementAt(int32 index, int32 *size = NULL) const;
	int32 CountElements() const;

private:
	void SetTo(KMessage *message, int32 headerOffset);

	KMessage::FieldHeader* _Header() const;

	friend class KMessage;

	KMessage				*fMessage;
	int32					fHeaderOffset;
};

} // namespace BPrivate

using BPrivate::KMessage;
using BPrivate::KMessageField;

// #pragma mark -
// inline functions

// AddBool
inline
status_t
KMessage::AddBool(const char *name, bool value)
{
	return AddData(name, B_BOOL_TYPE, &value, sizeof(bool), true);
}

// AddInt8
inline
status_t
KMessage::AddInt8(const char *name, int8 value)
{
	return AddData(name, B_INT8_TYPE, &value, sizeof(int8), true);
}

// AddInt16
inline
status_t
KMessage::AddInt16(const char *name, int16 value)
{
	return AddData(name, B_INT16_TYPE, &value, sizeof(int16), true);
}

// AddInt32
inline
status_t
KMessage::AddInt32(const char *name, int32 value)
{
	return AddData(name, B_INT32_TYPE, &value, sizeof(int32), true);
}

// AddInt64
inline
status_t
KMessage::AddInt64(const char *name, int64 value)
{
	return AddData(name, B_INT64_TYPE, &value, sizeof(int64), true);
}

// AddString
inline
status_t
KMessage::AddString(const char *name, const char *value)
{
	if (!value)
		return B_BAD_VALUE;
	return AddData(name, B_STRING_TYPE, value, strlen(value) + 1, false);
}

// #pragma mark -

// FindBool
inline
status_t
KMessage::FindBool(const char *name, bool *value) const
{
	return FindBool(name, 0, value);
}

// FindBool
inline
status_t
KMessage::FindBool(const char *name, int32 index, bool *value) const
{
	return _FindType(name, B_BOOL_TYPE, index, value);
}

// FindInt8
inline
status_t
KMessage::FindInt8(const char *name, int8 *value) const
{
	return FindInt8(name, 0, value);
}

// FindInt8
inline
status_t
KMessage::FindInt8(const char *name, int32 index, int8 *value) const
{
	return _FindType(name, B_INT8_TYPE, index, value);
}

// FindInt16
inline
status_t
KMessage::FindInt16(const char *name, int16 *value) const
{
	return FindInt16(name, 0, value);
}

// FindInt16
inline
status_t
KMessage::FindInt16(const char *name, int32 index, int16 *value) const
{
	return _FindType(name, B_INT16_TYPE, index, value);
}

// FindInt32
inline
status_t
KMessage::FindInt32(const char *name, int32 *value) const
{
	return FindInt32(name, 0, value);
}

// FindInt32
inline
status_t
KMessage::FindInt32(const char *name, int32 index, int32 *value) const
{
	return _FindType(name, B_INT32_TYPE, index, value);
}

// FindInt64
inline
status_t
KMessage::FindInt64(const char *name, int64 *value) const
{
	return FindInt64(name, 0, value);
}

// FindInt64
inline
status_t
KMessage::FindInt64(const char *name, int32 index, int64 *value) const
{
	return _FindType(name, B_INT64_TYPE, index, value);
}

// FindString
inline
status_t
KMessage::FindString(const char *name, const char **value) const
{
	return FindString(name, 0, value);
}

// FindString
inline
status_t
KMessage::FindString(const char *name, int32 index, const char **value) const
{
	int32 size;
	return FindData(name, B_STRING_TYPE, index, (const void**)value, &size);
}

#endif	// KMESSAGE_H
