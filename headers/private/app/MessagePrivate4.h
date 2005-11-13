#ifndef _MESSAGE_PRIVATE_H_
#define _MESSAGE_PRIVATE_H_

#include <Message.h>
#include <Messenger.h>
#include <MessengerPrivate.h>
#include <TokenSpace.h>


#define MESSAGE_BODY_HASH_TABLE_SIZE	10
#define MAX_DATA_PREALLOCATION			B_PAGE_SIZE * 10
#define MAX_FIELD_PREALLOCATION			50
#define MAX_ITEM_PREALLOCATION			B_PAGE_SIZE


enum {
	MESSAGE_FLAG_VALID = 0x0001,
	MESSAGE_FLAG_REPLY_REQUIRED = 0x0002,
	MESSAGE_FLAG_REPLY_DONE = 0x0004,
	MESSAGE_FLAG_IS_REPLY = 0x0008,
	MESSAGE_FLAG_WAS_DELIVERED = 0x0010,
	MESSAGE_FLAG_HAS_SPECIFIERS = 0x0020,
	MESSAGE_FLAG_READ_ONLY = 0x0040
};


enum {
	FIELD_FLAG_VALID = 0x0001,
	FIELD_FLAG_FIXED_SIZE = 0x0002,
};


typedef struct field_header_s {
	uint32		flags;
	type_code	type;
	int32		nameLength;
	int32		count;
	ssize_t		dataSize;
	ssize_t		allocated;
	int32		offset;
	int32		nextField;
} FieldHeader;


typedef struct message_header_s {
	uint32		format;
	uint32		what;
	uint32		flags;

	ssize_t		fieldsSize;
	ssize_t		dataSize;
	ssize_t		fieldsAvailable;
	ssize_t		dataAvailable;

	int32		target;
	int32		currentSpecifier;

	// reply info
	port_id		replyPort;
	int32		replyTarget;
	team_id		replyTeam;

	// body info
	int32		fieldCount;
	int32		hashTableSize;
	int32		hashTable[MESSAGE_BODY_HASH_TABLE_SIZE];

	/*	The hash table does contain indexes into the field list and
		not direct offsets to the fields. This has the advantage
		of not needing to update offsets in two locations.
		The hash table must be reevaluated when we remove a field
		though.
	*/
} MessageHeader;


class BMessage::Private {

public:
						Private(BMessage *msg)
							: fMessage(msg)
						{
						}

						Private(BMessage &msg)
							: fMessage(&msg)
						{
						}

inline	void			SetTarget(int32 token, bool preferred)
						{
							fMessage->fHeader->target = (preferred
								? B_PREFERRED_TOKEN : token);
						}

inline	void			SetReply(BMessenger messenger)
						{
							BMessenger::Private messengerPrivate(messenger);
							fMessage->fHeader->replyPort = messengerPrivate.Port();
							fMessage->fHeader->replyTarget = messengerPrivate.Token();
							fMessage->fHeader->replyTeam = messengerPrivate.Team();
						}

inline	int32			GetTarget()
						{
							return fMessage->fHeader->target;
						}

inline	bool			UsePreferredTarget()
						{
							return fMessage->fHeader->target == B_PREFERRED_TOKEN;
						}

inline	status_t		Clear()
						{
							return fMessage->_Clear();
						}

inline	status_t		InitHeader()
						{
							return fMessage->_InitHeader();
						}

inline	MessageHeader	*GetMessageHeader()
						{
							return fMessage->fHeader;
						}

inline	FieldHeader		*GetMessageFields()
						{
							return fMessage->fFields;
						}

inline	uint8			*GetMessageData()
						{
							return fMessage->fData;
						}

inline	ssize_t			NativeFlattenedSize() const
						{
							return fMessage->_NativeFlattenedSize();
						}

inline	status_t		NativeFlatten(char *buffer, ssize_t size) const
						{
							return fMessage->_NativeFlatten(buffer, size);
						}

inline	status_t		NativeFlatten(BDataIO *stream, ssize_t *size) const
						{
							return fMessage->_NativeFlatten(stream, size);
						}

inline	status_t		SendMessage(port_id port, int32 token, bool preferred,
							bigtime_t timeout, bool replyRequired,
							BMessenger &replyTo) const
						{
							return fMessage->_SendMessage(port, token,
								preferred, timeout, replyRequired, replyTo);
						}

inline	status_t		SendMessage(port_id port, team_id portOwner,
							int32 token, bool preferred, BMessage *reply,
							bigtime_t sendTimeout, bigtime_t replyTimeout) const
						{
							return fMessage->_SendMessage(port, portOwner, token,
								preferred, reply, sendTimeout, replyTimeout);
						}

static
inline	status_t		SendFlattenedMessage(void *data, int32 size,
							port_id port, int32 token, bool preferred,
							bigtime_t timeout)
						{
							return BMessage::_SendFlattenedMessage(data, size,
								port, token, preferred, timeout);
						}

static
inline	void			StaticInit()
						{
							BMessage::_StaticInit();
						}

static
inline	void			StaticCleanup()
						{
							BMessage::_StaticCleanup();
						}

static
inline	void			StaticCacheCleanup()
						{
							BMessage::_StaticCacheCleanup();
						}

private:
		BMessage		*fMessage;
};

#endif	// _MESSAGE_PRIVATE_H_
