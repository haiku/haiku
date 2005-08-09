/*
 * Copyright 2001-2005, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Erik Jaesler (erik@cgsoftware.com)
 *		DarkWyrm <bpmagic@columbus.rr.com>
 *		Ingo Weinhold <bonefish@users.sf.net>
 *		Michael Lotz <mmlr@mlotz.ch>
 */

/**	BMessage class creates objects that store data and that
 *	can be processed in a message loop.  BMessage objects
 *	are also used as data containers by the archiving and
 *	the scripting mechanisms.
 */

// debugging
//#define DBG(x) x
#define DBG(x)	;
#define PRINT(x)	DBG({ printf("[%6ld] ", find_thread(NULL)); printf x; })

#include <stdio.h>
#include <ctype.h>

#include <Message3.h>
#include <Application.h>
#include <BlockCache.h>
#include <ByteOrder.h>
#include <DataIO.h>
#include <Errors.h>
#include <Messenger.h>
#include <MessengerPrivate.h>
#include <String.h>

#include <AppMisc.h>
#include <KMessage.h>
#include <TokenSpace.h>

#include "MessageBody3.h"
#include "MessageField3.h"
#include "MessageUtils3.h"
#include "dano_message.h"

// flags for the overall message (the bitfield is 1 byte)
#define MSG_FLAG_BIG_ENDIAN		0x01
#define MSG_FLAG_INCL_TARGET	0x02
#define MSG_FLAG_INCL_REPLY		0x04
#define MSG_FLAG_SCRIPT_MSG		0x08

#if 0
// These are for future improvement
#define MSG_FLAG_USE_PREFERRED	0x10
#define MSG_FLAG_REPLY_WANTED	0x20
#define MSG_FLAG_REPLY_DONE		0x40
#define MSG_FLAG_IS_REPLY		0x80

#define MSG_FLAG_HDR_MASK		0xF0
#endif

#define MSG_HEADER_MAX_SIZE		38
#define MSG_NAME_MAX_SIZE		256

using namespace BPrivate;

static const uint32 kMessageMagic = 'FOB1';
static const uint32 kMessageMagicSwapped = '1BOF';
static const uint32 kMessageMagicDano = 'FOB2';
static const uint32 kMessageMagicDanoSwapped = '2BOF';

const char* B_SPECIFIER_ENTRY = "specifiers";
const char* B_PROPERTY_ENTRY = "property";
const char* B_PROPERTY_NAME_ENTRY = "name";

BBlockCache *BMessage::sMsgCache = NULL;
port_id BMessage::sReplyPorts[sNumReplyPorts];
long BMessage::sReplyPortInUse[sNumReplyPorts];

static status_t		handle_reply(port_id replyPort, int32 *pCode,
						bigtime_t timeout, BMessage *reply);
static status_t		convert_message(const KMessage *fromMessage,
						BMessage *toMessage);
static ssize_t		min_hdr_size();


class BMessage::Header {
public:
					Header() {};
					Header(const BMessage &message) { ReadFrom(message); };

		status_t	SetMagic(uint32 magic);

		status_t	ReadFrom(BDataIO &stream);
		void		ReadFrom(const BMessage &message);
		status_t	WriteTo(BDataIO &stream, bool calculateCheckSum = true) const;
		void		WriteTo(BMessage &message) const;

		uint32		CalculateCheckSum() const;
		uint32		CalculateHeaderSize() const;
		uint32		CalculatedBodySize() const;

		bool		IsSwapped() const { return fSwapped; };
		bool		HasTarget() const { return (fFlags & MSG_FLAG_INCL_TARGET); };
		void		SetTarget(int32 token, bool preferred);

		void		Dump() const;

private:
		uint32		fMagic;
		int32		fBodySize;
		uint32		fWhat;
		uint8		fFlags;
		int32		fTargetToken;
		port_id		fReplyPort;
		int32		fReplyToken;
		team_id		fReplyTeam;
		bool		fPreferredTarget;
		bool		fReplyRequired;
		bool		fReplyDone;
		bool		fIsReply;
		bool		fSwapped;
};


status_t
BMessage::Header::SetMagic(uint32 magic)
{
	if (magic == kMessageMagicSwapped)
		fSwapped = true;
	else if (magic == kMessageMagic)
		fSwapped = false;
	else {
		// This is *not* a message
		return B_NOT_A_MESSAGE;
	}

	fMagic = magic;
	return B_OK;
}


status_t
BMessage::Header::ReadFrom(BDataIO &stream)
{
	int32 checkSum;
	uchar csBuffer[MSG_HEADER_MAX_SIZE];

	TReadHelper read_helper(&stream);
	TChecksumHelper checksum_helper(csBuffer);
	int32 flattenedSize;

	try {
		read_helper.SetSwap(fSwapped);

		// get the checksum
		read_helper(checkSum);
		// get the size
		read_helper(flattenedSize);
		// get the what
		read_helper(fWhat);
		// get the flags
		read_helper(fFlags);

		checksum_helper.Cache(flattenedSize);
		checksum_helper.Cache(fWhat);
		checksum_helper.Cache(fFlags);

		if (fFlags & MSG_FLAG_BIG_ENDIAN) {
			// TODO: isn't this already indicated by the byte order of the
			//	message version?
		}

		if (fFlags & MSG_FLAG_INCL_TARGET) {
			// Get the target data
			read_helper(fTargetToken);
			checksum_helper.Cache(fTargetToken);
		}

		if (fFlags & MSG_FLAG_INCL_REPLY) {
			// Get the reply port
			read_helper(fReplyPort);
			read_helper(fReplyToken);
			read_helper(fReplyTeam);
			checksum_helper.Cache(fReplyPort);
			checksum_helper.Cache(fReplyToken);
			checksum_helper.Cache(fReplyTeam);

			// get the "big flags"
			uint8 bigFlags;
			// get the preferred flag
			read_helper(bigFlags);
			checksum_helper.Cache(bigFlags);
			fPreferredTarget = bigFlags;

			// get the reply requirement flag
			read_helper(bigFlags);
			checksum_helper.Cache(bigFlags);
			fReplyRequired = bigFlags;

			// get the reply done flag
			read_helper(bigFlags);
			checksum_helper.Cache(bigFlags);
			fReplyDone = bigFlags;

			// Get the "is reply" flag
			read_helper(bigFlags);
			checksum_helper.Cache(bigFlags);
			fIsReply = bigFlags;
		}
	} catch (status_t &error) {
		return error;
	}

	fBodySize = flattenedSize - CalculateHeaderSize();

	if (checkSum != checksum_helper.CheckSum())
		return B_NOT_A_MESSAGE;

	return B_OK;
}


