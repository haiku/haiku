// KMessage.cpp

#include <stdlib.h>
#include <string.h>

#include <Debug.h>
#include <KernelExport.h>
#include <TypeConstants.h>

#include "KMessage.h"

// define the PANIC macro
#ifndef PANIC
#	if USER
#		define PANIC(str)	debugger(str)
#	else
#		define PANIC(str)	panic(str)
#	endif
#endif

static const uint32 kMessageHeaderMagic = 'kMsG';
static const int32 kMessageReallocChunkSize = 64;

#ifndef B_BUFFER_OVERFLOW
#	define B_BUFFER_OVERFLOW	EOVERFLOW
#endif

/*// strnlen
static inline
size_t
strnlen(const char *str, size_t maxLen)
{
	if (str) {
		size_t origMaxLen = maxLen;
		while (maxLen > 0 && *str != '\0') {
			maxLen--;
			str++;
		}
		return origMaxLen - maxLen;
	}
	return 0;
}*/

// _Align
static inline
int32
_Align(int32 offset)
{
	return (offset + 3) & ~0x3;
}

// _Align
static inline
void*
_Align(void *address, int32 offset = 0)
{
	return (void*)(((uint32)address + offset + 3) & ~0x3);
}

// FieldValueHeader
struct KMessage::FieldValueHeader {
	int32		size;

	void *Data()
	{
		return _Align(this, sizeof(FieldValueHeader));
	}

	FieldValueHeader *NextFieldValueHeader()
	{
		return (FieldValueHeader*)_Align(Data(), size);
	}
};

// FieldHeader
struct KMessage::FieldHeader {
	type_code	type;
	int32		elementSize;	// if < 0: non-fixed size
	int32		elementCount;
	int32		fieldSize;
	int16		headerSize;
	char		name[1];

	void *Data()
	{
		return (uint8*)this + headerSize;
	}

	bool HasFixedElementSize() { return (elementSize >= 0); }

	void *ElementAt(int32 index, int32 *size)
	{
		if (index < 0 || index >= elementCount)
			return NULL;
		uint8 *data = (uint8*)this + headerSize;
		if (HasFixedElementSize()) {
			*size = elementSize;
			return data + elementSize * index;
		}
		// non-fixed element size: we need to iterate
		FieldValueHeader *valueHeader = (FieldValueHeader *)data;
		for (int i = 0; i < index; i++)
			valueHeader = valueHeader->NextFieldValueHeader();
		*size = valueHeader->size;
		return valueHeader->Data();
	}

	FieldHeader *NextFieldHeader()
	{
		return (FieldHeader*)_Align(this, fieldSize);
	}
};

// constructor
KMessage::KMessage()
	: fBuffer(NULL),
	  fBufferCapacity(0),
	  fFlags(0),
	  fLastFieldOffset(0)
{
	Unset();
}

// constructor
KMessage::KMessage(uint32 what)
	: fBuffer(NULL),
	  fBufferCapacity(0),
	  fFlags(0),
	  fLastFieldOffset(0)
{
	Unset();
	SetWhat(what);
}

// destructor
KMessage::~KMessage()
{
	Unset();
}

// SetTo
status_t
KMessage::SetTo(uint32 what, uint32 flags)
{
	// There are no flags interesting in this case at the moment.
	Unset();
	SetWhat(what);
	return B_OK;
}

// SetTo
status_t
KMessage::SetTo(void *buffer, int32 bufferSize, uint32 what, uint32 flags)
{
	Unset();
	if (!buffer || bufferSize < (int)sizeof(Header))
		return B_BAD_VALUE;
	// if read-only, we need to init from the buffer, too
	if (flags & KMESSAGE_READ_ONLY && !(flags & KMESSAGE_INIT_FROM_BUFFER))
		return B_BAD_VALUE;
	fBuffer = buffer;
	fBufferCapacity = bufferSize;
	fFlags = flags;
	status_t error = B_OK;
	if (flags & KMESSAGE_INIT_FROM_BUFFER)
		error = _InitFromBuffer();
	else
		_InitBuffer(what);
	if (error != B_OK)
		Unset();
	return error;
}

// SetTo
status_t
KMessage::SetTo(const void *buffer, int32 bufferSize)
{
	return SetTo(const_cast<void*>(buffer), bufferSize, 0,
		KMESSAGE_INIT_FROM_BUFFER | KMESSAGE_READ_ONLY);
}

