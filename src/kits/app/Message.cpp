//------------------------------------------------------------------------------
//	Copyright (c) 2001-2002, OpenBeOS
//
//	Permission is hereby granted, free of charge, to any person obtaining a
//	copy of this software and associated documentation files (the "Software"),
//	to deal in the Software without restriction, including without limitation
//	the rights to use, copy, modify, merge, publish, distribute, sublicense,
//	and/or sell copies of the Software, and to permit persons to whom the
//	Software is furnished to do so, subject to the following conditions:
//
//	The above copyright notice and this permission notice shall be included in
//	all copies or substantial portions of the Software.
//
//	THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//	IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//	FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//	AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//	LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
//	FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
//	DEALINGS IN THE SOFTWARE.
//
//	File Name:		Message.h
//	Author(s):		Erik Jaesler (erik@cgsoftware.com)
//					DarkWyrm <bpmagic@columbus.rr.com>
//	Description:	BMessage class creates objects that store data and that
//					can be processed in a message loop.  BMessage objects
//					are also used as data containers by the archiving and 
//					the scripting mechanisms.
//------------------------------------------------------------------------------

#define USING_TEMPLATE_MADNESS

// Standard Includes -----------------------------------------------------------
#include <stdio.h>

// System Includes -------------------------------------------------------------
#include <Application.h>
#include <BlockCache.h>
#include <ByteOrder.h>
#include <Errors.h>
#include <Message.h>
#include <Messenger.h>
#include <String.h>

//#include <CRTDBG.H>

// Project Includes ------------------------------------------------------------
#ifdef USING_TEMPLATE_MADNESS
#include <AppMisc.h>
#include <DataBuffer.h>
#include <MessageBody.h>
#include <MessageUtils.h>
#include <TokenSpace.h>
#endif	// USING_TEMPLATE_MADNESS

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------
#define	MSG_FIELD_VERSION		'FOB1'

// flags for the overall message (the bitfield is 1 byte)
#define MSG_FLAG_BIG_ENDIAN		0x01
#define MSG_FLAG_INCL_TARGET	0x02
#define MSG_FLAG_INCL_REPLY		0x04
#define MSG_FLAG_SCRIPT_MSG		0x08
// These are for future improvement
#if 0
#define MSG_FLAG_USE_PREFERRED	0x10
#define MSG_FLAG_REPLY_WANTED	0x20
#define MSG_FLAG_REPLY_DONE		0x40
#define MSG_FLAG_IS_REPLY		0x80

#define MSG_FLAG_HDR_MASK		0xF0
#endif

#define MSG_HEADER_MAX_SIZE		38
#define MSG_NAME_MAX_SIZE		256

// Globals ---------------------------------------------------------------------

#ifdef USING_TEMPLATE_MADNESS
using namespace BPrivate;
#endif	// USING_TEMPLATE_MADNESS
const char* B_SPECIFIER_ENTRY = "specifiers";
const char* B_PROPERTY_ENTRY = "property";
const char* B_PROPERTY_NAME_ENTRY = "name";

BBlockCache* BMessage::sMsgCache = NULL;
port_id BMessage::sReplyPorts[sNumReplyPorts];
long BMessage::sReplyPortInUse[sNumReplyPorts];


static status_t handle_reply(port_id   reply_port,
                             int32*    pCode,
                             bigtime_t timeout,
                             BMessage* reply);

//------------------------------------------------------------------------------
extern "C" {
void _msg_cache_cleanup_()
{
	delete BMessage::sMsgCache;
	BMessage::sMsgCache = NULL;
}
//------------------------------------------------------------------------------
int _init_message_()
{
	BMessage::sReplyPorts[0] = create_port(1, "tmp_rport0");
	BMessage::sReplyPorts[1] = create_port(1, "tmp_rport1");
	BMessage::sReplyPorts[2] = create_port(1, "tmp_rport2");

	BMessage::sReplyPortInUse[0] = 0;
	BMessage::sReplyPortInUse[1] = 0;
	BMessage::sReplyPortInUse[2] = 0;
	return 0;
}
//------------------------------------------------------------------------------
int _delete_message_()
{
	delete_port(BMessage::sReplyPorts[0]);
	BMessage::sReplyPorts[0] = -1;
	delete_port(BMessage::sReplyPorts[1]);
	BMessage::sReplyPorts[1] = -1;
	delete_port(BMessage::sReplyPorts[2]);
	BMessage::sReplyPorts[2] = -1;
	return 0;
}
}	// extern "C"
//------------------------------------------------------------------------------
BMessage *_reconstruct_msg_(uint32,uint32,uint32)
{
	return NULL;
}
//------------------------------------------------------------------------------

#ifdef USING_TEMPLATE_MADNESS
void BMessage::_ReservedMessage1() {}
void BMessage::_ReservedMessage2() {}
void BMessage::_ReservedMessage3() {}

//------------------------------------------------------------------------------
BMessage::BMessage()
	:	what(0), fBody(NULL)
{
	init_data();
}
//------------------------------------------------------------------------------
BMessage::BMessage(uint32 w)
	:	fBody(NULL)
{
	init_data();
	what = w;
}
//------------------------------------------------------------------------------
BMessage::BMessage(const BMessage& a_message)
	:	fBody(NULL)
{
	init_data();
	*this = a_message;
}
//------------------------------------------------------------------------------
BMessage::BMessage(BMessage *a_message)
	:	fBody(NULL)
{
	init_data();
	*this = *a_message;
}
//------------------------------------------------------------------------------
BMessage::~BMessage()
{
	if (IsSourceWaiting())
	{
		SendReply(B_NO_REPLY);
	}
	delete fBody;
}
//------------------------------------------------------------------------------
BMessage& BMessage::operator=(const BMessage& msg)
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
//------------------------------------------------------------------------------
void BMessage::init_data()
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
	{
		fBody->MakeEmpty();
	}
	else
	{
		fBody = new BPrivate::BMessageBody;
	}
}
//------------------------------------------------------------------------------
status_t BMessage::GetInfo(type_code typeRequested, int32 which, char** name,
						   type_code* typeReturned, int32* count) const
{
	return fBody->GetInfo(typeRequested, which, name, typeReturned, count);
}
//------------------------------------------------------------------------------
status_t BMessage::GetInfo(const char* name, type_code* type, int32* c) const
{
	return fBody->GetInfo(name, type, c);
}
//------------------------------------------------------------------------------
status_t BMessage::GetInfo(const char* name, type_code* type,
						   bool* fixed_size) const
{
	return fBody->GetInfo(name, type, fixed_size);
}
//------------------------------------------------------------------------------
int32 BMessage::CountNames(type_code type) const
{
	return fBody->CountNames(type);
}
//------------------------------------------------------------------------------
bool BMessage::IsEmpty() const
{
	return fBody->IsEmpty();
}
//------------------------------------------------------------------------------
bool BMessage::IsSystem() const
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
	{
		return true;
	}

	return false;
}
//------------------------------------------------------------------------------
bool BMessage::IsReply() const
{
	return fIsReply;
}
//------------------------------------------------------------------------------
void BMessage::PrintToStream() const
{
	printf("\nBMessage: what =  (0x%lX or %ld)\n", what, what);
	fBody->PrintToStream();
}
//------------------------------------------------------------------------------
status_t BMessage::Rename(const char* old_entry, const char* new_entry)
{
	return fBody->Rename(old_entry, new_entry);
}
//------------------------------------------------------------------------------
bool BMessage::WasDelivered() const
{
	return fWasDelivered;
}
//------------------------------------------------------------------------------
bool BMessage::IsSourceWaiting() const
{
	return fReplyRequired && !fReplyDone;
}
//------------------------------------------------------------------------------
bool BMessage::IsSourceRemote() const
{
	return WasDelivered() && fReplyTo.team != BPrivate::current_team();
}
//------------------------------------------------------------------------------
BMessenger BMessage::ReturnAddress() const
{
	if (WasDelivered())
	{
		return BMessenger(fReplyTo.team, fReplyTo.port, fReplyTo.target,
						  fReplyTo.preferred);
	}

	return BMessenger();
}
//------------------------------------------------------------------------------
const BMessage* BMessage::Previous() const
{
	// TODO: test
	// In particular, look to see if the "_previous_" field is used in R5
	if (!fOriginal)
	{
		BMessage* fOriginal = new BMessage;
		if (FindMessage("_previous_", fOriginal) != B_OK)
		{
			delete fOriginal;
			fOriginal = NULL;
		}
	}

	return fOriginal;
}
//------------------------------------------------------------------------------
bool BMessage::WasDropped() const
{
	return fReadOnly;
}
//------------------------------------------------------------------------------
BPoint BMessage::DropPoint(BPoint* offset) const
{
	// TODO: Where do we get this stuff???
	if (offset)
	{
		*offset = FindPoint("_drop_offset_");
	}
	return FindPoint("_drop_point_");
}
//------------------------------------------------------------------------------
status_t BMessage::SendReply(uint32 command, BHandler* reply_to)
{
	BMessage msg(command);
	return SendReply(&msg, reply_to);
}
//------------------------------------------------------------------------------
status_t BMessage::SendReply(BMessage* the_reply, BHandler* reply_to,
							 bigtime_t timeout)
{
	BMessenger messenger(reply_to);
	return SendReply(the_reply, messenger, timeout);
}
//------------------------------------------------------------------------------
#if 0
template<class Sender>
status_t SendReplyHelper(BMessage* the_message, BMessage* the_reply,
						 Sender& the_sender)
{
	BMessenger messenger(the_message->fReplyTo.team, the_message->fReplyTo.port,
						 the_message->fReplyTo.target,
						 the_message->fReplyTo.preferred);
	if (the_message->fReplyRequired)
	{
		if (the_message->fReplyDone)
		{
			return B_DUPLICATE_REPLY;
		}
		the_message->fReplyDone = true;
		the_reply->fIsReply = true;
		status_t err = the_sender.Send(messenger, the_reply);
		the_reply->fIsReply = false;
		if (err)
		{
			if (set_port_owner(messenger.fPort, messenger.fTeam) == B_BAD_TEAM_ID)
			{
				delete_port(messenger.fPort);
			}
		}
		return err;
	}
	// no reply required
	if (!the_message->fWasDelivered)
	{
		return B_BAD_REPLY;
	}

#if 0
	char tmp[0x800];
	ssize_t size;
	char* p = stack_flatten(tmp, sizeof(tmp), true /* include reply */, &size);
	the_reply->AddData("_previous_", B_RAW_TYPE, p ? p : tmp, &size);
	if (p)
	{
		free(p);
	}
#endif
	the_reply->AddMessage("_previous_", the_message);
	the_reply->fIsReply = true;
	status_t err = the_sender.Send(messenger, the_reply);
	the_reply->fIsReply = false;
	the_reply->RemoveName("_previous_");
	return err;
};
#endif
//------------------------------------------------------------------------------
#if 0
struct Sender1
{
	BMessenger& reply_to;
	bigtime_t timeout;

	Sender1(BMessenger& m, bigtime_t t) : reply_to(m), timeout(t) {;}