void
BMessage::Header::ReadFrom(const BMessage &message)
{
	fMagic = kMessageMagic;

	fBodySize = message.fBody->FlattenedSize();
	fWhat = message.what;
	fFlags = 0;
#ifdef B_HOST_IS_BENDIAN
	fFlags |= MSG_FLAG_BIG_ENDIAN;
#endif
	if (message.HasSpecifiers())
		fFlags |= MSG_FLAG_SCRIPT_MSG;

	if (message.fTarget != B_NULL_TOKEN)
		fFlags |= MSG_FLAG_INCL_TARGET;

	if (message.fReplyTo.port >= 0 &&
		message.fReplyTo.target != B_NULL_TOKEN &&
		message.fReplyTo.team >= 0) {
		fFlags |= MSG_FLAG_INCL_REPLY;
	}

	fTargetToken = message.fTarget;
	fReplyPort = message.fReplyTo.port;
	fReplyToken = message.fReplyTo.target;
	fReplyTeam = message.fReplyTo.team;
	fPreferredTarget = message.fPreferred;
	fReplyRequired = message.fReplyRequired;
	fReplyDone = message.fReplyDone;
	fIsReply = message.fIsReply;
}


status_t
BMessage::Header::WriteTo(BDataIO &stream, bool calculateCheckSum) const
{
	try {
		// write the version of the binary data format
		int32 data = fMagic;
		write_helper(&stream, (const void *)&data, sizeof(data));

		// compute checksum
		data = (calculateCheckSum ? CalculateCheckSum() : 0);
		write_helper(&stream, (const void *)&data, sizeof(data));

		// write the flattened size of the entire message
		data = CalculateHeaderSize() + fBodySize;
		write_helper(&stream, (const void *)&data, sizeof(data));

		// write the 'what' member
		write_helper(&stream, (const void *)&fWhat, sizeof(fWhat));

		// write the header flags
		write_helper(&stream, (const void *)&fFlags, sizeof(fFlags));

		// write targeting info if necessary
		if (fFlags & MSG_FLAG_INCL_TARGET) {
			data = fPreferredTarget ? B_PREFERRED_TOKEN : fTargetToken;
			write_helper(&stream, (const void *)&data, sizeof(data));
		}

		// write reply info if necessary
		if (fFlags & MSG_FLAG_INCL_REPLY) {
			write_helper(&stream, (const void *)&fReplyPort, sizeof(fReplyPort));
			write_helper(&stream, (const void *)&fReplyToken, sizeof(fReplyToken));
			write_helper(&stream, (const void *)&fReplyTeam, sizeof(fReplyTeam));

			uint8 bigFlags;
			bigFlags = fPreferredTarget ? 1 : 0;
			write_helper(&stream, (const void *)&bigFlags, sizeof(bigFlags));

			bigFlags = fReplyRequired ? 1 : 0;
			write_helper(&stream, (const void *)&bigFlags, sizeof(bigFlags));

			bigFlags = fReplyDone ? 1 : 0;
			write_helper(&stream, (const void *)&bigFlags, sizeof(bigFlags));

			bigFlags = fIsReply ? 1 : 0;
			write_helper(&stream, (const void *)&bigFlags, sizeof(bigFlags));
		}
	} catch (status_t &error) {
		return error;
	}

	return B_OK;
}


void
BMessage::Header::WriteTo(BMessage &message) const
{
	// make way for the new data
	message.MakeEmpty();
	message.what = fWhat;

	message.fHasSpecifiers = fFlags & MSG_FLAG_SCRIPT_MSG;

	if (fFlags & MSG_FLAG_INCL_TARGET) {
		// get the target data
		message.fTarget = fTargetToken;
	}

	if (fFlags & MSG_FLAG_INCL_REPLY) {
		// get the reply port
		message.fReplyTo.port = fReplyPort;
		message.fReplyTo.target = fReplyToken;
		message.fReplyTo.team = fReplyTeam;

		message.fWasDelivered = true;

		message.fPreferred = fPreferredTarget;
		if (fPreferredTarget)
			message.fTarget = B_PREFERRED_TOKEN;

		message.fReplyRequired = fReplyRequired;
		message.fReplyDone = fReplyDone;
		message.fIsReply = fIsReply;
	}
}


uint32
BMessage::Header::CalculateCheckSum() const
{
	uint8 buffer[MSG_HEADER_MAX_SIZE];
	BMemoryIO stream(buffer, sizeof(buffer));
	WriteTo(stream, false);
	int32 size = stream.Position();
	return _checksum_(buffer + 8, size - 8);
}


uint32
BMessage::Header::CalculateHeaderSize() const
{
	ssize_t size = min_hdr_size();

	if (fTargetToken != B_NULL_TOKEN)
		size += sizeof(fTargetToken);

	if (fReplyPort >= 0 && fReplyToken != B_NULL_TOKEN && fReplyTeam >= 0) {
		size += sizeof(fReplyPort);
		size += sizeof(fReplyToken);
		size += sizeof(fReplyTeam);

		size += 4;	// For the "big" flags
	}

	return size;
}


uint32
BMessage::Header::CalculatedBodySize() const
{
	return fBodySize;
}


void
BMessage::Header::SetTarget(int32 token, bool preferred)
{
	fTargetToken = token;

	if (fTargetToken == B_NULL_TOKEN)
		fFlags &= ~MSG_FLAG_INCL_TARGET;
	else
		fFlags |= MSG_FLAG_INCL_TARGET;

	fPreferredTarget = preferred;
}


void
BMessage::Header::Dump() const
{
	printf("BMessage::Header:\n");
	printf("  magic:            %lx\n", fMagic);
	printf("  body size:        %ld\n", fBodySize);
	printf("  what:             %lx\n", fWhat);
	printf("  flags:            %x\n", fFlags);
	printf("  target token:     %ld\n", fTargetToken);
	printf("  reply port:       %ld\n", fReplyPort);
	printf("  reply token:      %ld\n", fReplyToken);
	printf("  reply team:       %ld\n", fReplyTeam);
	printf("  preferred target: %d\n", fPreferredTarget);
	printf("  reply required:   %d\n", fReplyRequired);
	printf("  reply done:       %d\n", fReplyDone);
	printf("  is reply:         %d\n", fIsReply);
	printf("  swapped:          %d\n", fSwapped);
}


void BMessage::_ReservedMessage1() {}
void BMessage::_ReservedMessage2() {}
void BMessage::_ReservedMessage3() {}


BMessage::BMessage()
	:	fBody(NULL)
{
	init_data();
}


BMessage::BMessage(uint32 _what)
	:	fBody(NULL)
{
	init_data();
	what = _what;
}


BMessage::BMessage(const BMessage &other)
	:	fBody(NULL)
{
	init_data();
	*this = other;
}


BMessage::BMessage(BMessage *other)
	:	fBody(NULL)
{
	init_data();
	*this = *other;
}