// Unset
void
KMessage::Unset()
{
	// free buffer
	if (fBuffer && fBuffer != &fHeader && (fFlags & KMESSAGE_OWNS_BUFFER))
		free(fBuffer);
	fBuffer = &fHeader;
	fBufferCapacity = sizeof(Header);
	_InitBuffer(0);
}

// SetWhat
void
KMessage::SetWhat(uint32 what)
{
	_Header()->what = what;
}

// What
uint32
KMessage::What() const
{
	return _Header()->what;
}

// Buffer
const void *
KMessage::Buffer() const
{
	return fBuffer;
}

// BufferCapacity
int32
KMessage::BufferCapacity() const
{
	return fBufferCapacity;
}

// ContentSize
int32
KMessage::ContentSize() const
{
	return _Header()->size;
}

// AddField
status_t
KMessage::AddField(const char *name, type_code type, int32 elementSize,
	KMessageField* field)
{
	if (!name || type == B_ANY_TYPE)
		return B_BAD_VALUE;
	KMessageField existingField;
	if (FindField(name, &existingField) == B_OK)
		return B_NAME_IN_USE;
	return _AddField(name, type, elementSize, field);
}

// FindField
status_t
KMessage::FindField(const char *name, KMessageField *field) const
{
	return FindField(name, B_ANY_TYPE, field);
}

// FindField
status_t
KMessage::FindField(const char *name, type_code type,
	KMessageField *field) const
{
	if (!name)
		return B_BAD_VALUE;
	KMessageField stackField;
	if (field)
		field->Unset();
	else
		field = &stackField;
	while (GetNextField(field) == B_OK) {
		if ((type == B_ANY_TYPE || field->TypeCode() == type)
			&& strcmp(name, field->Name()) == 0) {
			return B_OK;
		}
	}
	return B_NAME_NOT_FOUND;
}

// GetNextField
status_t
KMessage::GetNextField(KMessageField *field) const
{
	if (!field || (field->Message() != NULL && field->Message() != this))
		return B_BAD_VALUE;
	FieldHeader *fieldHeader = field->_Header();
	FieldHeader* lastField = _LastFieldHeader();
	if (!lastField)
		return B_NAME_NOT_FOUND;
	if (fieldHeader == NULL) {
		fieldHeader = _FirstFieldHeader();
	} else {
		if ((uint8*)fieldHeader < (uint8*)_FirstFieldHeader()
			|| (uint8*)fieldHeader > (uint8*)lastField) {
			return B_BAD_VALUE;
		}
		if (fieldHeader == lastField)
			return B_NAME_NOT_FOUND;
		fieldHeader = fieldHeader->NextFieldHeader();
	}
	field->SetTo(const_cast<KMessage*>(this), _BufferOffsetFor(fieldHeader));
	return B_OK;
}

// AddData
status_t
KMessage::AddData(const char *name, type_code type, const void *data,
	int32 numBytes, bool isFixedSize)
{
	if (!name || type == B_ANY_TYPE || !data || numBytes < 0)
		return B_BAD_VALUE;
	KMessageField field;
	if (FindField(name, &field) == B_OK) {
		// field with that name already exists: check its type
		if (field.TypeCode() != type)
			return B_BAD_TYPE;
	} else {
		// no such field yet: add it
		status_t error = _AddField(name, type, (isFixedSize ? numBytes : -1),
			&field);
		if (error != B_OK)
			return error;
	}
	return _AddFieldData(&field, data, numBytes, 1);
}

// AddArray
status_t
KMessage::AddArray(const char *name, type_code type, const void *data,
	int32 elementSize, int32 elementCount)
{
	if (!name || type == B_ANY_TYPE || !data || elementSize < 0
		|| elementCount < 0) {
		return B_BAD_VALUE;
	}
	KMessageField field;
	if (FindField(name, &field) == B_OK) {
		// field with that name already exists: check its type
		if (field.TypeCode() != type)
			return B_BAD_TYPE;
	} else {
		// no such field yet: add it
		status_t error = _AddField(name, type, elementSize, &field);
		if (error != B_OK)
			return error;
	}
	return _AddFieldData(&field, data, elementSize, elementCount);
}

// FindData
status_t
KMessage::FindData(const char *name, type_code type, const void **data,
	int32 *numBytes) const
{
	return FindData(name, type, 0, data, numBytes);
}