	status_t Send(BMessenger& messenger, BMessage* the_reply)
	{
		return messenger.SendMessage(the_reply, reply_to, timeout);
	}
};
status_t BMessage::SendReply(BMessage* the_reply, BMessenger reply_to,
							 bigtime_t timeout)
{
	Sender1 mySender(reply_to, timeout);
	return SendReplyHelper(this, the_reply, mySender);
}
#endif
status_t BMessage::SendReply(BMessage* the_reply, BMessenger reply_to,
							 bigtime_t timeout)
{
	// TODO: test
	BMessenger messenger(fReplyTo.team, fReplyTo.port,
						 fReplyTo.target,
						 fReplyTo.preferred);
	if (fReplyRequired)
	{
		if (fReplyDone)
		{
			return B_DUPLICATE_REPLY;
		}
		fReplyDone = true;
		the_reply->fIsReply = true;
		status_t err = messenger.SendMessage(the_reply, reply_to, timeout);
		the_reply->fIsReply = false;
		if (err)
		{
			if (set_port_owner(messenger.fPort, messenger.fTeam) == B_BAD_TEAM_ID)
			{
				delete_port(messenger.fPort);
			}
		}
		return err;
	}
	// no reply required
	if (!fWasDelivered)
	{
		return B_BAD_REPLY;
	}

	the_reply->AddMessage("_previous_", this);
	the_reply->fIsReply = true;
	status_t err = messenger.SendMessage(the_reply, reply_to, timeout);
	the_reply->fIsReply = false;
	the_reply->RemoveName("_previous_");
	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::SendReply(uint32 command, BMessage* reply_to_reply)
{
	BMessage msg(command);
	return SendReply(&msg, reply_to_reply);
}
//------------------------------------------------------------------------------
#if 0
struct Sender2
{
	BMessage* reply_to_reply;
	bigtime_t send_timeout;
	bigtime_t reply_timeout;
	
	Sender2(BMessage* m, bigtime_t t1, bigtime_t t2)
		:	reply_to_reply(m), send_timeout(t1), reply_timeout(t2) {;}
	
	status_t Send(BMessenger& messenger, BMessage* the_reply)
	{
		return messenger.SendMessage(the_reply, reply_to_reply,
									 send_timeout, reply_timeout);
	}
};
status_t BMessage::SendReply(BMessage* the_reply, BMessage* reply_to_reply,
							 bigtime_t send_timeout, bigtime_t reply_timeout)
{
	Sender2 mySender(reply_to_reply, send_timeout, reply_timeout);	
	return SendReplyHelper(this, the_reply, mySender);
}
#endif
status_t BMessage::SendReply(BMessage* the_reply, BMessage* reply_to_reply,
							 bigtime_t send_timeout, bigtime_t reply_timeout)
{
	// TODO: test
	BMessenger messenger(fReplyTo.team, fReplyTo.port,
						 fReplyTo.target,
						 fReplyTo.preferred);
	if (fReplyRequired)
	{
		if (fReplyDone)
		{
			return B_DUPLICATE_REPLY;
		}
		fReplyDone = true;
		the_reply->fIsReply = true;
		status_t err = messenger.SendMessage(the_reply, reply_to_reply,
											 send_timeout, reply_timeout);
		the_reply->fIsReply = false;
		if (err)
		{
			if (set_port_owner(messenger.fPort, messenger.fTeam) == B_BAD_TEAM_ID)
			{
				delete_port(messenger.fPort);
			}
		}
		return err;
	}
	// no reply required
	if (!fWasDelivered)
	{
		return B_BAD_REPLY;
	}

	the_reply->AddMessage("_previous_", this);
	the_reply->fIsReply = true;
	status_t err = messenger.SendMessage(the_reply, reply_to_reply,
										 send_timeout, reply_timeout);
	the_reply->fIsReply = false;
	the_reply->RemoveName("_previous_");
	return err;
}
//------------------------------------------------------------------------------
ssize_t BMessage::FlattenedSize() const
{
	return calc_hdr_size(0) + fBody->FlattenedSize();
}
//------------------------------------------------------------------------------
status_t BMessage::Flatten(char* buffer, ssize_t size) const
{
	return real_flatten(buffer, size);
}
//------------------------------------------------------------------------------
status_t BMessage::Flatten(BDataIO* stream, ssize_t* size) const
{
	status_t err = B_OK;
	ssize_t len = FlattenedSize();
	char* buffer = new(nothrow) char[len];
	if (buffer)
	{
		err = Flatten(buffer, len);
		if (!err)
		{
			// size is an optional parameter, don't crash on NULL
			if (size != NULL)
			{
				*size = len;
			}
			err = stream->Write(buffer, len);
			if (err > B_OK)
				err = B_OK;
		}

		delete[] buffer;
	}
	else
	{
		err = B_NO_MEMORY;
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::Unflatten(const char* flat_buffer)
{
	uint32 size = ((uint32*)flat_buffer)[2];
	
	BMemoryIO MemIO(flat_buffer, size);
	return Unflatten(&MemIO);
}
//------------------------------------------------------------------------------
status_t BMessage::Unflatten(BDataIO* stream)
{
	bool swap;
	status_t err = unflatten_hdr(stream, swap);
	
	if (!err)
	{
		TReadHelper reader(stream, swap);
		int8 flags;
		type_code type;
		uint32 count;
		uint32 dataLen;
		uint8 nameLen;
		char name[MSG_NAME_MAX_SIZE];
		unsigned char* databuffer = NULL;

		try
		{
			reader(flags);
			while (flags != MSG_LAST_ENTRY)
			{
				reader(type);
				// Is there more than one data item? (!flags & MSG_FLAG_SINGLE_ITEM)
				if (flags & MSG_FLAG_SINGLE_ITEM)
				{
					count = 1;
					if (flags & MSG_FLAG_MINI_DATA)
					{
						uint8 littleLen;
						reader(littleLen);
						dataLen = littleLen;
					}
					else
					{
						reader(dataLen);
					}
				}
				else
				{
					// Is there a little data? (flags & MSG_FLAG_MINI_DATA)
					if (flags & MSG_FLAG_MINI_DATA)
					{
						// Get item count (1 byte)
						uint8 littleCount;
						reader(littleCount);
						count = littleCount;
						
						// Get data length (1 byte)
						uint8 littleLen;
						reader(littleLen);
						dataLen = littleLen;
					}
					else
					{
					// Is there a lot of data? (!flags & MSG_FLAG_MINI_DATA)
						// Get item count (4 bytes)
						reader(count);
						// Get data length (4 bytes)
						reader(dataLen);
					}
				}

				// Get the name length (1 byte)
				reader(nameLen);
				// Get the name (name length bytes)
				reader(name, nameLen);
				name[nameLen] = '\0';

				// Copy the data into a new buffer to byte align it
				databuffer = (unsigned char*)realloc(databuffer, dataLen);
				if (!databuffer)
					throw B_NO_MEMORY;
				// Get the data
				reader(databuffer, dataLen);

				// Is the data fixed size? (flags & MSG_FLAG_FIXED_SIZE)
				if (flags & MSG_FLAG_FIXED_SIZE && swap)
				{
					// Make sure to swap the data
					err = swap_data(type, (void*)databuffer, dataLen,
									B_SWAP_ALWAYS);
					if (err)
						throw err;
				}
				// Is the data variable size? (!flags & MSG_FLAG_FIXED_SIZE)
				else if (swap && type == B_REF_TYPE)
				{
					// Apparently, entry_refs are the only variable-length data
					// 	  explicitely swapped -- the dev_t and ino_t
					//    specifically
					byte_swap(*(entry_ref*)databuffer);
				}

				// Add each data field to the message
				uint32 itemSize=0;
				if (flags & MSG_FLAG_FIXED_SIZE)
				{
					itemSize = dataLen / count;
				}
				unsigned char* dataPtr = databuffer;
				for (uint32 i = 0; i < count; ++i)
				{
					// Line up for the next item
					if (i)
					{
						if (flags & MSG_FLAG_FIXED_SIZE)
						{
							dataPtr += itemSize;
						}
						else
						{
// ToDo: gcc is right, itemSize is used uninitialized here!!
//		Since I don't know how to fix it properly, I leave this warning in
							// Have to account for 8-byte boundary padding
							dataPtr += itemSize + (8 - (itemSize % 8));
						}
					}

					if ((flags & MSG_FLAG_FIXED_SIZE) == 0)
					{
						itemSize = *(uint32*)dataPtr;
						dataPtr += sizeof (uint32);
					}

					err = AddData(name, type, dataPtr, itemSize,
								  flags & MSG_FLAG_FIXED_SIZE);
					if (err)
						throw err;
				}

				reader(flags);
			}
		}
		catch (status_t& e)
		{
			err = e;
		}
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::AddSpecifier(const char* property)
{
	BMessage message(B_DIRECT_SPECIFIER);
	status_t err = message.AddString(B_PROPERTY_ENTRY, property);
	if (err)
		return err;

	return AddSpecifier(&message);
}
//------------------------------------------------------------------------------
status_t BMessage::AddSpecifier(const char* property, int32 index)
{
	BMessage message(B_INDEX_SPECIFIER);
	status_t err = message.AddString(B_PROPERTY_ENTRY, property);
	if (err)
		return err;

	err = message.AddInt32("index", index);
	if (err)
		return err;

	return AddSpecifier(&message);
}
//------------------------------------------------------------------------------
status_t BMessage::AddSpecifier(const char* property, int32 index, int32 range)
{
	if (range < 0)
		return B_BAD_VALUE;

	BMessage message(B_RANGE_SPECIFIER);	
	status_t err = message.AddString(B_PROPERTY_ENTRY, property);
	if (err)
		return err;

	err = message.AddInt32("index", index);
	if (err)
		return err;

	err = message.AddInt32("range", range);
	if (err)
		return err;

	return AddSpecifier(&message);
}
//------------------------------------------------------------------------------
status_t BMessage::AddSpecifier(const char* property, const char* name)
{
	BMessage message(B_NAME_SPECIFIER);
	status_t err = message.AddString(B_PROPERTY_ENTRY, property);
	if (err)
		return err;

	err = message.AddString(B_PROPERTY_NAME_ENTRY, name);
	if (err)
		return err;

	return AddSpecifier(&message);
}
//------------------------------------------------------------------------------
status_t BMessage::AddSpecifier(const BMessage* specifier)
{
	status_t err = AddMessage(B_SPECIFIER_ENTRY, specifier);
	if (!err)
	{
		++fCurSpecifier;
		fHasSpecifiers = true;
	}
	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::SetCurrentSpecifier(int32 index)
{
	type_code	type;
	int32		count;
	status_t	err = GetInfo(B_SPECIFIER_ENTRY, &type, &count);
	if (err)
		return err;

	if (index < 0 || index >= count)
		return B_BAD_INDEX;

	fCurSpecifier = index;
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::GetCurrentSpecifier(int32* index, BMessage* specifier,
									   int32* what, const char** property) const
{
	if (fCurSpecifier == -1 || !WasDelivered())
		return B_BAD_SCRIPT_SYNTAX;

	if (index)
		*index = fCurSpecifier;

	if (specifier)
	{
		if (FindMessage(B_SPECIFIER_ENTRY, fCurSpecifier, specifier))
			return B_BAD_SCRIPT_SYNTAX;

		if (what)
			*what = specifier->what;

		if (property)
		{
			if (specifier->FindString(B_PROPERTY_ENTRY, property))
				return B_BAD_SCRIPT_SYNTAX;
		}
	}

	return B_OK;
}
//------------------------------------------------------------------------------
bool BMessage::HasSpecifiers() const
{
	return fHasSpecifiers;
}
//------------------------------------------------------------------------------
status_t BMessage::PopSpecifier()
{
	if (fCurSpecifier < 0 || !WasDelivered())
	{
		return B_BAD_VALUE;
	}

	--fCurSpecifier;
	return B_OK;
}
//------------------------------------------------------------------------------
//	return fBody->AddData<TYPE>(name, val, TYPESPEC);
//	return fBody->FindData<TYPE>(name, index, val, TYPESPEC);
//	return fBody->ReplaceData<TYPE>(name, index, val, TYPESPEC);
//	return fBody->HasData(name, TYPESPEC, n);

#define DEFINE_FUNCTIONS(TYPE, fnName, TYPESPEC)									\
	status_t BMessage::Add ## fnName(const char* name, TYPE val)					\
	{ return fBody->AddData<TYPE>(name, val, TYPESPEC); }							\
	status_t BMessage::Find ## fnName(const char* name, TYPE* p) const				\
	{ return Find ## fnName(name, 0, p); }											\
	status_t BMessage::Find ## fnName(const char* name, int32 index, TYPE* p) const	\
	{																				\
		*p = TYPE();																\
		return fBody->FindData<TYPE>(name, index, p, TYPESPEC);						\
	}																				\
	status_t BMessage::Replace ## fnName(const char* name, TYPE val)				\
	{ return Replace ## fnName(name, 0, val); }										\
	status_t BMessage::Replace ## fnName(const char *name, int32 index, TYPE val)	\
	{  return fBody->ReplaceData<TYPE>(name, index, val, TYPESPEC); }				\
	bool BMessage::Has ## fnName(const char* name, int32 n) const					\
	{ return fBody->HasData(name, TYPESPEC, n); }

DEFINE_FUNCTIONS(int8  , Int8  , B_INT8_TYPE)
DEFINE_FUNCTIONS(int16 , Int16 , B_INT16_TYPE)
DEFINE_FUNCTIONS(int32 , Int32 , B_INT32_TYPE)
DEFINE_FUNCTIONS(int64 , Int64 , B_INT64_TYPE)
DEFINE_FUNCTIONS(BPoint, Point , B_POINT_TYPE)
DEFINE_FUNCTIONS(BRect , Rect  , B_RECT_TYPE)
DEFINE_FUNCTIONS(float , Float , B_FLOAT_TYPE)
DEFINE_FUNCTIONS(double, Double, B_DOUBLE_TYPE)
DEFINE_FUNCTIONS(bool  , Bool  , B_BOOL_TYPE)

#undef DEFINE_FUNCTIONS


#define DEFINE_HAS_FUNCTION(fnName, TYPESPEC)						\
	bool BMessage::Has ## fnName(const char* name, int32 n) const	\
	{ return HasData(name, TYPESPEC, n); }

DEFINE_HAS_FUNCTION(Message  , B_MESSAGE_TYPE)
DEFINE_HAS_FUNCTION(String   , B_STRING_TYPE)
DEFINE_HAS_FUNCTION(Pointer  , B_POINTER_TYPE)
DEFINE_HAS_FUNCTION(Messenger, B_MESSENGER_TYPE)
DEFINE_HAS_FUNCTION(Ref      , B_REF_TYPE)

#undef DEFINE_HAS_FUNCTION

#define DEFINE_LAZY_FIND_FUNCTION(TYPE, fnName)						\
	TYPE BMessage::Find ## fnName(const char* name, int32 n) const	\
	{																\
		TYPE i = 0;													\
		Find ## fnName(name, n, &i);								\
		return i;													\
	}

DEFINE_LAZY_FIND_FUNCTION(int8			, Int8)
DEFINE_LAZY_FIND_FUNCTION(int16			, Int16)
DEFINE_LAZY_FIND_FUNCTION(int32			, Int32)
DEFINE_LAZY_FIND_FUNCTION(int64			, Int64)
DEFINE_LAZY_FIND_FUNCTION(float			, Float)
DEFINE_LAZY_FIND_FUNCTION(double		, Double)
DEFINE_LAZY_FIND_FUNCTION(bool			, Bool)
DEFINE_LAZY_FIND_FUNCTION(const char*	, String)

#undef DEFINE_LAZY_FIND_FUNCTION

//------------------------------------------------------------------------------
status_t BMessage::AddString(const char* name, const char* a_string)
{
	return fBody->AddData<BString>(name, a_string, B_STRING_TYPE);
}
//------------------------------------------------------------------------------
status_t BMessage::AddString(const char* name, const BString& a_string)
{
	return AddString(name, a_string.String());
}
//------------------------------------------------------------------------------
status_t BMessage::AddPointer(const char* name, const void* ptr)
{
	return fBody->AddData<void*>(name, (void*)ptr, B_POINTER_TYPE);
}
//------------------------------------------------------------------------------
status_t BMessage::AddMessenger(const char* name, BMessenger messenger)
{
	return fBody->AddData<BMessenger>(name, messenger, B_MESSENGER_TYPE);
}
//------------------------------------------------------------------------------
status_t BMessage::AddRef(const char* name, const entry_ref* ref)
{
	char* buffer = new(nothrow) char[sizeof (entry_ref) + B_PATH_NAME_LENGTH];
	size_t size;
	status_t err = entry_ref_flatten(buffer, &size, ref);
	if (!err)
	{
		BDataBuffer DB((void*)buffer, size);
		err = fBody->AddData<BDataBuffer>(name, DB, B_REF_TYPE);
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::AddMessage(const char* name, const BMessage* msg)
{
	status_t err = B_OK;
	ssize_t size = msg->FlattenedSize();
	char* buffer = new(nothrow) char[size];
	if (buffer)
	{
		err = msg->Flatten(buffer, size);
		if (!err)
		{
			BDataBuffer DB((void*)buffer, size);
			err = fBody->AddData<BDataBuffer>(name, DB, B_MESSAGE_TYPE);
		}
	}
	else
	{
		err = B_NO_MEMORY;
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::AddFlat(const char* name, BFlattenable* obj, int32 count)
{
	status_t err = B_OK;
	ssize_t size = obj->FlattenedSize();
	char* buffer = new(nothrow) char[size];
	if (buffer)
	{
		err = obj->Flatten((void*)buffer, size);
		if (!err)
		{
			err = AddData(name, obj->TypeCode(), (void*)buffer, size,
						  obj->IsFixedSize(), count);
		}
		delete[] buffer;
	}
	else
	{
		err = B_NO_MEMORY;
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::AddData(const char* name, type_code type, const void* data,
						   ssize_t numBytes, bool is_fixed_size, int32 /*count*/)
{
/**
	@note	Because we're using vectors for our item storage, the count param 
			is no longer useful to us:  dynamically adding more items is not 
			really a performance issue, so pre-allocating space for objects 
			gives us no real advantage.
 */

	// TODO: test
	// In particular, we want to see what happens if is_fixed_size == true and
	// the user attempts to add something bigger or smaller.  We may need to
	// enforce the size thing.
	//--------------------------------------------------------------------------
	// voidref suggests creating a BDataBuffer type which we can use here to
	// avoid having to specialize BMessageBody::AddData().
	//--------------------------------------------------------------------------

	// TODO: Fix this horrible hack
	status_t err = B_OK;
	switch (type)
	{
		case B_BOOL_TYPE:
			err = AddBool(name, *(bool*)data);
			break;
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			err = AddInt8(name, *(int8*)data);
			break;
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			err = AddInt16(name, *(int16*)data);
			break;
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
			err = AddInt32(name, *(int32*)data);
			break;
		case B_INT64_TYPE:
		case B_UINT64_TYPE:
			err = AddInt64(name, *(int64*)data);
			break;
		case B_FLOAT_TYPE:
			err = AddFloat(name, *(float*)data);
			break;
		case B_DOUBLE_TYPE:
			err = AddDouble(name, *(double*)data);
			break;
		case B_POINT_TYPE:
			err = AddPoint(name, *(BPoint*)data);
			break;
		case B_RECT_TYPE:
			err = AddRect(name, *(BRect*)data);
			break;
		case B_REF_TYPE:
		{
			BDataBuffer DB((void*)data, numBytes, true);
			err = fBody->AddData<BDataBuffer>(name, DB, type);
			break;
		}
		case B_MESSAGE_TYPE:
		{
			BDataBuffer DB((void*)data, numBytes, true);
			err = fBody->AddData<BDataBuffer>(name, DB, type);
			break;
		}
		case B_MESSENGER_TYPE:
			err = AddMessenger(name, *(BMessenger*)data);
			break;
		case B_POINTER_TYPE:
			err = AddPointer(name, *(void**)data);
			break;
		case B_STRING_TYPE:
			err = AddString(name, (const char*)data);
			break;
		default:
			// TODO: test
			// Using the mythical BDataBuffer
			BDataBuffer DB((void*)data, numBytes, true);
			err = fBody->AddData<BDataBuffer>(name, DB, type);
			break;
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::RemoveData(const char* name, int32 index)
{
	return fReadOnly ? B_ERROR : fBody->RemoveData(name, index);
}
//------------------------------------------------------------------------------
status_t BMessage::RemoveName(const char* name)
{
	return fReadOnly ? B_ERROR : fBody->RemoveName(name);
}
//------------------------------------------------------------------------------
status_t BMessage::MakeEmpty()
{
	return fReadOnly ? B_ERROR : fBody->MakeEmpty();
}
//------------------------------------------------------------------------------
status_t BMessage::FindString(const char* name, const char** str) const
{
	return FindString(name, 0, str);
}
//------------------------------------------------------------------------------
status_t BMessage::FindString(const char* name, int32 index,
							  const char** str) const
{
	ssize_t bytes;
	return FindData(name, B_STRING_TYPE, index,
					(const void**)str, &bytes);
}
//------------------------------------------------------------------------------
status_t BMessage::FindString(const char* name, BString* str) const
{
	return FindString(name, 0, str);
}
//------------------------------------------------------------------------------
status_t BMessage::FindString(const char* name, int32 index, BString* str) const
{
	const char* cstr;
	status_t err = FindString(name, index, &cstr);
	if (!err)
	{
		*str = cstr;
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::FindPointer(const char* name, void** ptr) const
{
	return FindPointer(name, 0, ptr);
}
//------------------------------------------------------------------------------
status_t BMessage::FindPointer(const char* name, int32 index, void** ptr) const
{
	*ptr = NULL;
	return fBody->FindData<void*>(name, index, ptr, B_POINTER_TYPE);
}
//------------------------------------------------------------------------------
status_t BMessage::FindMessenger(const char* name, BMessenger* m) const
{
	return FindMessenger(name, 0, m);
}
//------------------------------------------------------------------------------
status_t BMessage::FindMessenger(const char* name, int32 index, BMessenger* m) const
{
	return fBody->FindData<BMessenger>(name, index, m, B_MESSENGER_TYPE);
}
//------------------------------------------------------------------------------
status_t BMessage::FindRef(const char* name, entry_ref* ref) const
{
	return FindRef(name, 0, ref);
}
//------------------------------------------------------------------------------
status_t BMessage::FindRef(const char* name, int32 index, entry_ref* ref) const
{
	void* data = NULL;
	ssize_t size = 0;
	status_t err = FindData(name, B_REF_TYPE, index, (const void**)&data, &size);
	if (!err)
	{
		err = entry_ref_unflatten(ref, (char*)data, size);
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::FindMessage(const char* name, BMessage* msg) const
{
	return FindMessage(name, 0, msg);
}
//------------------------------------------------------------------------------
status_t BMessage::FindMessage(const char* name, int32 index, BMessage* msg) const
{
	void* data = NULL;
	ssize_t size = 0;
	status_t err = FindData(name, B_MESSAGE_TYPE, index,
							(const void**)&data, &size);
	if (!err)
	{
		err = msg->Unflatten((const char*)data);
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::FindFlat(const char* name, BFlattenable* obj) const
{
	return FindFlat(name, 0, obj);
}
//------------------------------------------------------------------------------
status_t BMessage::FindFlat(const char* name, int32 index,
							BFlattenable* obj) const
{
	const void* data;
	ssize_t numBytes;
	status_t err = FindData(name, obj->TypeCode(), index, &data, &numBytes);
	if (!err)
	{
		err = obj->Unflatten(obj->TypeCode(), data, numBytes);
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::FindData(const char* name, type_code type, const void** data,
							ssize_t* numBytes) const
{
	return FindData(name, type, 0, data, numBytes);
}
//------------------------------------------------------------------------------
status_t BMessage::FindData(const char* name, type_code type, int32 index,
							const void** data, ssize_t* numBytes) const
{
	status_t err = B_OK;
	// Oh, the humanity!
	err = fBody->FindData(name, type, index, data, numBytes);

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceString(const char* name, const char* string)
{
	return ReplaceString(name, 0, string);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceString(const char* name, int32 index,
								 const char* string)
{
	return fBody->ReplaceData<BString>(name, index, string, B_STRING_TYPE);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceString(const char* name, const BString& string)
{
	return ReplaceString(name, 0, string);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceString(const char* name, int32 index, const BString& string)
{
	return fBody->ReplaceData<BString>(name, index, string, B_STRING_TYPE);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplacePointer(const char* name, const void* ptr)
{
	return ReplacePointer(name, 0, ptr);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplacePointer(const char* name, int32 index,
								  const void* ptr)
{
	return fBody->ReplaceData<void*>(name, index, (void*)ptr, B_POINTER_TYPE);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceMessenger(const char* name, BMessenger messenger)
{
	// Don't want to copy the BMessenger
	return fBody->ReplaceData<BMessenger>(name, 0, messenger, B_MESSENGER_TYPE);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceMessenger(const char* name, int32 index,
									BMessenger msngr)
{
	return fBody->ReplaceData<BMessenger>(name, index, msngr, B_MESSENGER_TYPE);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceRef(const char* name, const entry_ref* ref)
{
	return ReplaceRef(name, 0, ref);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceRef(const char* name, int32 index, const entry_ref* ref)
{
	// TODO: test
	// Use voidref's theoretical BDataBuffer
	char* buffer = new(nothrow) char[sizeof (entry_ref) + B_PATH_NAME_LENGTH];
	size_t size;
	status_t err = entry_ref_flatten(buffer, &size, ref);
	if (!err)
	{
		BDataBuffer DB((void*)buffer, size);
		err = fBody->ReplaceData<BDataBuffer>(name, index, DB, B_REF_TYPE);
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceMessage(const char* name, const BMessage* msg)
{
	return ReplaceMessage(name, 0, msg);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceMessage(const char* name, int32 index,
								  const BMessage* msg)
{
	status_t err = B_OK;
	ssize_t size = msg->FlattenedSize();
	char* buffer = new(nothrow) char[size];
	if (buffer)
	{
		err = msg->Flatten(buffer, size);
		if (!err)
		{
			BDataBuffer DB((void*)buffer, size);
			err = fBody->ReplaceData<BDataBuffer>(name, index, DB,
												  B_MESSAGE_TYPE);
		}
	}
	else
	{
		err = B_NO_MEMORY;
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceFlat(const char* name, BFlattenable* obj)
{
	return ReplaceFlat(name, 0, obj);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceFlat(const char* name, int32 index, BFlattenable* obj)
{
	// TODO: test
	status_t err = B_OK;
	ssize_t size = obj->FlattenedSize();
	char* buffer = new(nothrow) char[size];
	if (buffer)
	{
		err = obj->Flatten(buffer, size);
		if (!err)
		{
			err = ReplaceData(name, obj->TypeCode(), index, (void*)buffer, size);
		}
		delete[] buffer;
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceData(const char* name, type_code type,
							   const void* data, ssize_t data_size)
{
	return ReplaceData(name, type, 0, data, data_size);
}
//------------------------------------------------------------------------------
status_t BMessage::ReplaceData(const char* name, type_code type, int32 index,
							   const void* data, ssize_t data_size)
{
	// TODO: Fix this horrible hack
	status_t err = B_OK;
	switch (type)
	{
		case B_BOOL_TYPE:
			err = ReplaceBool(name, index, *(bool*)data);
			break;
		case B_INT8_TYPE:
		case B_UINT8_TYPE:
			err = ReplaceInt8(name, index, *(int8*)data);
			break;
		case B_INT16_TYPE:
		case B_UINT16_TYPE:
			err = ReplaceInt16(name, index, *(int16*)data);
			break;
		case B_INT32_TYPE:
		case B_UINT32_TYPE:
			err = ReplaceInt32(name, index, *(int32*)data);
			break;
		case B_INT64_TYPE:
		case B_UINT64_TYPE:
			err = ReplaceInt64(name, index, *(int64*)data);
			break;
		case B_FLOAT_TYPE:
			err = ReplaceFloat(name, index, *(float*)data);
			break;
		case B_DOUBLE_TYPE:
			err = ReplaceDouble(name, index, *(double*)data);
			break;
		case B_POINT_TYPE:
			err = ReplacePoint(name, index, *(BPoint*)data);
			break;
		case B_RECT_TYPE:
			err = ReplaceRect(name, index, *(BRect*)data);
			break;
		case B_REF_TYPE:
			err = ReplaceRef(name, index, (entry_ref*)data);
			break;
		case B_MESSAGE_TYPE:
			err = ReplaceMessage(name, index, (BMessage*)data);
			break;
		case B_MESSENGER_TYPE:
			err = ReplaceMessenger(name, index, *(BMessenger*)data);
			break;
		case B_POINTER_TYPE:
			err = ReplacePointer(name, index, (void**)data);
			break;
		default:
			// TODO: test
			// Using the mythical BDataBuffer
			BDataBuffer DB((void*)data, data_size, true);
			err = fBody->ReplaceData<BDataBuffer>(name, index, DB, type);
			break;
	}

	return err;
}
//------------------------------------------------------------------------------
void* BMessage::operator new(size_t size)
{
	if (!sMsgCache)
	{
		sMsgCache = new BBlockCache(10, size, B_OBJECT_CACHE);
	}
	return sMsgCache->Get(size);
}
//------------------------------------------------------------------------------
void* BMessage::operator new(size_t, void* p)
{
	return p;
}
//------------------------------------------------------------------------------
void BMessage::operator delete(void* ptr, size_t size)
{
	sMsgCache->Save(ptr, size);
}
//------------------------------------------------------------------------------
bool BMessage::HasFlat(const char* name, const BFlattenable* flat) const
{
	return HasFlat(name, 0, flat);
}
//------------------------------------------------------------------------------
bool BMessage::HasFlat(const char* name, int32 n, const BFlattenable* flat) const
{
	return fBody->HasData(name, flat->TypeCode(), n);
}
//------------------------------------------------------------------------------
bool BMessage::HasData(const char* name, type_code t, int32 n) const
{
	return fBody->HasData(name, t, n);
}
//------------------------------------------------------------------------------
BRect BMessage::FindRect(const char* name, int32 n) const
{
	BRect r(0, 0, -1, -1);
	FindRect(name, n, &r);
	return r;
}
//------------------------------------------------------------------------------
BPoint BMessage::FindPoint(const char* name, int32 n) const
{
	BPoint p(0, 0);
	FindPoint(name, n, &p);
	return p;
}
//------------------------------------------------------------------------------
status_t BMessage::flatten_hdr(BDataIO* stream) const
{
	status_t err = B_OK;
	int32 data = MSG_FIELD_VERSION;

	write_helper(stream, (const void*)&data, sizeof (data), err);
	if (!err)
	{
		// faked up checksum; we'll fix it up later
		write_helper(stream, (const void*)&data, sizeof (data), err);
	}
	if (!err)
	{
		data = fBody->FlattenedSize() + calc_hdr_size(0);
		write_helper(stream, (const void*)&data, sizeof (data), err);
	}
	if (!err)
	{
		write_helper(stream, (const void*)&what, sizeof (what), err);
	}

	uint8 flags = 0;
#ifdef B_HOST_IS_BENDIAN
	flags |= MSG_FLAG_BIG_ENDIAN;
#endif
	if (HasSpecifiers())
	{
		flags |= MSG_FLAG_SCRIPT_MSG;
	}

	if (fTarget != B_NULL_TOKEN)
	{
		flags |= MSG_FLAG_INCL_TARGET;
	}

	if (fReplyTo.port >= 0 &&
		fReplyTo.target != B_NULL_TOKEN &&
		fReplyTo.team >= 0)
	{
		flags |= MSG_FLAG_INCL_REPLY;
	}

	write_helper(stream, (const void*)&flags, sizeof (flags), err);

	// Write targeting and reply info if necessary
	if (!err && (flags & MSG_FLAG_INCL_TARGET))
	{
		data = fPreferred ? B_PREFERRED_TOKEN : fTarget;
		write_helper(stream, (const void*)&data, sizeof (data), err);
	}

	if (!err && (flags & MSG_FLAG_INCL_REPLY))
	{
		write_helper(stream, (const void*)&fReplyTo.port,
					 sizeof (fReplyTo.port), err);
		if (!err)
		{
			write_helper(stream, (const void*)&fReplyTo.target,
						 sizeof (fReplyTo.target), err);
		}
		if (!err)
		{
			write_helper(stream, (const void*)&fReplyTo.team,
						 sizeof (fReplyTo.team), err);
		}

		uint8 bigFlags;
		if (!err)
		{
			bigFlags = fPreferred ? 1 : 0;
			write_helper(stream, (const void*)&bigFlags,
						 sizeof (bigFlags), err);
		}
		if (!err)
		{
			bigFlags = fReplyRequired ? 1 : 0;
			write_helper(stream, (const void*)&bigFlags,
						 sizeof (bigFlags), err);
		}
		if (!err)
		{
			bigFlags = fReplyDone ? 1 : 0;
			write_helper(stream, (const void*)&bigFlags,
						 sizeof (bigFlags), err);
		}
		if (!err)
		{
			bigFlags = fIsReply ? 1 : 0;
			write_helper(stream, (const void*)&bigFlags,
						 sizeof (bigFlags), err);
		}
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::unflatten_hdr(BDataIO* stream, bool& swap)
{
	status_t err = B_OK;
	int32 data;
	int32 checksum;
	uchar csBuffer[MSG_HEADER_MAX_SIZE];

	TReadHelper read_helper(stream);
	TChecksumHelper checksum_helper(csBuffer);

	try {

	// Get the message version
	read_helper(data);
	if (data == '1BOF')
	{
		swap = true;
	}
	else if (data == 'FOB1')
	{
		swap = false;
	}
	else
	{
		// This is *not* a message
		return B_NOT_A_MESSAGE;
	}

	// Make way for the new data
	MakeEmpty();

	read_helper.SetSwap(swap);
	
	// get the checksum
	read_helper(checksum);
	// get the size
	read_helper(data);
	checksum_helper.Cache(data);
	// Get the what
	read_helper(what);
	checksum_helper.Cache(what);
	// Get the flags
	uint8 flags = 0;
	read_helper(flags);
	checksum_helper.Cache(flags);

	fHasSpecifiers = flags & MSG_FLAG_SCRIPT_MSG;

	if (flags & MSG_FLAG_BIG_ENDIAN)
	{
		// TODO: ???
		// Isn't this already indicated by the byte order of the message version?
	}		
	if (flags & MSG_FLAG_INCL_TARGET)
	{
		// Get the target data
		read_helper(data);
		checksum_helper.Cache(data);
		fTarget = data;
	}
	if (flags & MSG_FLAG_INCL_REPLY)
	{
		// Get the reply port
		read_helper(fReplyTo.port);
		read_helper(fReplyTo.target);
		read_helper(fReplyTo.team);
		checksum_helper.Cache(fReplyTo.port);
		checksum_helper.Cache(fReplyTo.target);
		checksum_helper.Cache(fReplyTo.team);

		// Get the "big flags"
		uint8 bigFlags;
		// Get the preferred flag
		read_helper(bigFlags);
		checksum_helper.Cache(bigFlags);
		fPreferred = bigFlags;
		if (fPreferred)
		{
			fTarget = B_PREFERRED_TOKEN;
		}
		// Get the reply requirement flag
		read_helper(bigFlags);
		checksum_helper.Cache(bigFlags);
		fReplyRequired = bigFlags;
		// Get the reply done flag
		read_helper(bigFlags);
		checksum_helper.Cache(bigFlags);
		fReplyDone = bigFlags;
		// Get the "is reply" flag
		read_helper(bigFlags);
		checksum_helper.Cache(bigFlags);
		fIsReply = bigFlags;
	}
	}
	catch (status_t& e)
	{
		err = e;
	}

	if (checksum != checksum_helper.CheckSum())
		err = B_NOT_A_MESSAGE;

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::real_flatten(char* result, ssize_t size) const
{
	BMemoryIO stream((void*)result, size);
	status_t err = real_flatten(&stream);

	if (!err)
	{
		// Fixup the checksum; it is calculated on data size, what, flags,
		// and target info (including big flags if appropriate)
		((uint32*)result)[1] = _checksum_((uchar*)result + (sizeof (uint32) * 2),
										  calc_hdr_size(0) - (sizeof (uint32) * 2));
	}

	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::real_flatten(BDataIO* stream) const
{
	status_t err = flatten_hdr(stream);

	if (!err)
	{
		err = fBody->Flatten(stream);
	}

	return err;
}
//------------------------------------------------------------------------------
char* BMessage::stack_flatten(char* stack_ptr, ssize_t stack_size,
							  bool /*incl_reply*/, ssize_t* size) const
{
	const ssize_t calcd_size = calc_hdr_size(0) + fBody->FlattenedSize();
	char* new_ptr = NULL;
	if (calcd_size > stack_size)
	{
		stack_ptr = new char[calcd_size];
		new_ptr = stack_ptr;
	}
	real_flatten(stack_ptr, calcd_size);
	if (size)
	{
		*size = calcd_size;
	}
	return new_ptr;
}
//------------------------------------------------------------------------------
ssize_t BMessage::calc_hdr_size(uchar flags) const
{
	ssize_t size = min_hdr_size();

	if (fTarget != B_NULL_TOKEN)
	{
		size += sizeof (fTarget);
	}

	if (fReplyTo.port >= 0 &&
		fReplyTo.target != B_NULL_TOKEN &&
		fReplyTo.team >= 0)
	{
		size += sizeof (fReplyTo.port);
		size += sizeof (fReplyTo.target);
		size += sizeof (fReplyTo.team);

		size += 4;	// For the "big" flags
	}

	return size;
}
//------------------------------------------------------------------------------
ssize_t BMessage::min_hdr_size() const
{
	ssize_t size = 0;

	size += 4;	// version
	size += 4;	// checksum
	size += 4;	// flattened size
	size += 4;	// 'what'
	size += 1;	// flags

	return size;
}
//------------------------------------------------------------------------------
status_t BMessage::_send_(port_id port, int32 token, bool preferred,
						  bigtime_t timeout, bool reply_required,
						  BMessenger& reply_to) const
{
	BMessage tmp_msg;
	tmp_msg.fPreferred     = fPreferred;
	tmp_msg.fTarget        = fTarget;
	tmp_msg.fReplyRequired = fReplyRequired;
	tmp_msg.fReplyTo       = fReplyTo;

	BMessage* self = const_cast<BMessage*>(this);
	self->fPreferred         = preferred;
	self->fTarget            = token;
	self->fReplyRequired     = reply_required;
	self->fReplyTo.team      = reply_to.fTeam;
	self->fReplyTo.port      = reply_to.fPort;
	self->fReplyTo.target    = reply_to.fHandlerToken;
	self->fReplyTo.preferred = reply_to.fPreferredTarget;

	char tmp[0x800];
	ssize_t size;
	char* p = stack_flatten(tmp, sizeof(tmp), true /* include reply */, &size);
	char* pMem = p ? p : tmp;
	status_t err;
	do
	{
		err = write_port_etc(port, 'pjpp', pMem, size, B_RELATIVE_TIMEOUT, timeout);
	} while (err == B_INTERRUPTED);
	if (p)
	{
		delete[] p;
	}
	self->fPreferred     = tmp_msg.fPreferred;
	self->fTarget        = tmp_msg.fTarget;
	self->fReplyRequired = tmp_msg.fReplyRequired;
	self->fReplyTo       = tmp_msg.fReplyTo;
	tmp_msg.init_data();
	return err;
}
//------------------------------------------------------------------------------
status_t BMessage::send_message(port_id port, team_id port_owner, int32 token,
								bool preferred, BMessage* reply,
								bigtime_t send_timeout,
								bigtime_t reply_timeout) const
{
	const int32 cached_reply_port = sGetCachedReplyPort();
	port_id reply_port;
	status_t err;
	if (cached_reply_port == -1)
	{
		// All the cached reply ports are in use; create a new one
		reply_port = create_port(1 /* for one message */, "tmp_reply_port");
		if (reply_port < 0)
		{
			return reply_port;
		}
	}
	else
	{
		assert(cached_reply_port < sNumReplyPorts);
		reply_port = sReplyPorts[cached_reply_port];
	}

	team_id team = B_BAD_TEAM_ID;
	if (be_app != NULL)
		team = be_app->Team();
	else {
		port_info pi;
		err = get_port_info(reply_port, &pi);
		if (err)
			goto error;

		team = pi.team;
	}

	err = set_port_owner(reply_port, port_owner);
	if (err)
		goto error;

	{
		BMessenger messenger(team, reply_port, B_PREFERRED_TOKEN, false);
		err = _send_(port, token, preferred, send_timeout, true, messenger);
	}
	if (err)
	{
		goto error;
	}
	int32 code;
	err = handle_reply(reply_port, &code, reply_timeout, reply);
	if (err && cached_reply_port >= 0)
	{
		delete_port(reply_port);
		sReplyPorts[cached_reply_port] = create_port(1, "tmp_rport");
	}

error:
	if (cached_reply_port >= 0)
	{
		// Reclaim ownership of cached port
		set_port_owner(reply_port, team);
		// Flag as available
		atomic_add(&sReplyPortInUse[cached_reply_port], -1);
		return err;
	}
	delete_port(reply_port);
	return err;
}
//------------------------------------------------------------------------------
int32 BMessage::sGetCachedReplyPort()
{
	int index = -1;
	for (int32 i = 0; i < sNumReplyPorts; i++)
	{
		int32 old = atomic_add(&(sReplyPortInUse[i]), 1);
		if (old == 0)
		{
			// This entry is free
			index = i;
			break;
		}
		else
		{
			// This entry is being used.
			atomic_add(&(sReplyPortInUse[i]), -1);
		}
	}

	return index;
}
//------------------------------------------------------------------------------
static status_t handle_reply(port_id   reply_port,
                             int32*    pCode,
                             bigtime_t timeout,
                             BMessage* reply)
{
	status_t err;
	do
	{
		err = port_buffer_size_etc(reply_port, 8, timeout);
	} while (err == B_INTERRUPTED);
	if (err < 0)
	{
		return err;
	}
	// The API lied. It really isn't an error code, but the message size...
	char* pAllocd = NULL;
	char* pMem    = NULL;
	char tmp[0x800];
	if (err < 0x800)
	{
		pMem = tmp;
	}
	else
	{
		pAllocd = new char[0x800];
		pMem = pAllocd;
	}
	do
	{
		err = read_port(reply_port, pCode, pMem, err);
	} while (err == B_INTERRUPTED);

	if (err < 0)
	{
		return err;
	}

	if (*pCode == 'PUSH')
	{
		return B_ERROR;
	}
	if (*pCode != 'pjpp')
	{
		return B_OK;
	}

	err = reply->Unflatten(pMem);

	// There seems to be a bug in the original Be implementation.
	// It never free'd pAllocd !
	if (pAllocd)
	{
		delete[] pAllocd;
	}
	return err;
}
//------------------------------------------------------------------------------

#else	// USING_TEMPLATE_MADNESS

//------------------------------------------------------------------------------
BMessage::BMessage(uint32 command)
{
	init_data();

	what = command;
}
//------------------------------------------------------------------------------
BMessage::BMessage(const BMessage &message)
{
	*this = message;
}
//------------------------------------------------------------------------------
BMessage::BMessage()
{
	init_data();
}
//------------------------------------------------------------------------------
BMessage::~BMessage ()
{
}
//------------------------------------------------------------------------------
BMessage &BMessage::operator=(const BMessage &message)
{
	what = message.what;

	link = message.link;
	fTarget = message.fTarget;	
	fOriginal = message.fOriginal;
	fChangeCount = message.fChangeCount;
	fCurSpecifier = message.fCurSpecifier;
	fPtrOffset = message.fPtrOffset;

	fEntries = NULL;

    entry_hdr *src_entry;
    
    for ( src_entry = message.fEntries; src_entry != NULL; src_entry = src_entry->fNext )
	{
		entry_hdr* new_entry = (entry_hdr*)new char[da_total_logical_size ( src_entry )];
		
		if ( new_entry != NULL )
		{
			memcpy ( new_entry, src_entry, da_total_logical_size ( src_entry ) );

			new_entry->fNext = fEntries;
			new_entry->fPhysicalBytes = src_entry->fLogicalBytes;
			
			fEntries = new_entry;
		}
    }

	fReplyTo.port = message.fReplyTo.port;
	fReplyTo.target = message.fReplyTo.target;
	fReplyTo.team = message.fReplyTo.team;
	fReplyTo.preferred = message.fReplyTo.preferred;

	fPreferred = message.fPreferred;
	fReplyRequired = message.fReplyRequired;
	fReplyDone = message.fReplyDone;
	fIsReply = message.fIsReply;
	fWasDelivered = message.fWasDelivered;
	fReadOnly = message.fReadOnly;
	fHasSpecifiers = message.fHasSpecifiers;

    return *this;
}
//------------------------------------------------------------------------------
status_t BMessage::GetInfo(const char *name, type_code *typeFound,
							int32 *countFound) const
{
	for(entry_hdr *entry = fEntries; entry != NULL; entry = entry->fNext)
	{
		if (entry->fNameLength == strlen(name) &&
			strncmp(entry->fName, name, entry->fNameLength) == 0)
		{
			if (typeFound)
				*typeFound = entry->fType;

			if (countFound)
				*countFound = entry->fCount;

			return B_OK;
		}
	}

	return B_NAME_NOT_FOUND;
}
//------------------------------------------------------------------------------
status_t BMessage::GetInfo(const char *name, type_code *typeFound, bool *fixedSize) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessage::GetInfo(type_code type, int32 index, char **nameFound, type_code *typeFound, 
	int32 *countFound) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
int32 BMessage::CountNames(type_code type) const
{
	return -1;
}
//------------------------------------------------------------------------------
//bool BMessage::IsEmpty () const;
//------------------------------------------------------------------------------
//bool BMessage::IsSystem () const;
//------------------------------------------------------------------------------
bool BMessage::IsReply() const
{
	return fIsReply;
}
//------------------------------------------------------------------------------
void BMessage::PrintToStream() const
{
	char name[256];

	for (entry_hdr *entry = fEntries; entry != NULL; entry = entry->fNext)
	{
		memcpy(name, entry->fName, entry->fNameLength);
		name[entry->fNameLength] = 0;

		printf("#entry %s, type = %c%c%c%c, count = %d\n", name,
			(entry->fType & 0xFF000000) >> 24, (entry->fType & 0x00FF0000) >> 16,
			(entry->fType & 0x0000FF00) >> 8, entry->fType & 0x000000FF, entry->fCount);

		((BMessage*)this)->da_dump((dyn_array*)entry);
	}
}
//------------------------------------------------------------------------------
//status_t BMessage::Rename(const char *old_entry, const char *new_entry);
//------------------------------------------------------------------------------
//------------------------------------------------------------------------------
bool BMessage::WasDelivered () const
{
	return fWasDelivered;
}
//------------------------------------------------------------------------------
//bool BMessage::IsSourceWaiting() const;
//------------------------------------------------------------------------------
//bool BMessage::IsSourceRemote() const;
//------------------------------------------------------------------------------
BMessenger BMessage::ReturnAddress() const
{
	return BMessenger(fReplyTo.team, fReplyTo.port, fReplyTo.target,
		fReplyTo.preferred);
}
//------------------------------------------------------------------------------
const BMessage *BMessage::Previous () const
{
	return fOriginal;
}
//------------------------------------------------------------------------------
bool BMessage::WasDropped () const
{
	if (GetInfo("_drop_point_", NULL, (int32*)NULL) == B_OK)
		return true;
	else
		return false;
}
//------------------------------------------------------------------------------
BPoint BMessage::DropPoint(BPoint *offset) const
{
	BPoint point;

	FindPoint("_drop_point_", &point);

	if (offset)
		FindPoint("_drop_offset_", offset);
	
	return point;
}
//------------------------------------------------------------------------------
status_t BMessage::SendReply(uint32 command, BHandler *reply_to)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessage::SendReply(BMessage *the_reply, BHandler *reply_to,
					bigtime_t timeout)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessage::SendReply(BMessage *the_reply, BMessenger reply_to,
					bigtime_t timeout)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessage::SendReply(uint32 command, BMessage *reply_to_reply)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessage::SendReply(BMessage *the_reply, BMessage *reply_to_reply,
					bigtime_t send_timeout,
					bigtime_t reply_timeout)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
ssize_t BMessage::FlattenedSize() const
{
	ssize_t size = 7 * sizeof(int32);

	for (entry_hdr *entry = fEntries; entry != NULL; entry = entry->fNext)
		size += da_total_logical_size(entry);

	return size;
}
//------------------------------------------------------------------------------
status_t BMessage::Flatten(char *address, ssize_t numBytes) const
{
	if (address!= NULL && numBytes < FlattenedSize())
		return B_NO_MEMORY;

	int position = 7 * sizeof(int32);

	((int32*)address)[1] = what;
	((int32*)address)[2] = fCurSpecifier;
	((int32*)address)[3] = fHasSpecifiers;
	((int32*)address)[4] = 0;
	((int32*)address)[5] = 0;
	((int32*)address)[6] = 0;

	for (entry_hdr *entry = fEntries; entry != NULL; entry = entry->fNext)
	{
		memcpy(address + position, entry, da_total_logical_size(entry));

		position += da_total_logical_size(entry);
    }

    ((size_t*)address)[0] = position;
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::Flatten(BDataIO *object, ssize_t *numBytes) const
{
	if (numBytes != NULL && *numBytes < FlattenedSize())
		return B_NO_MEMORY;

	int32 size = FlattenedSize();
	char *buffer = new char[size];

	Flatten(buffer, size);

	object->Write(buffer, size);

	delete buffer;

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::Unflatten(const char *address)
{
	size_t size;
  
    MakeEmpty();

    size = ((size_t*)address)[0];
    what = ((int32*)address)[1];
	fCurSpecifier = ((int32*)address)[2];
	fHasSpecifiers = ((int32*)address)[3];

    size_t position = 7 * sizeof(int32);

    while (position < size)
	{
		entry_hdr* src_entry = (entry_hdr*)&address[position];
		entry_hdr* new_entry = (entry_hdr*)new char[da_total_logical_size ( src_entry )];

		if (new_entry != NULL)
		{
			memcpy(new_entry, src_entry, da_total_logical_size(src_entry));

			new_entry->fNext = fEntries;
			new_entry->fPhysicalBytes = src_entry->fLogicalBytes;
			position += da_total_logical_size(src_entry);
			
			fEntries = new_entry;
		}
		else
			return B_NO_MEMORY;
    }

    if (position != size)
	{
		MakeEmpty();
		return B_BAD_VALUE;
    }

    return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::Unflatten(BDataIO *object)
{
	size_t size;

	object->Read(&size, sizeof(int32));

	char *buffer = new char[size];

	*(int32*)buffer = size;

	object->Read(buffer + sizeof(int32), size - sizeof(int32));

	status_t status =  Unflatten(buffer);

	delete[] buffer;

    return status;
}
//------------------------------------------------------------------------------
status_t BMessage::AddSpecifier(const char *property)
{
	BMessage message(B_DIRECT_SPECIFIER);
	message.AddString("property", property);

	fCurSpecifier ++;
	fHasSpecifiers = true;
	
	return AddMessage("specifiers", &message);
}
//------------------------------------------------------------------------------
status_t BMessage::AddSpecifier(const char *property, int32 index)
{
	BMessage message(B_INDEX_SPECIFIER);
	message.AddString("property", property);
	message.AddInt32("index", index);

	fCurSpecifier++;
	fHasSpecifiers = true;
	
	return AddMessage("specifiers", &message);
}
//------------------------------------------------------------------------------
status_t BMessage::AddSpecifier(const char *property, int32 index, int32 range)
{
	BMessage message(B_RANGE_SPECIFIER);	
	message.AddString("property", property);
	message.AddInt32("index", index);
	message.AddInt32("range", range);

	fCurSpecifier ++;
	fHasSpecifiers = true;
	
	return AddMessage("specifiers", &message);
}
//------------------------------------------------------------------------------
status_t BMessage::AddSpecifier(const char *property, const char *name)
{
	BMessage message(B_NAME_SPECIFIER);
	message.AddString("property", property);
	message.AddString("name", name);

	fCurSpecifier ++;
	fHasSpecifiers = true;

	return AddMessage("specifiers", &message);
}
//------------------------------------------------------------------------------
//status_t BMessage::AddSpecifier(const BMessage *message);
//------------------------------------------------------------------------------
status_t SetCurrentSpecifier(int32 index)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessage::GetCurrentSpecifier(int32 *index, BMessage *specifier,
									   int32 *what, const char **property) const
{
	if (fCurSpecifier == -1)
		return B_BAD_SCRIPT_SYNTAX;

	if (index)
		*index = fCurSpecifier;

	if (specifier)
	{
		FindMessage("specifiers", fCurSpecifier, specifier);

		if (what)
			*what = specifier->what;

		if (property)
			specifier->FindString("property", property);
	}

	return B_OK;
}
//------------------------------------------------------------------------------
bool BMessage::HasSpecifiers() const
{
	return fHasSpecifiers;
}
//------------------------------------------------------------------------------
status_t BMessage::PopSpecifier()
{
	if (fCurSpecifier)
	{
		fCurSpecifier--;
		return B_OK;
	}
	else
		return B_BAD_VALUE;
}
//------------------------------------------------------------------------------
status_t BMessage::AddRect(const char *name, BRect rect)
{
	return AddData(name, B_RECT_TYPE, &rect, sizeof(BRect), true);
}
//------------------------------------------------------------------------------
status_t BMessage::AddPoint(const char *name, BPoint point)
{
	return AddData(name, B_POINT_TYPE, &point, sizeof(BPoint), true);
}
//------------------------------------------------------------------------------
status_t BMessage::AddString(const char *name, const char *string)
{
	return AddData(name, B_STRING_TYPE, string, strlen(string) + 1, false);
}
//------------------------------------------------------------------------------
//status_t BMessage::AddString(const char *name, const CString &string);
//------------------------------------------------------------------------------
status_t BMessage::AddInt8(const char *name, int8 anInt8)
{
	return AddData(name, B_INT8_TYPE, &anInt8, sizeof(int8));
}
//------------------------------------------------------------------------------
status_t BMessage::AddInt16(const char *name, int16 anInt16)
{
	return AddData(name, B_INT16_TYPE, &anInt16, sizeof(int16));
}
//------------------------------------------------------------------------------
status_t BMessage::AddInt32(const char *name, int32 anInt32)
{
	return AddData(name, B_INT32_TYPE, &anInt32, sizeof(int32));
}
//------------------------------------------------------------------------------
status_t BMessage::AddInt64(const char *name, int64 anInt64)
{
	return AddData(name, B_INT64_TYPE, &anInt64, sizeof(int64));
}
//------------------------------------------------------------------------------
status_t BMessage::AddBool(const char *name, bool aBool)
{
	return AddData(name, B_BOOL_TYPE, &aBool, sizeof(bool));
}
//------------------------------------------------------------------------------
status_t BMessage::AddFloat ( const char *name, float aFloat)
{
	return AddData(name, B_FLOAT_TYPE, &aFloat, sizeof(float));
}
//------------------------------------------------------------------------------
status_t BMessage::AddDouble ( const char *name, double aDouble)
{
	return AddData(name, B_DOUBLE_TYPE, &aDouble, sizeof(double));
}
//------------------------------------------------------------------------------
status_t BMessage::AddPointer(const char *name, const void *pointer)
{
	return AddData(name, B_POINTER_TYPE, &pointer, sizeof(void*), true);
}
//------------------------------------------------------------------------------
status_t BMessage::AddMessenger(const char *name, BMessenger messenger)
{
	return B_OK;
}
//------------------------------------------------------------------------------
//status_t BMessage::AddRef(const char *name, const entry_ref *ref);
//------------------------------------------------------------------------------
status_t BMessage::AddMessage(const char *name, const BMessage *message)
{
	int32 size = message->FlattenedSize();
	char *buffer = new char[size];

	message->Flatten (buffer, size);

	return AddData(name, B_MESSAGE_TYPE, buffer, size, false);
}
//------------------------------------------------------------------------------
status_t BMessage::AddFlat(const char *name, BFlattenable *object,
						   int32 numItems)
{
	int32 size = object->FlattenedSize ();
	char *buffer = new char[size];

	object->Flatten(buffer, size);

	return AddData(name, object->TypeCode(), buffer, size,
		object->IsFixedSize(), numItems);
}
//------------------------------------------------------------------------------
status_t BMessage::AddData(const char *name, type_code type, const void *data,
						   ssize_t numBytes, bool fixedSize, int32 numItems)
{
	entry_hdr *entry = entry_find(name, type);

	if (entry == NULL)
	{
		if ( strlen(name) > 255)
			return B_BAD_VALUE;

		entry = (entry_hdr*)da_create(sizeof(entry_hdr) - sizeof(dyn_array) +
			strlen(name), numBytes, fixedSize, numItems);

		entry->fNext = fEntries;
		entry->fType = type;
		entry->fNameLength = strlen(name);
		memcpy(entry->fName, name, entry->fNameLength);

		fEntries = entry;
	}

	return da_add_data((dyn_array**)&entry, data, numBytes);
}
//------------------------------------------------------------------------------
//status_t BMessage::RemoveData(const char *name, int32 index);
//status_t BMessage::RemoveName(const char *name);
//------------------------------------------------------------------------------
status_t BMessage::MakeEmpty()
{
	entry_hdr *entry;

    while(entry = fEntries)
	{
		fEntries = entry->fNext;
		delete entry;
    }

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindRect(const char *name, BRect *rect) const
{
	entry_hdr *entry = entry_find(name, B_RECT_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	*rect = *(BRect*)da_first_chunk(entry);
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindRect(const char *name, int32 index, BRect *rect) const
{
	entry_hdr *entry = entry_find(name, B_RECT_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	if (index < entry->fCount)
	{
		BRect* data = (BRect*)da_first_chunk(entry);
		*rect = data[index];

		return B_OK;
	}
	
	return B_BAD_INDEX;
}
//------------------------------------------------------------------------------
status_t BMessage::FindPoint(const char *name, BPoint *point) const
{
	entry_hdr *entry = entry_find(name, B_POINT_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	*point = *(BPoint*)da_first_chunk(entry);
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindPoint(const char *name, int32 index, BPoint *point) const
{
	entry_hdr *entry = entry_find(name, B_POINT_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	if (index < entry->fCount)
	{
		BPoint* data = (BPoint*)da_first_chunk(entry);
		*point = data[index];

		return B_OK;
	}
	
	return B_BAD_INDEX;
}
//------------------------------------------------------------------------------
status_t BMessage::FindString(const char *name, int32 index,
							  const char **string) const
{
	return FindData(name, B_STRING_TYPE, index, (const void**)string, NULL);
}
//------------------------------------------------------------------------------
status_t BMessage::FindString(const char *name, const char **string) const
{
	return FindData(name, B_STRING_TYPE, (const void**)string, NULL);
}
//------------------------------------------------------------------------------
//status_t BMessage::FindString ( const char *name, int32 index,
//CString *string ) const;
//------------------------------------------------------------------------------
//status_t BMessage::FindString ( const char *name, CString *string ) const;
//------------------------------------------------------------------------------
status_t BMessage::FindInt8(const char *name, int8 *anInt8) const
{
	entry_hdr *entry = entry_find(name, B_INT8_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	*anInt8 = *(int8*)da_first_chunk(entry);
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindInt8(const char *name, int32 index, int8 *anInt8) const
{
	entry_hdr *entry = entry_find(name, B_INT8_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	if (index < entry->fCount)
	{
		int8* data = (int8*)da_first_chunk(entry);
		*anInt8 = data[index];

		return B_OK;
	}
	
	return B_BAD_INDEX;
}
//------------------------------------------------------------------------------
status_t BMessage::FindInt16(const char *name, int16 *anInt16) const
{
	entry_hdr *entry = entry_find(name, B_INT16_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	*anInt16 = *(int16*)da_first_chunk(entry);
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindInt16(const char *name, int32 index, int16 *anInt16) const
{
	entry_hdr *entry = entry_find(name, B_INT16_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	if (index < entry->fCount)
	{
		int16* data = (int16*)da_first_chunk(entry);
		*anInt16 = data[index];

		return B_OK;
	}
	
	return B_BAD_INDEX;
}
//------------------------------------------------------------------------------
status_t BMessage::FindInt32(const char *name, int32 *anInt32) const
{
	entry_hdr *entry = entry_find(name, B_INT32_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	*anInt32 = *(int32*)da_first_chunk(entry);
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindInt32(const char *name, int32 index,
							 int32 *anInt32) const
{
	entry_hdr *entry = entry_find(name, B_INT32_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	if (index < entry->fCount)
	{
		int32* data = (int32*)da_first_chunk(entry);
		*anInt32 = data[index];

		return B_OK;
	}
	
	return B_BAD_INDEX;
}
//------------------------------------------------------------------------------
//status_t BMessage::FindInt64 ( const char *name, int64 *anInt64) const;
//------------------------------------------------------------------------------
//status_t BMessage::FindInt64 ( const char *name, int32 index,
//							  int64 *anInt64 ) const;
//------------------------------------------------------------------------------
status_t BMessage::FindBool(const char *name, bool *aBool) const
{
	entry_hdr *entry = entry_find(name, B_BOOL_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	*aBool = *(bool*)da_first_chunk(entry);
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindBool(const char *name, int32 index, bool *aBool) const
{
	entry_hdr *entry = entry_find(name, B_BOOL_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	if (index < entry->fCount)
	{
		bool* data = (bool*)da_first_chunk(entry);
		*aBool = data[index];

		return B_OK;
	}
	
	return B_BAD_INDEX;
}
//------------------------------------------------------------------------------
status_t BMessage::FindFloat(const char *name, float *aFloat) const
{
	entry_hdr *entry = entry_find(name, B_FLOAT_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	*aFloat = *(float*)da_first_chunk(entry);
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindFloat(const char *name, int32 index, float *aFloat) const
{
	entry_hdr *entry = entry_find(name, B_FLOAT_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	if (index < entry->fCount)
	{
		float* data = (float*)da_first_chunk(entry);
		*aFloat = data[index];

		return B_OK;
	}
	
	return B_BAD_INDEX;
}
//------------------------------------------------------------------------------
status_t BMessage::FindDouble(const char *name, double *aDouble) const
{
	entry_hdr *entry = entry_find(name, B_DOUBLE_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	*aDouble = *(double*)da_first_chunk(entry);
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindDouble(const char *name, int32 index,
							  double *aDouble) const
{
	entry_hdr *entry = entry_find(name, B_DOUBLE_TYPE);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;
  
	if (index < entry->fCount)
	{
		double* data = (double*)da_first_chunk(entry);
		*aDouble = data[index];

		return B_OK;
	}
	
	return B_BAD_INDEX;
}
//------------------------------------------------------------------------------
status_t BMessage::FindPointer(const char *name, void **pointer) const
{
	entry_hdr *entry = entry_find(name, B_POINTER_TYPE);

	if (entry == NULL)
	{
		*pointer = NULL;
		return B_NAME_NOT_FOUND;
	}
  
	*pointer = *(void**)da_first_chunk(entry);
	
	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindPointer(const char *name, int32 index,
							   void **pointer) const
{
	entry_hdr *entry = entry_find(name, B_POINTER_TYPE);

	if (entry == NULL)
	{
		*pointer = NULL;
		return B_NAME_NOT_FOUND;
	}
  
	if (index >= entry->fCount)
	{
		*pointer = NULL;
		return B_BAD_INDEX;
	}

	void** data = (void**)da_first_chunk(entry);
	*pointer = data[index];

	return B_OK;
}
//------------------------------------------------------------------------------
//status_t BMessage::FindMessenger ( const char *name, CMessenger *messenger ) const;
//------------------------------------------------------------------------------
//status_t BMessage::FindMessenger ( const char *name, int32 index, CMessenger *messenger ) const;
//------------------------------------------------------------------------------
//status_t BMessage::FindRef ( const char *name, entry_ref *ref ) const;
//------------------------------------------------------------------------------
//status_t BMessage::FindRef ( const char *name, int32 index, entry_ref *ref ) const;
//------------------------------------------------------------------------------
status_t BMessage::FindMessage(const char *name, BMessage *message) const
{
	const char *data;
  
	if ( FindData(name, B_MESSAGE_TYPE, (const void**)&data, NULL) != B_OK)
		return B_NAME_NOT_FOUND;

	return message->Unflatten(data);
}
//------------------------------------------------------------------------------
status_t BMessage::FindMessage(const char *name, int32 index,
							   BMessage *message) const
{
	const char *data;
  
	if ( FindData(name, B_MESSAGE_TYPE, index, (const void**)&data,
		NULL) != B_OK)
		return B_NAME_NOT_FOUND;

	return message->Unflatten(data);
}
//------------------------------------------------------------------------------
status_t BMessage::FindFlat(const char *name, BFlattenable *object) const
{
	status_t ret;
	const void *data;
	ssize_t numBytes;

	ret = FindData(name, object->TypeCode (), &data, &numBytes);

	if ( ret != B_OK )
		return ret;

	if ( object->Unflatten(object->TypeCode (), data, numBytes) != B_OK)
		return B_BAD_VALUE;

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindFlat(const char *name, int32 index,
							BFlattenable *object) const
{
	status_t ret;
	const void *data;
	ssize_t numBytes;

	ret = FindData(name, object->TypeCode(), index, &data, &numBytes);

	if (ret != B_OK)
		return ret;

	if (object->Unflatten(object->TypeCode(), data, numBytes) != B_OK)
		return B_BAD_VALUE;

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindData(const char *name, type_code type, const void **data,
							ssize_t *numBytes) const
{
	entry_hdr *entry = entry_find(name, type);

	if (entry == NULL)
	{
		*data = NULL;
		return B_NAME_NOT_FOUND;
	}

	int32 size;
	
	*data = da_find_data(entry, 0, &size);
	
	if (numBytes)
		*numBytes = size;

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindData(const char *name, type_code type, int32 index,
							const void **data, ssize_t *numBytes) const
{
	entry_hdr *entry = entry_find(name, type);

	if (entry == NULL)
	{
		*data = NULL;
		return B_NAME_NOT_FOUND;
	}

	if (index >= entry->fCount)
	{
		*data = NULL;
		return B_BAD_INDEX;
	}

	int32 size;
		
	*data = da_find_data(entry, index, &size);

	if (numBytes)
		*numBytes = size;

	return B_OK;	
}
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceRect(const char *name, BRect rect);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceRect(const char *name, int32 index, BRect rect);
//------------------------------------------------------------------------------
status_t BMessage::ReplacePoint(const char *name, BPoint point)
{
	return ReplaceData(name, B_POINT_TYPE, &point, sizeof(BPoint));
}
//------------------------------------------------------------------------------
//status_t BMessage::ReplacePoint(const char *name, int32 index, BPoint point); 
//------------------------------------------------------------------------------
status_t BMessage::ReplaceString(const char *name, const char *string)
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceString(const char *name, int32 index,
//								 const char *string);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceString(const char *name, CString &string);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceString(const char *name, int32 index, CString &string);
//------------------------------------------------------------------------------
status_t BMessage::ReplaceInt8(const char *name, int8 anInt8)
{
	return ReplaceData(name, B_INT8_TYPE, &anInt8, sizeof(int8));
}
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceInt8(const char *name, int32 index, int8 anInt8);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceInt16(const char *name, int16 anInt16);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceInt16(const char *name, int32 index, int16 anInt16);
//------------------------------------------------------------------------------
status_t BMessage::ReplaceInt32(const char *name, long anInt32)
{
	return ReplaceData(name, B_INT32_TYPE, &anInt32, sizeof(int32));
}
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceInt32(const char *name, int32 index, int32 anInt32);
//------------------------------------------------------------------------------
status_t BMessage::ReplaceInt64(const char *name, int64 anInt64)
{
	return ReplaceData(name, B_INT64_TYPE, &anInt64, sizeof(int64));
}
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceInt64(const char *name, int32 index, int64 anInt64); 
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceBool(const char *name, bool aBool);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceBool(const char *name, int32 index, bool aBool);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceFloat(const char *name, float aFloat);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceFloat(const char *name, int32 index, float aFloat);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceDouble(const char *name, double aDouble);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceDouble(const char *name, int32 index, double aDouble); 
//------------------------------------------------------------------------------
//status_t BMessage::ReplacePointer(const char *name, const void *pointer);
//------------------------------------------------------------------------------
//status_t BMessage::ReplacePointer(const char *name, int32 index,
//								  const void *pointer);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceMessenger(const char *name, BMessenger messenger);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceMessenger(const char *name, int32 index,
//									BMessenger messenger);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceRef(const char *name, entry_ref *ref);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceRef(const char *name, int32 index, entry_ref *ref);
//------------------------------------------------------------------------------
//status_t ReplaceMessage(const char *name, const BMessage *msg);
//------------------------------------------------------------------------------
//status_t ReplaceMessage(const char *name, int32 index, const BMessage *msg);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceFlat(const char *name, BFlattenable *object);
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceFlat(const char *name, int32 index,
//							   BFlattenable *object); 
//------------------------------------------------------------------------------
status_t BMessage::ReplaceData(const char *name, type_code type,
							   const void *data, ssize_t numBytes)
{
	entry_hdr *entry = entry_find(name, type);

	if (entry == NULL)
		return B_NAME_NOT_FOUND;

	if (entry->fType != type)
		return B_BAD_TYPE;

	da_replace_data((dyn_array**)&entry, 0, data, numBytes);

	return B_OK;
}
//------------------------------------------------------------------------------
//status_t BMessage::ReplaceData(const char *name, type_code type, int32 index,
//							   const void *data, size_t numBytes);
//------------------------------------------------------------------------------
//void *BMessage::operator new(size_t numBytes);
//------------------------------------------------------------------------------
//void BMessage::operator delete(void *memory, size_t numBytes);
//------------------------------------------------------------------------------
/*bool		HasRect(const char *, int32 n = 0) const;
bool		HasPoint(const char *, int32 n = 0) const;
bool		HasString(const char *, int32 n = 0) const;
bool		HasInt8(const char *, int32 n = 0) const;
bool		HasInt16(const char *, int32 n = 0) const;
bool		HasInt32(const char *, int32 n = 0) const;
bool		HasInt64(const char *, int32 n = 0) const;
bool		HasBool(const char *, int32 n = 0) const;
bool		HasFloat(const char *, int32 n = 0) const;
bool		HasDouble(const char *, int32 n = 0) const;
bool		HasPointer(const char *, int32 n = 0) const;
bool		HasMessenger(const char *, int32 n = 0) const;
bool		HasRef(const char *, int32 n = 0) const;
bool		HasMessage(const char *, int32 n = 0) const;
bool		HasFlat(const char *, const BFlattenable *) const;
bool		HasFlat(const char *,int32 ,const BFlattenable *) const;
bool		HasData(const char *, type_code , int32 n = 0) const;
BRect		FindRect(const char *, int32 n = 0) const;
BPoint		FindPoint(const char *, int32 n = 0) const;
const char	*FindString(const char *, int32 n = 0) const;
int8		FindInt8(const char *, int32 n = 0) const;
int16		FindInt16(const char *, int32 n = 0) const;*/
//------------------------------------------------------------------------------
int32 BMessage::FindInt32(const char *name, int32 index) const
{
	int32 anInt32 = 0;

	BMessage::FindInt32(name, index, &anInt32);

	return anInt32;
}
//------------------------------------------------------------------------------
/*int64		FindInt64(const char *, int32 n = 0) const;
bool		FindBool(const char *, int32 n = 0) const;
float		FindFloat(const char *, int32 n = 0) const;
double		FindDouble(const char *, int32 n = 0) const;*/
//------------------------------------------------------------------------------
BMessage::BMessage(BMessage *a_message)	
{
	*this=*a_message;
}
//------------------------------------------------------------------------------
void BMessage::_ReservedMessage1() {}
void BMessage::_ReservedMessage2() {}
void BMessage::_ReservedMessage3() {}
//------------------------------------------------------------------------------
void BMessage::init_data()
{
	what = 0;

	link = NULL;
	fTarget = -1;	
	fOriginal = NULL;
	fChangeCount = 0;
	fCurSpecifier = -1;
	fPtrOffset = 0;

	fEntries = NULL;

	fReplyTo.port = -1;
	fReplyTo.target = -1;
	fReplyTo.team = -1;
	fReplyTo.preferred = false;

	fPreferred = false;
	fReplyRequired = false;
	fReplyDone = false;
	fIsReply = false;
	fWasDelivered = false;
	fReadOnly = false;
	fHasSpecifiers = false;
}
//------------------------------------------------------------------------------
//	These are not declared in the header anymore and aren't actually used by the
//	"old" implementation.
#if 0
int32 BMessage::flatten_hdr(uchar *result, ssize_t size, uchar flags) const
{
	return -1;
}
//------------------------------------------------------------------------------
status_t BMessage::real_flatten(char *result, ssize_t size, uchar flags) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessage::real_flatten(BDataIO *stream, ssize_t size,
								uchar flags) const
{
	return B_ERROR;
}
#endif
//------------------------------------------------------------------------------
char* BMessage::stack_flatten(char* stack_ptr, ssize_t stack_size,
							  bool incl_reply, ssize_t* size) const
{
	return NULL;
}
//------------------------------------------------------------------------------
ssize_t BMessage::calc_size(uchar flags) const
{
	return -1;
}
//------------------------------------------------------------------------------
ssize_t BMessage::calc_hdr_size(uchar flags) const
{
	return -1;
}
//------------------------------------------------------------------------------
ssize_t BMessage::min_hdr_size() const
{
	return -1;
}
//------------------------------------------------------------------------------
status_t BMessage::nfind_data(const char *name, type_code type, int32 index,
							  const void **data, ssize_t *data_size) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessage::copy_data(const char *name, type_code type, int32 index,
							 void *data, ssize_t data_size) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessage::_send_(port_id port, int32 token, bool preferred,
						  bigtime_t timeout, bool reply_required,
						  BMessenger &reply_to) const
{
	/*_set_message_target_(this, token, preferred);
	_set_message_reply_(this, reply_to);

	int32 size;
	char *buffer;

	write_port_etc(port, what, buffer, size, B_TIMEOUT, timeout);*/

	return B_ERROR;
}
//------------------------------------------------------------------------------
status_t BMessage::send_message(port_id port, team_id port_owner, int32 token,
								bool preferred, BMessage *reply,
								bigtime_t send_timeout,
								bigtime_t reply_timeout) const
{
	return B_ERROR;
}
//------------------------------------------------------------------------------
BMessage::entry_hdr * BMessage::entry_find(const char *name, uint32 type,
											status_t *result) const
{
	for (entry_hdr *entry = fEntries; entry != NULL; entry = entry->fNext)
		if (entry->fType ==type && entry->fNameLength == strlen(name) &&
			strncmp(entry->fName, name, entry->fNameLength) == 0)
		{
			if (result)
				*result = B_OK;

			return entry;
		}

	if (result)
		*result = B_NAME_NOT_FOUND;

	return NULL;
}
//------------------------------------------------------------------------------
void BMessage::entry_remove(entry_hdr *entry)
{
	if (entry == fEntries)
		fEntries = entry->fNext;
	else
	{
		for (entry_hdr *entry_ptr = fEntries; entry_ptr != NULL; entry_ptr = entry_ptr->fNext)
			if (entry_ptr->fNext == entry)
				entry_ptr->fNext = entry->fNext;
	}
}
//------------------------------------------------------------------------------
void *BMessage::da_create(int32 header_size, int32 chunk_size, bool fixed,
						  int32 nchunks)
{
	int size = da_calc_size(header_size, chunk_size, fixed, nchunks);

	dyn_array *da = (dyn_array*)new char[size];

	da->fLogicalBytes = 0;
	da->fPhysicalBytes = size - sizeof(dyn_array) - header_size;
	
	if ( fixed )
		da->fChunkSize = chunk_size;
	else
		da->fChunkSize = 0;
	
	da->fCount = 0;
	da->fEntryHdrSize = header_size;

	return da;
}
//------------------------------------------------------------------------------
status_t BMessage::da_add_data(dyn_array **da, const void *data, int32 size )
{
//	_ASSERT(_CrtCheckMemory());

	int32 new_size = (*da)->fLogicalBytes + ((*da)->fChunkSize ? (*da)->fChunkSize :
		da_pad_8(size + da_chunk_hdr_size()));

	if (new_size > (*da)->fPhysicalBytes)
		da_grow(da, new_size - (*da)->fPhysicalBytes);

	void *ptr = da_find_data(*da, (*da)->fCount++, NULL);

	memcpy(ptr, data, size);

	if ((*da)->fChunkSize)
		(*da)->fLogicalBytes += size;
	else
	{
		da_chunk_ptr(ptr)->fDataSize = size;
		(*da)->fLogicalBytes += da_chunk_size(da_chunk_ptr(ptr));
	}

//	_ASSERT(_CrtCheckMemory());

	return B_OK;
}
//------------------------------------------------------------------------------
void *BMessage::da_find_data(dyn_array *da, int32 index, int32 *size) const
{
	if (da->fChunkSize)
	{
		if (size)
			*size = da->fChunkSize;

		return (char*)da_start_of_data(da) + index * da->fChunkSize;
	}
	else
	{
		var_chunk *chunk = da_first_chunk(da);

		for (int i = 0; i < index; i++)
			chunk = da_next_chunk(chunk);

		if (size)
			*size = chunk->fDataSize;

		return chunk->fData;
	}
}
//------------------------------------------------------------------------------
status_t BMessage::da_delete_data(dyn_array **pda, int32 index)
{
	return 0;
}
//------------------------------------------------------------------------------
status_t BMessage::da_replace_data(dyn_array **pda, int32 index,
								   const void *data, int32 dsize)
{
	void *ptr = da_find_data(*pda, index, NULL);

	memcpy(ptr, data, dsize);

	return B_OK;
}
//------------------------------------------------------------------------------
int32 BMessage::da_calc_size(int32 hdr_size, int32 chunksize, bool is_fixed,
							  int32 nchunks) const
{
	int size = sizeof(dyn_array) + hdr_size;

	if (is_fixed)
		size += chunksize * nchunks;
	else
		size += (chunksize + da_chunk_hdr_size()) * nchunks;

	return size;
}
//------------------------------------------------------------------------------
void *BMessage::da_grow(dyn_array **pda, int32 increase)
{
	dyn_array *da = (dyn_array*)new char[da_total_size(*pda) + increase];
	dyn_array *old_da = *pda;

	memcpy(da, *pda, da_total_size(*pda));

	*pda = da;
	(*pda)->fPhysicalBytes += increase;

	entry_remove((entry_hdr*)old_da);

	delete old_da;

	((entry_hdr*)*pda)->fNext = fEntries;
	fEntries = (entry_hdr*)*pda;

	return *pda;
}
//------------------------------------------------------------------------------
void BMessage::da_dump(dyn_array *da)
{
	entry_hdr *entry = (entry_hdr*)da;

	printf("\tLogicalBytes=%d\n\tPhysicalBytes=%d\n\tChunkSize=%d\n\tCount=%d\n\tEntryHdrSize=%d\n",
		entry->fLogicalBytes, entry->fPhysicalBytes, entry->fChunkSize, entry->fCount,
		entry->fEntryHdrSize);
	printf("\tNext=%p\n\tType=%c%c%c%c\n\tNameLength=%d\n\tName=%s\n",
		entry->fNext, (entry->fType & 0xFF000000) >> 24, (entry->fType & 0x00FF0000) >> 16,
			(entry->fType & 0x0000FF00) >> 8, entry->fType & 0x000000FF, entry->fNameLength,
			entry->fName);

	printf("\tData=");

	switch (entry->fType)
	{
		case B_BOOL_TYPE:
		{
			printf("%s", *(bool*)da_find_data(entry, 0, NULL) ? "true" : "false");

			for (int i = 1; i < entry->fCount; i++)
				printf(", %s", *(bool*)da_find_data ( entry, i, NULL ) ? "true" : "false");
			break;
		}
		case B_INT32_TYPE:
		{
			printf("%d", *(int32*)da_find_data(entry, 0, NULL));

			for (int i = 1; i < entry->fCount; i++)
				printf(", %d", *(int32*)da_find_data(entry, i, NULL));
			break;
		}
		case B_FLOAT_TYPE:
		{
			printf("%f", *(float*)da_find_data(entry, 0, NULL));

			for (int i = 1; i < entry->fCount; i++)
				printf(", %f", *(float*)da_find_data ( entry, i, NULL));
			break;
		}
		case B_STRING_TYPE:
		{
			printf("%s", da_find_data(entry, 0, NULL));

			for (int i = 1; i < entry->fCount; i++)
				printf(", %s", da_find_data(entry, i, NULL));
			break;
		}
		case B_POINT_TYPE:
		{
			float *data = (float*)da_find_data(entry, 0, NULL);

			printf("[%f,%f]", data[0], data[1]);

			for (int i = 1; i < entry->fCount; i++)
			{
				data = (float*)da_find_data ( entry, i, NULL);
				printf(", [%f,%f]", data[0], data[1]);
			}
			break;
		}
		case B_RECT_TYPE:
		{
			float *data = (float*)da_find_data(entry, 0, NULL);

			printf("[%f,%f,%f,%f]", data[0], data[1], data[2], data[3]);

			for (int i = 1; i < entry->fCount; i++)
			{
				data = (float*)da_find_data ( entry, i, NULL);
				printf(", [%f,%f,%f,%f]", data[0], data[1], data[2], data[3]);
			}
			break;
		}
	}

	printf("\n");
}
//------------------------------------------------------------------------------
void BMessage::da_swap_var_sized(dyn_array *da)
{
}
//------------------------------------------------------------------------------
void BMessage::da_swap_fixed_sized(dyn_array *da)
{
}
//------------------------------------------------------------------------------

#endif	// USING_TEMPLATE_MADNESS