BMessage::~BMessage()
{
	if (IsSourceWaiting()) {
		SendReply(B_NO_REPLY);
	}

	delete fBody;
}


BMessage &
BMessage::operator=(const BMessage &msg)
{
	what = msg.what;

	link = msg.link;
	fTarget = msg.fTarget;
	fOriginal = msg.fOriginal;
	fChangeCount = msg.fChangeCount;
	fCurSpecifier = msg.fCurSpecifier;
	fPtrOffset = msg.fPtrOffset;

	fEntries = msg.fEntries;

	fReplyTo.port = msg.fReplyTo.port;
	fReplyTo.target = msg.fReplyTo.target;
	fReplyTo.team = msg.fReplyTo.team;
	fReplyTo.preferred = msg.fReplyTo.preferred;

	fPreferred = msg.fPreferred;
	fReplyRequired = msg.fReplyRequired;
	fReplyDone = msg.fReplyDone;
	fIsReply = msg.fIsReply;
	fWasDelivered = msg.fWasDelivered;
	fReadOnly = msg.fReadOnly;
	fHasSpecifiers = msg.fHasSpecifiers;

	*fBody = *(msg.fBody);
	return *this;
}


void
BMessage::init_data()
{
	what = 0;

	link = NULL;
	fTarget = B_NULL_TOKEN;
	fOriginal = NULL;
	fChangeCount = 0;
	fCurSpecifier = -1;
	fPtrOffset = 0;

	fEntries = NULL;

	fReplyTo.port = -1;
	fReplyTo.target = B_NULL_TOKEN;
	fReplyTo.team = -1;
	fReplyTo.preferred = false;

	fPreferred = false;
	fReplyRequired = false;
	fReplyDone = false;
	fIsReply = false;
	fWasDelivered = false;
	fReadOnly = false;
	fHasSpecifiers = false;

	if (fBody)
		fBody->MakeEmpty();
	else
		fBody = new BPrivate::BMessageBody();
}


status_t
BMessage::GetInfo(type_code typeRequested, int32 which, char **name,
	type_code *typeReturned, int32 *count) const
{
	return fBody->GetInfo(typeRequested, which, name, typeReturned, count);
}


status_t
BMessage::GetInfo(const char *name, type_code *type, int32 *count) const
{
	return fBody->GetInfo(name, type, count);
}


status_t
BMessage::GetInfo(const char *name, type_code *type, bool *fixed_size) const
{
	return fBody->GetInfo(name, type, fixed_size);
}


int32
BMessage::CountNames(type_code type) const
{
	return fBody->CountNames(type);
}


bool
BMessage::IsEmpty() const
{
	return fBody->IsEmpty();
}


bool
BMessage::IsSystem() const
{
	char a = char(what >> 24);
	char b = char(what >> 16);
	char c = char(what >> 8);
	char d = char(what);

	// The BeBook says:
	//		... we've adopted a strict convention for assigning values to all
	//		Be-defined constants.  The value assigned will always be formed by
	//		combining four characters into a multicharacter constant, with the
	//		characters limited to uppercase letters and the underbar
	// Between that and what's in AppDefs.h, this algo seems like a safe bet:
	if (a == '_' && isupper(b) && isupper(c) && isupper(d))
		return true;

	return false;
}


bool
BMessage::IsReply() const
{
	return fIsReply;
}


void
BMessage::PrintToStream() const
{
	printf("\nBMessage: what = ");
	printf("%c%c%c%c", (uint8)(what >> 24), (uint8)(what >> 16),
		(uint8)(what >> 8), (uint8)what);
	printf(" (0x%lX or %ld)\n", what, what);

	fBody->PrintToStream();
}


status_t
BMessage::Rename(const char *old_entry, const char *new_entry)
{
	return fBody->Rename(old_entry, new_entry);
}


bool
BMessage::WasDelivered() const
{
	return fWasDelivered;
}


bool
BMessage::IsSourceWaiting() const
{
	return fReplyRequired && !fReplyDone;
}


bool
BMessage::IsSourceRemote() const
{
	return WasDelivered() && fReplyTo.team != BPrivate::current_team();
}


BMessenger
BMessage::ReturnAddress() const
{
	if (WasDelivered()) {
		BMessenger messenger;
		BMessenger::Private(messenger).SetTo(fReplyTo.team, fReplyTo.port,
			fReplyTo.target, fReplyTo.preferred);
		return messenger;
	}

	return BMessenger();
}


const BMessage *
BMessage::Previous() const
{
	// TODO: test
	// In particular, look to see if the "_previous_" field is used in R5
	if (!fOriginal) {
		BMessage *fOriginal = new BMessage;

		if (FindMessage("_previous_", fOriginal) != B_OK) {
			delete fOriginal;
			fOriginal = NULL;
		}
	}

	return fOriginal;
}


bool
BMessage::WasDropped() const
{
	return fReadOnly;
}


BPoint
BMessage::DropPoint(BPoint *offset) const
{
	// TODO: Where do we get this stuff???
	if (offset)
		*offset = FindPoint("_drop_offset_");

	return FindPoint("_drop_point_");
}


status_t
BMessage::SendReply(uint32 command, BHandler *reply_to)
{
	BMessage msg(command);
	return SendReply(&msg, reply_to);
}


status_t
BMessage::SendReply(BMessage *the_reply, BHandler *reply_to, bigtime_t timeout)
{
	BMessenger messenger(reply_to);
	return SendReply(the_reply, messenger, timeout);
}


status_t
BMessage::SendReply(BMessage *the_reply, BMessenger reply_to, bigtime_t timeout)
{
	// TODO: test
	BMessenger messenger;
	BMessenger::Private messengerPrivate(messenger);
	messengerPrivate.SetTo(fReplyTo.team, fReplyTo.port, fReplyTo.target,
		fReplyTo.preferred);

	if (fReplyRequired) {
		if (fReplyDone)
			return B_DUPLICATE_REPLY;

		fReplyDone = true;
		the_reply->fIsReply = true;
		status_t error = messenger.SendMessage(the_reply, reply_to, timeout);
		the_reply->fIsReply = false;

		if (error) {
			if (set_port_owner(messengerPrivate.Port(),
				messengerPrivate.Team()) == B_BAD_TEAM_ID) {
				delete_port(messengerPrivate.Port());
			}
		}

		return error;
	}

	// no reply required
	if (!fWasDelivered)
		return B_BAD_REPLY;

	the_reply->AddMessage("_previous_", this);
	the_reply->fIsReply = true;

	status_t error = messenger.SendMessage(the_reply, reply_to, timeout);
	the_reply->fIsReply = false;
	the_reply->RemoveName("_previous_");

	return error;
}