// FindData
status_t
KMessage::FindData(const char *name, type_code type, int32 index,
	const void **data, int32 *numBytes) const
{
	if (!name || !data || !numBytes)
		return B_BAD_VALUE;
	KMessageField field;
	status_t error = FindField(name, type, &field);
	if (error != B_OK)
		return error;
	const void *foundData = field.ElementAt(index, numBytes);
	if (!foundData)
		return B_BAD_INDEX;
	if (data)
		*data = foundData;
	return B_OK;
}

// Sender
team_id
KMessage::Sender() const
{
	return _Header()->sender;
}

// TargetToken
int32
KMessage::TargetToken() const
{
	return _Header()->targetToken;
}

// ReplyPort
port_id
KMessage::ReplyPort() const
{
	return _Header()->replyPort;
}

// ReplyToken
int32
KMessage::ReplyToken() const
{
	return _Header()->replyToken;
}

// SendTo
status_t
KMessage::SendTo(port_id targetPort, int32 targetToken, port_id replyPort,
	int32 replyToken, bigtime_t timeout, team_id senderTeam)
{
	// set the deliver info
	Header* header = _Header();
	header->sender = senderTeam;
	header->targetToken = targetToken;
	header->replyPort = replyPort;
	header->replyToken = replyToken;
	// get the sender team
	if (senderTeam >= 0) {
		thread_info info;
		status_t error = get_thread_info(find_thread(NULL), &info);
		if (error != B_OK)
			return error;
		header->sender = info.team;
	}
	// send the message
	if (timeout < 0)
		return write_port(targetPort, 'KMSG', fBuffer, ContentSize());
	return write_port_etc(targetPort, 'KMSG', fBuffer, ContentSize(),
		B_RELATIVE_TIMEOUT, timeout);
}

// SendTo
status_t
KMessage::SendTo(port_id targetPort, int32 targetToken, KMessage* reply,
	bigtime_t deliveryTimeout, bigtime_t replyTimeout, team_id senderTeam)
{
	// get the team the target port belongs to
	port_info portInfo;
	status_t error = get_port_info(targetPort, &portInfo);
	if (error != B_OK)
		return error;
	team_id targetTeam = portInfo.team;
	// allocate a reply port, if a reply is desired
	port_id replyPort = -1;
	if (reply) {
		// get our team
		team_id ourTeam = B_SYSTEM_TEAM;
		#if USER
			if (targetTeam != B_SYSTEM_TEAM) {
				thread_info threadInfo;
				error = get_thread_info(find_thread(NULL), &threadInfo);
				if (error != B_OK)
					return error;
				ourTeam = threadInfo.team;
			}
		#endif
		// create the port
		replyPort = create_port(1, "KMessage reply port");
		if (replyPort < 0)
			return replyPort;
		// If the target team is not our team and not the kernel team either,
		// we transfer the ownership of the port to it, so we will not block
		if (targetTeam != ourTeam && targetTeam != B_SYSTEM_TEAM)
			set_port_owner(replyPort, targetTeam);
	}
	struct PortDeleter {
		PortDeleter(port_id port) : port(port) {}
		~PortDeleter()
		{
			if (port >= 0)
				delete_port(port);
		}

		port_id	port;
	} replyPortDeleter(replyPort);
	// send the message
	error = SendTo(targetPort, targetToken, replyPort, 0,
		deliveryTimeout, senderTeam);
	if (error != B_OK)
		return error;
	// get the reply
	if (reply)
		return reply->ReceiveFrom(replyPort, replyTimeout);
	return B_OK;
}

// SendReply
status_t
KMessage::SendReply(KMessage* message, port_id replyPort, int32 replyToken,
	bigtime_t timeout, team_id senderTeam)
{
	if (!message)
		return B_BAD_VALUE;
	return message->SendTo(ReplyPort(), ReplyToken(), replyPort, replyToken,
		timeout, senderTeam);
}

// SendReply
status_t
KMessage::SendReply(KMessage* message, KMessage* reply,
	bigtime_t deliveryTimeout, bigtime_t replyTimeout, team_id senderTeam)
{
	if (!message)
		return B_BAD_VALUE;
	return message->SendTo(ReplyPort(), ReplyToken(), reply, deliveryTimeout,
		replyTimeout, senderTeam);
}

// ReceiveFrom
status_t
KMessage::ReceiveFrom(port_id fromPort, bigtime_t timeout)
{
	// get the port buffer size
	ssize_t size;
	if (timeout < 0)
		size = port_buffer_size(fromPort);
	else
		size = port_buffer_size_etc(fromPort, B_RELATIVE_TIMEOUT, timeout);
	if (size < 0)
		return size;
	// allocate a buffer
	uint8* buffer = (uint8*)malloc(size);
	if (!buffer)
		return B_NO_MEMORY;
	// read the message
	int32 what;
	ssize_t realSize = read_port_etc(fromPort, &what, buffer, size,
		B_RELATIVE_TIMEOUT, 0);
	if (realSize < 0)
		return realSize;
	if (size != realSize)
		return B_ERROR;
	// init the message
	return SetTo(buffer, size, 0,
		KMESSAGE_OWNS_BUFFER | KMESSAGE_INIT_FROM_BUFFER);
}

// _Header
KMessage::Header *
KMessage::_Header() const
{
	return (Header*)fBuffer;
}

// _BufferOffsetFor
int32
KMessage::_BufferOffsetFor(const void* data) const
{
	if (!data)
		return -1;
	return ((uint8*)data - (uint8*)fBuffer);
}

// _FirstFieldHeader
KMessage::FieldHeader *
KMessage::_FirstFieldHeader() const
{
	return (FieldHeader*)_Align(fBuffer, sizeof(Header));
}

// _LastFieldHeader
KMessage::FieldHeader *
KMessage::_LastFieldHeader() const
{
	return _FieldHeaderForOffset(fLastFieldOffset);
}

// _FieldHeaderForOffset
KMessage::FieldHeader *
KMessage::_FieldHeaderForOffset(int32 offset) const
{
	if (offset <= 0 || offset >= _Header()->size)
		return NULL;
	return (FieldHeader*)((uint8*)fBuffer + offset);
}

// _AddField
status_t
KMessage::_AddField(const char *name, type_code type, int32 elementSize,
	KMessageField *field)
{
	FieldHeader *fieldHeader;
	int32 alignedSize;
	status_t error = _AllocateSpace(sizeof(FieldHeader) + strlen(name), true,
		true, (void**)&fieldHeader, &alignedSize);
	if (error != B_OK)
		return error;
	fieldHeader->type = type;
	fieldHeader->elementSize = elementSize;
	fieldHeader->elementCount = 0;
	fieldHeader->fieldSize = alignedSize;
	fieldHeader->headerSize = alignedSize;
	strcpy(fieldHeader->name, name);
	fLastFieldOffset = _BufferOffsetFor(fieldHeader);
	if (field)
		field->SetTo(this, _BufferOffsetFor(fieldHeader));
	return B_OK;
}

// _AddFieldData
status_t
KMessage::_AddFieldData(KMessageField *field, const void *data,
	int32 elementSize, int32 elementCount)
{
	if (!field)
		return B_BAD_VALUE;
	FieldHeader *fieldHeader = field->_Header();
	FieldHeader* lastField = _LastFieldHeader();
	if (!fieldHeader || fieldHeader != lastField || !data
		|| elementSize < 0 || elementCount < 0) {
		return B_BAD_VALUE;
	}
	if (elementCount == 0)
		return B_OK;
	// fixed size values
	if (fieldHeader->HasFixedElementSize()) {
		if (elementSize != fieldHeader->elementSize)
			return B_BAD_VALUE;
		void *address;
		int32 alignedSize;
		status_t error = _AllocateSpace(elementSize * elementCount,
			(fieldHeader->elementCount == 0), false, &address, &alignedSize);
		if (error != B_OK)
			return error;
		fieldHeader = field->_Header();	// might have been relocated
		memcpy(address, data, elementSize * elementCount);
		fieldHeader->elementCount += elementCount;
		fieldHeader->fieldSize = (uint8*)address + alignedSize
			- (uint8*)fieldHeader;
		return B_OK;
	}
	// non-fixed size values
	// add the elements individually (TODO: Optimize!)
	int32 valueHeaderSize = _Align(sizeof(FieldValueHeader));
	int32 entrySize = valueHeaderSize + elementSize;
	for (int32 i = 0; i < elementCount; i++) {
		void *address;
		int32 alignedSize;
		status_t error = _AllocateSpace(entrySize, true, false, &address,
			&alignedSize);
		if (error != B_OK)
			return error;
		fieldHeader = field->_Header();	// might have been relocated
		FieldValueHeader *valueHeader = (FieldValueHeader*)address;
		valueHeader->size = elementSize;
		memcpy(valueHeader->Data(), (const uint8*)data + i * elementSize,
			elementSize);
		fieldHeader->elementCount++;
		fieldHeader->fieldSize = (uint8*)address + alignedSize
			- (uint8*)fieldHeader;
	}
	return B_OK;
}