status_t
BMessage::SendReply(uint32 command, BMessage *reply_to_reply)
{
	BMessage msg(command);
	return SendReply(&msg, reply_to_reply);
}


status_t
BMessage::SendReply(BMessage *the_reply, BMessage *reply_to_reply,
	bigtime_t send_timeout, bigtime_t reply_timeout)
{
	// TODO: test
	BMessenger messenger;
	BMessenger::Private messengerPrivate(messenger);
	messengerPrivate.SetTo(fReplyTo.team, fReplyTo.port, fReplyTo.target,
		fReplyTo.preferred);

	if (fReplyRequired) {
		if (fReplyDone)
			return B_DUPLICATE_REPLY;

		fReplyDone = true;
		the_reply->fIsReply = true;
		status_t error = messenger.SendMessage(the_reply, reply_to_reply,
			send_timeout, reply_timeout);
		the_reply->fIsReply = false;

		if (error) {
			if (set_port_owner(messengerPrivate.Port(),
				messengerPrivate.Team()) == B_BAD_TEAM_ID) {
				delete_port(messengerPrivate.Port());
			}
		}

		return error;
	}

	// no reply required
	if (!fWasDelivered)
		return B_BAD_REPLY;

	the_reply->AddMessage("_previous_", this);
	the_reply->fIsReply = true;
	status_t error = messenger.SendMessage(the_reply, reply_to_reply,
		send_timeout, reply_timeout);
	the_reply->fIsReply = false;
	the_reply->RemoveName("_previous_");
	return error;
}


ssize_t
BMessage::FlattenedSize() const
{
	return calc_hdr_size(0) + fBody->FlattenedSize();
}


status_t
BMessage::Flatten(char *buffer, ssize_t size) const
{
	return real_flatten(buffer, size);
}


status_t
BMessage::Flatten(BDataIO *stream, ssize_t *size) const
{
	return real_flatten(stream, size);
}


status_t
BMessage::Unflatten(const char *buffer)
{
	if (!buffer)
		return B_BAD_VALUE;

	uint32 magic = *(uint32 *)buffer;

	// we support several message formats - this list is ordered
	// by importance and frequency

	if (magic == kMessageMagic) {
		// it appears to be a normal flattened BMessage
		BMemoryIO memoryStream(buffer, ((uint32 *)buffer)[2]);
		return Unflatten(&memoryStream);
	}

	// check whether this is a KMessage
	if (((KMessage::Header *)buffer)->magic == KMessage::kMessageHeaderMagic)
		return _UnflattenKMessage(buffer);

	if (magic == kMessageMagicSwapped) {
		// it appears to be a swapped flattened BMessage
		uint32 size = ((uint32 *)buffer)[2];
		BMemoryIO memoryStream(buffer, __swap_int32(size));
		return Unflatten(&memoryStream);
	}

	if (magic == kMessageMagicDano || magic == kMessageMagicDanoSwapped) {
		// dano style message
		BMemoryIO memoryStream(buffer, BPrivate::dano_message_size(buffer));
		return Unflatten(&memoryStream);
	}

	return B_NOT_A_MESSAGE;
}


status_t
BMessage::Unflatten(BDataIO *stream)
{
	TReadHelper reader(stream);
	Header header;
	status_t status = B_OK;

	// ToDo: while reading from a stream is certainly more convenient than
	//	from a buffer, it causes a lot of unnecessary copies, and therefore
	//	probably shouldn't be the preferred (or only) route.

	try {
		uint32 magic;
		reader(magic);

		status = header.SetMagic(magic);
		if (status < B_OK) {
			// we support reading Dano messages from disk as well
			if (magic == kMessageMagicDano)
				return BPrivate::unflatten_dano_message(magic, *stream, *this);

			return status;
		}

		status = header.ReadFrom(*stream);
		if (status < B_OK) {
			printf("BMessage::Unflatten(): Reading the header failed: %lx\n", status);
			return status;
		}

		header.WriteTo(*this);

		status = fBody->Unflatten(stream, header.CalculatedBodySize());
	} catch (status_t &error) {
		status = error;
	}

	return status;
}


status_t
BMessage::AddSpecifier(const char *property)
{
	BMessage message(B_DIRECT_SPECIFIER);
	status_t error = message.AddString(B_PROPERTY_ENTRY, property);
	if (error)
		return error;

	return AddSpecifier(&message);
}


status_t
BMessage::AddSpecifier(const char *property, int32 index)
{
	BMessage message(B_INDEX_SPECIFIER);
	status_t error = message.AddString(B_PROPERTY_ENTRY, property);
	if (error)
		return error;

	error = message.AddInt32("index", index);
	if (error)
		return error;

	return AddSpecifier(&message);
}


status_t
BMessage::AddSpecifier(const char *property, int32 index, int32 range)
{
	if (range < 0)
		return B_BAD_VALUE;

	BMessage message(B_RANGE_SPECIFIER);
	status_t error = message.AddString(B_PROPERTY_ENTRY, property);
	if (error)
		return error;

	error = message.AddInt32("index", index);
	if (error)
		return error;

	error = message.AddInt32("range", range);
	if (error)
		return error;

	return AddSpecifier(&message);
}


status_t
BMessage::AddSpecifier(const char *property, const char *name)
{
	BMessage message(B_NAME_SPECIFIER);
	status_t error = message.AddString(B_PROPERTY_ENTRY, property);
	if (error)
		return error;

	error = message.AddString(B_PROPERTY_NAME_ENTRY, name);
	if (error)
		return error;

	return AddSpecifier(&message);
}


status_t
BMessage::AddSpecifier(const BMessage *specifier)
{
	status_t error = AddMessage(B_SPECIFIER_ENTRY, specifier);
	if (!error) {
		fCurSpecifier++;
		fHasSpecifiers = true;
	}

	return error;
}


status_t
BMessage::SetCurrentSpecifier(int32 index)
{
	type_code type;
	int32 count;
	status_t error = GetInfo(B_SPECIFIER_ENTRY, &type, &count);
	if (error)
		return error;

	if (index < 0 || index >= count)
		return B_BAD_INDEX;

	fCurSpecifier = index;
	return B_OK;
}


status_t
BMessage::GetCurrentSpecifier(int32 *index, BMessage *specifier, int32 *what,
	const char **property) const
{
	if (fCurSpecifier == -1 || !WasDelivered())
		return B_BAD_SCRIPT_SYNTAX;

	if (index)
		*index = fCurSpecifier;

	if (specifier) {
		if (FindMessage(B_SPECIFIER_ENTRY, fCurSpecifier, specifier))
			return B_BAD_SCRIPT_SYNTAX;

		if (what)
			*what = specifier->what;

		if (property) {
			if (specifier->FindString(B_PROPERTY_ENTRY, property))
				return B_BAD_SCRIPT_SYNTAX;
		}
	}

	return B_OK;
}


bool
BMessage::HasSpecifiers() const
{
	return fHasSpecifiers;
}


status_t
BMessage::PopSpecifier()
{
	if (fCurSpecifier < 0 || !WasDelivered())
		return B_BAD_VALUE;

	fCurSpecifier--;
	return B_OK;
}


#define DEFINE_FUNCTIONS(TYPE, fnName, TYPESPEC)							\
status_t																	\
BMessage::Add ## fnName(const char *name, TYPE val)							\
{																			\
	return fBody->AddData(name, TYPESPEC, &val, sizeof(TYPE), true);		\
}																			\
																			\
status_t																	\
BMessage::Find ## fnName(const char *name, TYPE *p) const					\
{																			\
	return Find ## fnName(name, 0, p);										\
}																			\
																			\
status_t																	\
BMessage::Find ## fnName(const char *name, int32 index, TYPE *p) const		\
{																			\
	void *ptr = NULL;														\
	ssize_t bytes = 0;														\
	status_t error = B_OK;													\
																			\
	*p = TYPE();															\
	error = fBody->FindData(name, TYPESPEC, index, (const void **)&ptr,		\
		&bytes);															\
																			\
	if (error == B_OK)														\
		memcpy(p, ptr, sizeof(TYPE));										\
																			\
	return error;															\
}																			\
																			\
status_t																	\
BMessage::Replace ## fnName(const char* name, TYPE val)						\
{																			\
	return fBody->ReplaceData(name, TYPESPEC, 0, &val, sizeof(TYPE));		\
}																			\
																			\
status_t																	\
BMessage::Replace ## fnName(const char *name, int32 index, TYPE val)		\
{																			\
	return fBody->ReplaceData(name, TYPESPEC, index, &val, sizeof(TYPE));	\
}																			\
																			\
bool																		\
BMessage::Has ## fnName(const char *name, int32 n) const					\
{																			\
	return fBody->HasData(name, TYPESPEC, n);								\
}

DEFINE_FUNCTIONS(int8, Int8, B_INT8_TYPE);
DEFINE_FUNCTIONS(int16, Int16, B_INT16_TYPE);
DEFINE_FUNCTIONS(int32, Int32, B_INT32_TYPE);
DEFINE_FUNCTIONS(int64, Int64, B_INT64_TYPE);
DEFINE_FUNCTIONS(BPoint, Point, B_POINT_TYPE);
DEFINE_FUNCTIONS(BRect, Rect, B_RECT_TYPE);
DEFINE_FUNCTIONS(float, Float, B_FLOAT_TYPE);
DEFINE_FUNCTIONS(double, Double, B_DOUBLE_TYPE);
DEFINE_FUNCTIONS(bool, Bool, B_BOOL_TYPE);

#undef DEFINE_FUNCTIONS

#define DEFINE_HAS_FUNCTION(fnName, TYPESPEC)								\
bool																		\
BMessage::Has ## fnName(const char* name, int32 n) const					\
{																			\
	return fBody->HasData(name, TYPESPEC, n);								\
}

DEFINE_HAS_FUNCTION(Message, B_MESSAGE_TYPE);
DEFINE_HAS_FUNCTION(String, B_STRING_TYPE);
DEFINE_HAS_FUNCTION(Pointer, B_POINTER_TYPE);
DEFINE_HAS_FUNCTION(Messenger, B_MESSENGER_TYPE);
DEFINE_HAS_FUNCTION(Ref, B_REF_TYPE);

#undef DEFINE_HAS_FUNCTION

#define DEFINE_LAZY_FIND_FUNCTION(TYPE, fnName)								\
TYPE																		\
BMessage::Find ## fnName(const char* name, int32 n) const					\
{																			\
	TYPE i = 0;																\
	Find ## fnName(name, n, &i);											\
	return i;																\
}

DEFINE_LAZY_FIND_FUNCTION(int8, Int8);
DEFINE_LAZY_FIND_FUNCTION(int16, Int16);
DEFINE_LAZY_FIND_FUNCTION(int32, Int32);
DEFINE_LAZY_FIND_FUNCTION(int64, Int64);
DEFINE_LAZY_FIND_FUNCTION(float, Float);
DEFINE_LAZY_FIND_FUNCTION(double, Double);
DEFINE_LAZY_FIND_FUNCTION(bool, Bool);
DEFINE_LAZY_FIND_FUNCTION(const char *, String);

#undef DEFINE_LAZY_FIND_FUNCTION


status_t
BMessage::AddString(const char *name, const char *string)
{
	return fBody->AddData(name, B_STRING_TYPE, string, strlen(string) + 1, false);
}


status_t
BMessage::AddString(const char* name, const BString &string)
{
	return fBody->AddData(name, B_STRING_TYPE, string.String(), string.Length() + 1, false);
}


status_t
BMessage::AddPointer(const char *name, const void *pointer)
{
	return fBody->AddData(name, B_POINTER_TYPE, &pointer, sizeof(pointer), true);
}


status_t
BMessage::AddMessenger(const char *name, BMessenger messenger)
{
	return fBody->AddData(name, B_MESSENGER_TYPE, &messenger, sizeof(messenger), true);
}


status_t
BMessage::AddRef(const char* name, const entry_ref* ref)
{
	size_t size = sizeof(entry_ref) + B_PATH_NAME_LENGTH;
	char buffer[size];

	status_t error = entry_ref_flatten(buffer, &size, ref);

	if (error >= B_OK)
		error = fBody->AddData(name, B_REF_TYPE, buffer, size, false);

	return error;
}


status_t
BMessage::AddMessage(const char *name, const BMessage *msg)
{
	ssize_t size = msg->FlattenedSize();
	void *buffer = NULL;

	status_t error = fBody->AddData(name, B_MESSAGE_TYPE, &buffer, size);

	if (error >= B_OK)
		error = msg->Flatten((char *)buffer, size);

	return error;
}


status_t
BMessage::AddFlat(const char *name, BFlattenable *object, int32 count)
{
	ssize_t size = object->FlattenedSize();
	void *buffer = NULL;

	status_t error = fBody->AddData(name, object->TypeCode(), &buffer, size);

	if (error >= B_OK)
		error = object->Flatten(buffer, size);

	return error;
}


status_t
BMessage::AddData(const char *name, type_code type, const void *data,
	ssize_t numBytes, bool is_fixed_size, int32 /*count*/)
{
	return fBody->AddData(name, type, data, numBytes, is_fixed_size);
}


status_t
BMessage::RemoveData(const char *name, int32 index)
{
	return fReadOnly ? B_ERROR : fBody->RemoveData(name, index);
}


status_t
BMessage::RemoveName(const char *name)
{
	return fReadOnly ? B_ERROR : fBody->RemoveName(name);
}