// _InitFromBuffer
status_t
KMessage::_InitFromBuffer()
{
	if (!fBuffer || fBufferCapacity < (int)sizeof(Header)
		|| _Align(fBuffer) != fBuffer) {
		return B_BAD_DATA;
	}
	// check header
	Header *header = _Header();
	if (header->magic != kMessageHeaderMagic)
		return B_BAD_DATA;
	if (header->size < (int)sizeof(Header) || header->size > fBufferCapacity)
		return B_BAD_DATA;
	// check the fields
	FieldHeader *fieldHeader = NULL;
	uint8 *data = (uint8*)_FirstFieldHeader();
	int32 remainingBytes = (uint8*)fBuffer + header->size - data;
	while (remainingBytes > 0) {
		if (remainingBytes < (int)sizeof(FieldHeader))
			return B_BAD_DATA;
		fieldHeader = (FieldHeader*)data;
		// check field header
		if (fieldHeader->type == B_ANY_TYPE)
			return B_BAD_DATA;
		if (fieldHeader->elementCount < 0)
			return B_BAD_DATA;
		if (fieldHeader->fieldSize < (int)sizeof(FieldHeader)
			|| fieldHeader->fieldSize > remainingBytes) {
			return B_BAD_DATA;
		}
		if (fieldHeader->headerSize < (int)sizeof(FieldHeader)
			|| fieldHeader->headerSize > fieldHeader->fieldSize) {
			return B_BAD_DATA;
		}
		int32 maxNameLen = data + fieldHeader->headerSize
			- (uint8*)fieldHeader->name;
		int32 nameLen = strnlen(fieldHeader->name, maxNameLen);
		if (nameLen == maxNameLen || nameLen == 0)
			return B_BAD_DATA;
		int32 fieldSize =  fieldHeader->headerSize;
		if (fieldHeader->HasFixedElementSize()) {
			// fixed element size
			int32 dataSize = fieldHeader->elementSize
				* fieldHeader->elementCount;
			fieldSize = (uint8*)fieldHeader->Data() + dataSize - data;
		} else {
			// non-fixed element size
			FieldValueHeader *valueHeader
				= (FieldValueHeader *)fieldHeader->Data();
			for (int32 i = 0; i < fieldHeader->elementCount; i++) {
				remainingBytes = (uint8*)fBuffer + header->size
					- (uint8*)valueHeader;
				if (remainingBytes < (int)sizeof(FieldValueHeader))
					return B_BAD_DATA;
				uint8 *value = (uint8*)valueHeader->Data();
				remainingBytes = (uint8*)fBuffer + header->size - (uint8*)value;
				if (remainingBytes < valueHeader->size)
					return B_BAD_DATA;
				fieldSize = value + valueHeader->size - data;
				valueHeader = valueHeader->NextFieldValueHeader();
			}
			if (fieldSize > fieldHeader->fieldSize)
				return B_BAD_DATA;
		}
		data = (uint8*)fieldHeader->NextFieldHeader();
		remainingBytes = (uint8*)fBuffer + header->size - data;
	}
	fLastFieldOffset = _BufferOffsetFor(fieldHeader);
	return B_OK;
}

// _InitBuffer
void
KMessage::_InitBuffer(uint32 what)
{
	Header *header = _Header();
	header->magic = kMessageHeaderMagic;
	header->size = sizeof(Header);
	header->what = what;
	header->sender = -1;
	header->targetToken = -1;
	header->replyPort = -1;
	header->replyToken = -1;
	fLastFieldOffset = 0;
}

// _CheckBuffer
void
KMessage::_CheckBuffer()
{
	int32 lastFieldOffset = fLastFieldOffset;
	if (_InitFromBuffer() != B_OK) {
		PANIC("internal data mangled");
	}
	if (fLastFieldOffset != lastFieldOffset) {
		PANIC("fLastFieldOffset changed during KMessage::_CheckBuffer()");
	}
}