status_t
BMessage::MakeEmpty()
{
	return fReadOnly ? B_ERROR : fBody->MakeEmpty();
}


status_t
BMessage::FindString(const char *name, const char **string) const
{
	return FindString(name, 0, string);
}


status_t
BMessage::FindString(const char *name, int32 index, const char **string) const
{
	ssize_t bytes;
	return FindData(name, B_STRING_TYPE, index,
		(const void**)string, &bytes);
}


status_t
BMessage::FindString(const char *name, BString *string) const
{
	return FindString(name, 0, string);
}


status_t
BMessage::FindString(const char *name, int32 index, BString *string) const
{
	const char *cstr;
	status_t err = FindString(name, index, &cstr);
	if (err < B_OK)
		return err;

	*string = cstr;
	return B_OK;
}


status_t
BMessage::FindPointer(const char *name, void **ptr) const
{
	return FindPointer(name, 0, ptr);
}


status_t
BMessage::FindPointer(const char *name, int32 index, void **ptr) const
{
	void **data = NULL;
	ssize_t size = 0;
	status_t error = FindData(name, B_POINTER_TYPE, index, (const void  **)&data, &size);

	if (error == B_OK)
		*ptr = *data;
	else
		*ptr = NULL;

	return error;
}


status_t
BMessage::FindMessenger(const char *name, BMessenger *m) const
{
	return FindMessenger(name, 0, m);
}


status_t
BMessage::FindMessenger(const char *name, int32 index, BMessenger *m) const
{
	void *data = NULL; 
	ssize_t size = 0;
	status_t error = FindData(name, B_MESSENGER_TYPE, index, (const void **)&data, &size);

	if (error == B_OK)
		memcpy(m, data, sizeof(BMessenger));
	else
		*m = BMessenger();

	return error;
}


status_t
BMessage::FindRef(const char *name, entry_ref *ref) const
{
	return FindRef(name, 0, ref);
}


status_t
BMessage::FindRef(const char *name, int32 index, entry_ref *ref) const
{
	void *data = NULL;
	ssize_t size = 0;
	status_t error = FindData(name, B_REF_TYPE, index, (const void **)&data, &size);

	if (error == B_OK)
		error = entry_ref_unflatten(ref, (char *)data, size);
	else
		*ref = entry_ref();

	return error;
}


status_t
BMessage::FindMessage(const char *name, BMessage *msg) const
{
	return FindMessage(name, 0, msg);
}


status_t
BMessage::FindMessage(const char *name, int32 index, BMessage *msg) const
{
	void *data = NULL;
	ssize_t size = 0;
	status_t error = FindData(name, B_MESSAGE_TYPE, index, (const void **)&data, &size);

	if (!error)
		error = msg->Unflatten((const char *)data);
	else
		*msg = BMessage();

	return error;
}


status_t
BMessage::FindFlat(const char *name, BFlattenable *obj) const
{
	return FindFlat(name, 0, obj);
}


status_t
BMessage::FindFlat(const char *name, int32 index, BFlattenable *obj) const
{
	void *data = NULL;
	ssize_t numBytes = 0;
	status_t error = FindData(name, obj->TypeCode(), index, (const void **)&data, &numBytes);

	if (!error)
		error = obj->Unflatten(obj->TypeCode(), data, numBytes);

	return error;
}


status_t
BMessage::FindData(const char *name, type_code type, const void **data,
	ssize_t *numBytes) const
{
	return fBody->FindData(name, type, 0, data, numBytes);
}


status_t
BMessage::FindData(const char *name, type_code type, int32 index, 
	const void **data, ssize_t *numBytes) const
{
	return fBody->FindData(name, type, index, data, numBytes);
}


BRect
BMessage::FindRect(const char *name, int32 n) const
{
	BRect r(0, 0, -1, -1);
	FindRect(name, n, &r);
	return r;
}


BPoint
BMessage::FindPoint(const char *name, int32 n) const
{
	BPoint p(0, 0);
	FindPoint(name, n, &p);
	return p;
}


status_t
BMessage::ReplaceString(const char *name, const char *string)
{
	return ReplaceString(name, 0, string);
}


status_t
BMessage::ReplaceString(const char *name, int32 index, const char *string)
{
	return ReplaceData(name, B_STRING_TYPE, index, string, strlen(string)+1);
}


status_t
BMessage::ReplaceString(const char *name, const BString &string)
{
	return ReplaceString(name, 0, string);
}


status_t
BMessage::ReplaceString(const char *name, int32 index, const BString &string)
{
	return ReplaceData(name, B_STRING_TYPE, index, string.String(), string.Length()+1);
}


status_t
BMessage::ReplacePointer(const char *name, const void *ptr)
{
	return ReplacePointer(name, 0, ptr);
}


status_t
BMessage::ReplacePointer(const char *name, int32 index, const void *ptr)
{
	return ReplaceData(name, B_POINTER_TYPE, index, &ptr, sizeof(ptr));
}


status_t
BMessage::ReplaceMessenger(const char *name, BMessenger messenger)
{
	return ReplaceData(name, B_MESSENGER_TYPE, 0, &messenger, sizeof(BMessenger));
}


status_t
BMessage::ReplaceMessenger(const char *name, int32 index, BMessenger messenger)
{
	return ReplaceData(name, B_MESSENGER_TYPE, index, &messenger, sizeof(BMessenger));
}


status_t
BMessage::ReplaceRef(const char *name, const entry_ref *ref)
{
	return ReplaceRef(name, 0, ref);
}


status_t
BMessage::ReplaceRef(const char *name, int32 index, const entry_ref *ref)
{
	size_t size = sizeof(entry_ref) + B_PATH_NAME_LENGTH;
	char buffer[size];

	status_t error = entry_ref_flatten(buffer, &size, ref);
	

	if (error >= B_OK)
		error = fBody->ReplaceData(name, B_REF_TYPE, index, &buffer, size);

	return error;
}


status_t
BMessage::ReplaceMessage(const char *name, const BMessage *msg)
{
	return ReplaceMessage(name, 0, msg);
}


status_t
BMessage::ReplaceMessage(const char *name, int32 index, const BMessage *msg)
{
	ssize_t size = msg->FlattenedSize();
	void *buffer = NULL;

	status_t error = fBody->ReplaceData(name, B_MESSAGE_TYPE, index, &buffer, size);

	if (error >= B_OK)
		error = msg->Flatten((char *)buffer, size);

	return error;
}


status_t
BMessage::ReplaceFlat(const char *name, BFlattenable *object)
{
	return ReplaceFlat(name, 0, object);
}