// _AllocateSpace
status_t
KMessage::_AllocateSpace(int32 size, bool alignAddress, bool alignSize,
	void **address, int32 *alignedSize)
{
	if (fBuffer != &fHeader && (fFlags & KMESSAGE_READ_ONLY))
		return B_NOT_ALLOWED;
	int32 offset = ContentSize();
	if (alignAddress)
		offset = _Align(offset);
	int32 newSize = offset + size;
	if (alignSize)
		newSize = _Align(newSize);
	// reallocate if necessary
	if (fBuffer == &fHeader) {
		int32 newCapacity = _CapacityFor(newSize);
		void *newBuffer = malloc(newCapacity);
		if (!newBuffer)
			return B_NO_MEMORY;
		fBuffer = newBuffer;
		fBufferCapacity = newCapacity;
		fFlags |= KMESSAGE_OWNS_BUFFER;
		memcpy(fBuffer, &fHeader, sizeof(fHeader));
	} else {
		if (newSize > fBufferCapacity) {
			// if we don't own the buffer, we can't resize it
			if (!(fFlags & KMESSAGE_OWNS_BUFFER))
				return B_BUFFER_OVERFLOW;
			int32 newCapacity = _CapacityFor(newSize);
			void *newBuffer = realloc(fBuffer, newCapacity);
			if (!newBuffer)
				return B_NO_MEMORY;
			fBuffer = newBuffer;
			fBufferCapacity = newCapacity;
		}
	}
	_Header()->size = newSize;
	*address = (char*)fBuffer + offset;
	*alignedSize = newSize - offset;
	return B_OK;
}

// _CapacityFor
int32
KMessage::_CapacityFor(int32 size)
{
	return (size + kMessageReallocChunkSize - 1) / kMessageReallocChunkSize
		* kMessageReallocChunkSize;
}

// _FindType
template<typename T>
status_t
KMessage::_FindType(const char* name, type_code type, int32 index,
	T *value) const
{
	const void *data;
	int32 size;
	status_t error = FindData(name, type, index, &data, &size);
	if (error != B_OK)
		return error;
	if (size != sizeof(T))
		return B_BAD_DATA;
	*value = *(T*)data;
	return B_OK;
}


// #pragma mark -

// constructor
KMessageField::KMessageField()
	: fMessage(NULL),
	  fHeaderOffset(0)
{
}

// Unset
void
KMessageField::Unset()
{
	fMessage = NULL;
	fHeaderOffset = 0;
}

// Message
KMessage *
KMessageField::Message() const
{
	return fMessage;
}

// Name
const char *
KMessageField::Name() const
{
	KMessage::FieldHeader* header = _Header();
	return (header ? header->name : NULL);
}

// TypeCode
type_code
KMessageField::TypeCode() const
{
	KMessage::FieldHeader* header = _Header();
	return (header ? header->type : 0);
}

// HasFixedElementSize
bool
KMessageField::HasFixedElementSize() const
{
	KMessage::FieldHeader* header = _Header();
	return (header ? header->HasFixedElementSize() : false);
}

// ElementSize
int32
KMessageField::ElementSize() const
{
	KMessage::FieldHeader* header = _Header();
	return (header ? header->elementSize : -1); 
}

// AddElement
status_t
KMessageField::AddElement(const void *data, int32 size)
{
	KMessage::FieldHeader* header = _Header();
	if (!header || !data)
		return B_BAD_VALUE;
	if (size < 0) {
		size = ElementSize();
		if (size < 0)
			return B_BAD_VALUE;
	}
	return fMessage->_AddFieldData(this, data, size, 1);
}

// AddElements
status_t
KMessageField::AddElements(const void *data, int32 count, int32 elementSize)
{
	KMessage::FieldHeader* header = _Header();
	if (!header || !data || count < 0)
		return B_BAD_VALUE;
	if (elementSize < 0) {
		elementSize = ElementSize();
		if (elementSize < 0)
			return B_BAD_VALUE;
	}
	return fMessage->_AddFieldData(this, data, elementSize, count);
}

// ElementAt
const void *
KMessageField::ElementAt(int32 index, int32 *size) const
{
	KMessage::FieldHeader* header = _Header();
	return (header ? header->ElementAt(index, size) : NULL);
}

// CountElements
int32
KMessageField::CountElements() const
{
	KMessage::FieldHeader* header = _Header();
	return (header ? header->elementCount : 0); 
}

// SetTo
void
KMessageField::SetTo(KMessage *message, int32 headerOffset)
{
	fMessage = message;
	fHeaderOffset = headerOffset;
}

// _GetHeader
KMessage::FieldHeader*
KMessageField::_Header() const
{
	return (fMessage ? fMessage->_FieldHeaderForOffset(fHeaderOffset) : NULL);
}