status_t
BMessage::ReplaceFlat(const char *name, int32 index, BFlattenable *object)
{
	ssize_t size = object->FlattenedSize();
	void *buffer = NULL;

	status_t error = fBody->ReplaceData(name, object->TypeCode(), index,
		&buffer, size);

	if (error >= B_OK)
		error = object->Flatten(buffer, size);

	return error;
}


status_t
BMessage::ReplaceData(const char *name, type_code type, const void *data,
	ssize_t data_size)
{
	return fBody->ReplaceData(name, type, 0, data, data_size);
}


status_t
BMessage::ReplaceData(const char *name, type_code type, int32 index,
	const void *data, ssize_t data_size)
{
	return fBody->ReplaceData(name, type, index, data, data_size);
}


bool
BMessage::HasFlat(const char *name, const BFlattenable *flat) const
{
	return HasFlat(name, 0, flat);
}


bool
BMessage::HasFlat(const char *name, int32 n, const BFlattenable *flat) const
{
	return fBody->HasData(name, flat->TypeCode(), n);
}


bool
BMessage::HasData(const char *name, type_code t, int32 n) const
{
	return fBody->HasData(name, t, n);
}


void *
BMessage::operator new(size_t size)
{
	if (!sMsgCache)
		sMsgCache = new BBlockCache(10, size, B_OBJECT_CACHE);

	return sMsgCache->Get(size);
}


void *
BMessage::operator new(size_t, void *p)
{
	return p;
}


void
BMessage::operator delete(void *ptr, size_t size)
{
	sMsgCache->Save(ptr, size);
}


status_t
BMessage::real_flatten(char *result, ssize_t size) const
{
	BMemoryIO stream((void *)result, size);
	return real_flatten(&stream, NULL);
}


status_t
BMessage::real_flatten(BDataIO *stream, ssize_t *size) const
{
	Header header(*this);

	status_t error = header.WriteTo(*stream);
	if (error >= B_OK)
		error = fBody->Flatten(stream);

	if (size)
		*size = FlattenedSize();

	return error;
}


char *
BMessage::stack_flatten(char *stack_ptr, ssize_t stack_size,
	bool /*incl_reply*/, ssize_t *size) const
{
	const ssize_t calcd_size = calc_hdr_size(0) + fBody->FlattenedSize();
	char *new_ptr = NULL;
	if (calcd_size > stack_size) {
		stack_ptr = new char[calcd_size];
		new_ptr = stack_ptr;
	}

	real_flatten(stack_ptr, calcd_size);
	if (size)
		*size = calcd_size;

	return new_ptr;
}


status_t
BMessage::_UnflattenKMessage(const char *buffer)
{
	// init a real KMessage
	KMessage message;
	status_t error = message.SetTo(buffer, ((KMessage::Header*)buffer)->size);
	if (error != B_OK)
		return error;

	// let convert_message() do the real job
	return convert_message(&message, this);
}


ssize_t
BMessage::calc_hdr_size(uchar flags) const
{
	ssize_t size = min_hdr_size();

	if (fTarget != B_NULL_TOKEN)
		size += sizeof(fTarget);

	if (fReplyTo.port >= 0
		&& fReplyTo.target != B_NULL_TOKEN
		&& fReplyTo.team >= 0) {
		size += sizeof(fReplyTo.port);
		size += sizeof(fReplyTo.target);
		size += sizeof(fReplyTo.team);

		size += 4;	// For the "big" flags
	}

	return size;
}


status_t
BMessage::_send_(port_id port, int32 token, bool preferred, bigtime_t timeout,
	bool reply_required, BMessenger &reply_to) const
{
	PRINT(("BMessage::_send_(port: %ld, token: %ld, preferred: %d): "
		"what: %lx (%.4s)\n", port, token, preferred, what, (char *)&what));

	bool oldPreferred = fPreferred;
	int32 oldTarget = fTarget;
	reply_to_info oldReplyTo = fReplyTo;

	if (!reply_to.IsValid()) {
		BMessenger::Private(reply_to).SetTo(fReplyTo.team,
			fReplyTo.port, fReplyTo.target, fReplyTo.preferred);
		if (!reply_to.IsValid())
			reply_to = be_app_messenger;
	}

	BMessage *self = const_cast<BMessage *>(this);
	BMessenger::Private replyToPrivate(reply_to);
	self->fPreferred = preferred;
	self->fTarget = token;
	self->fReplyRequired = reply_required;
	self->fReplyTo.team = replyToPrivate.Team();
	self->fReplyTo.port = replyToPrivate.Port();
	self->fReplyTo.target = (replyToPrivate.IsPreferredTarget()
							? B_PREFERRED_TOKEN : replyToPrivate.Token());
	self->fReplyTo.preferred = replyToPrivate.IsPreferredTarget();

	char tmp[0x800];
	ssize_t size;
	char *p = stack_flatten(tmp, sizeof(tmp), true /* include reply */, &size);
	char *pMem = p ? p : tmp;
	status_t error;

	do {
		error = write_port_etc(port, 'pjpp', pMem, size, B_RELATIVE_TIMEOUT, timeout);
	} while (error == B_INTERRUPTED);

	if (p)
		delete[] p;

	self->fPreferred = oldPreferred;
	self->fTarget = oldTarget;
	self->fReplyRequired = false;	// To this copy, no reply is required.
									// Only relevant when forwarding anyway.
	self->fReplyTo = oldReplyTo;

	PRINT(("BMessage::_send_() done: %lx\n", error));
	return error;
}


status_t
BMessage::send_message(port_id port, team_id port_owner, int32 token,
	bool preferred, BMessage *reply, bigtime_t send_timeout,
	bigtime_t reply_timeout) const
{
	const int32 cached_reply_port = sGetCachedReplyPort();
	port_id reply_port;
	status_t error;

	if (cached_reply_port == -1) {
		// All the cached reply ports are in use; create a new one
		reply_port = create_port(1 /* for one message */, "tmp_reply_port");
		if (reply_port < 0)
			return reply_port;
	} else {
		assert(cached_reply_port < sNumReplyPorts);
		reply_port = sReplyPorts[cached_reply_port];
	}

	team_id team = B_BAD_TEAM_ID;
	if (be_app != NULL)
		team = be_app->Team();
	else {
		port_info pi;
		error = get_port_info(reply_port, &pi);
		if (error)
			goto error;

		team = pi.team;
	}

	error = set_port_owner(reply_port, port_owner);
	if (error)
		goto error;

	{
		BMessenger messenger;
		BMessenger::Private(messenger).SetTo(team, reply_port,
			B_PREFERRED_TOKEN, false);
		error = _send_(port, token, preferred, send_timeout, true, messenger);
	}

	if (error)
		goto error;

	int32 code;
	error = handle_reply(reply_port, &code, reply_timeout, reply);
	if (error && cached_reply_port >= 0) {
		delete_port(reply_port);
		sReplyPorts[cached_reply_port] = create_port(1, "tmp_rport");
	}

error:
	if (cached_reply_port >= 0) {
		// Reclaim ownership of cached port
		set_port_owner(reply_port, team);
		// Flag as available
		atomic_add(&sReplyPortInUse[cached_reply_port], -1);
		return error;
	}

	delete_port(reply_port);
	return error;
}


// Note, that in case of a flattened BMessage setting the target token will
// change the header size, if no token was set when the message was flattened.
// Hence the message would need to unflattened and flattened again before it
// can be sent.
status_t
BMessage::_SendFlattenedMessage(void *data, int32 size, port_id port,
	int32 token, bool preferred, bigtime_t timeout)
{
	if (!data)
		return B_BAD_VALUE;

	uint32 magic = *(uint32 *)data;

	// prepare flattened fields
	if (((KMessage::Header*)data)->magic == KMessage::kMessageHeaderMagic) {
		// a KMessage
		KMessage::Header *header = (KMessage::Header*)data;
		header->targetToken = (preferred ? B_PREFERRED_TOKEN : token);
	} else if (magic == kMessageMagic || magic == kMessageMagicSwapped) {
		// get the header
		BMemoryIO stream(data, size);

		stream.Read(&magic, sizeof(uint32));
			// "discard" the magic (we already know it anyway)

		Header header;
		status_t error = header.SetMagic(magic);
		if (error < B_OK)
			return error;

		error = header.ReadFrom(stream);
		if (error < B_OK)
			return error;

		if (!header.HasTarget()) {
			// fallback implementation -- the header size would change by
			// setting the token

			// unflatten the message
			BMessage message;
			error = message.Unflatten((const char*)data);
			if (error != B_OK)
				return error;

			// send the message
			BMessenger messenger;
			return message._send_(port, token, preferred, timeout, false,
				messenger);
		}

		// set the target token and replace the header
		header.SetTarget(token, preferred);
		stream.Seek(0LL, SEEK_SET);
		error = header.WriteTo(stream);
		if (error != B_OK)
			return error;
	} else
		return B_NOT_A_MESSAGE;

	// send the message
	status_t error;
	do {
		error = write_port_etc(port, 'pjpp', data, size, B_RELATIVE_TIMEOUT,
			timeout);
	} while (error == B_INTERRUPTED);

	return error;
}


void
BMessage::_StaticCacheCleanup()
{
	delete sMsgCache;
	sMsgCache = NULL;
}


void
BMessage::_StaticInit()
{
	sReplyPorts[0] = create_port(1, "tmp_rport0");
	sReplyPorts[1] = create_port(1, "tmp_rport1");
	sReplyPorts[2] = create_port(1, "tmp_rport2");

	sReplyPortInUse[0] = 0;
	sReplyPortInUse[1] = 0;
	sReplyPortInUse[2] = 0;
}


void
BMessage::_StaticCleanup()
{
	delete_port(sReplyPorts[0]);
	sReplyPorts[0] = -1;
	delete_port(sReplyPorts[1]);
	sReplyPorts[1] = -1;
	delete_port(sReplyPorts[2]);
	sReplyPorts[2] = -1;
}


int32
BMessage::sGetCachedReplyPort()
{
	int index = -1;
	for (int32 i = 0; i < sNumReplyPorts; i++) {
		int32 old = atomic_add(&(sReplyPortInUse[i]), 1);
		if (old == 0) {
			// This entry is free
			index = i;
			break;
		} else {
			// This entry is being used.
			atomic_add(&(sReplyPortInUse[i]), -1);
		}
	}

	return index;
}


static status_t
handle_reply(port_id reply_port, int32 *pCode, bigtime_t timeout,
	BMessage *reply)
{
	PRINT(("handle_reply(port: %ld)\n", reply_port));

	status_t error;
	do {
		error = port_buffer_size_etc(reply_port, 8, timeout);
	} while (error == B_INTERRUPTED);

	if (error < B_OK) {
		PRINT(("handle_reply() error 1: %lx\n", err));
		return error;
	}

	// The API lied. It really isn't an error code, but the message size...
	char *pAllocd = NULL;
	char *pMem = NULL;
	char tmp[0x800];

	if (error < 0x800)
		pMem = tmp;
	else {
		pAllocd = new char[error];
		pMem = pAllocd;
	}

	do {
		error = read_port(reply_port, pCode, pMem, error);
	} while (error == B_INTERRUPTED);

	if (error < 0) {
		PRINT(("handle_reply() error 2: %lx\n", error));
		return error;
	}

	if (*pCode == 'PUSH') {
		PRINT(("handle_reply() error 3: %x\n", B_ERROR));
		return B_ERROR;
	}

	if (*pCode != 'pjpp') {
		PRINT(("handle_reply() error 4: port message code not 'pjpp' but "
			"'%lx'\n", *pCode));
		return B_ERROR;
	}

	error = reply->Unflatten(pMem);

	// There seems to be a bug in the original Be implementation.
	// It never free'd pAllocd !
	if (pAllocd)
		delete[] pAllocd;

	PRINT(("handle_reply() done: %lx\n", error));
	return error;
}


// convert_message
static status_t
convert_message(const KMessage *fromMessage, BMessage *toMessage)
{
	if (!fromMessage || !toMessage)
		return B_BAD_VALUE;

	// make empty and init what of the target message
	toMessage->MakeEmpty();
	toMessage->what = fromMessage->What();

	// iterate through the fields and import them in the target message
	KMessageField field;
	while (fromMessage->GetNextField(&field) == B_OK) {
		int32 elementCount = field.CountElements();
		if (elementCount > 0) {
			for (int32 i = 0; i < elementCount; i++) {
				int32 size;
				const void *data = field.ElementAt(i, &size);
				status_t error;
				if (field.TypeCode() == B_MESSAGE_TYPE) {
					// message type: if it's a KMessage, convert it
					KMessage message;
					if (message.SetTo(data, size) == B_OK) {
						BMessage bMessage;
						error = convert_message(&message, &bMessage);
						if (error != B_OK)
							return error;
						error = toMessage->AddMessage(field.Name(), &bMessage);
					} else {
						// just add it
						error = toMessage->AddData(field.Name(),
							field.TypeCode(), data, size,
							field.HasFixedElementSize(), 1);
					}
				} else {
					error = toMessage->AddData(field.Name(), field.TypeCode(),
						data, size, field.HasFixedElementSize(), 1);
				}

				if (error != B_OK)
					return error;
			}
		}
	}
	return B_OK;
}


static ssize_t
min_hdr_size()
{
	ssize_t size = 0;

	size += 4;	// version
	size += 4;	// checksum
	size += 4;	// flattened size
	size += 4;	// 'what'
	size += 1;	// flags

	return size;
}
