
// Standard Includes -----------------------------------------------------------

// System Includes -------------------------------------------------------------
#include <Message.h>
#include <Messenger.h>
#include <Errors.h>

#include <CRTDBG.H>

// Project Includes ------------------------------------------------------------

// Local Includes --------------------------------------------------------------

// Local Defines ---------------------------------------------------------------

// Globals ---------------------------------------------------------------------
void _msg_cache_cleanup_()
{
}
//------------------------------------------------------------------------------
BMessage *_reconstruct_msg_(uint32,uint32,uint32)
{
	return NULL;
}
//------------------------------------------------------------------------------
void _set_message_target_(BMessage *msg, int32 target, bool preferred)
{
	if (preferred)
	{
		msg->fTarget = -1;
		msg->fPreferred = true;
	}
	else
	{
		msg->fTarget = target;
		msg->fPreferred = false;
	}
}
//------------------------------------------------------------------------------
void _set_message_reply_(BMessage *msg, BMessenger messenger)
{
	msg->fReplyTo.port = messenger.fPort;
	msg->fReplyTo.target = messenger.fHandlerToken;
	msg->fReplyTo.team = messenger.fTeam;
	msg->fReplyTo.preferred = messenger.fPreferredTarget;
	msg->fReplyRequired = true;
}
//------------------------------------------------------------------------------
int32 _get_message_target_(BMessage *msg)
{
	if (msg->fPreferred)
		return -1;
	else
		return msg->fTarget;
}
//------------------------------------------------------------------------------
bool _use_preferred_target_(BMessage *msg)
{
	return msg->fPreferred;
}
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
size_t BMessage::FlattenedSize() const
{
	size_t size = 7 * sizeof(int32);

	for (entry_hdr *entry = fEntries; entry != NULL; entry = entry->fNext)
		size += da_total_logical_size(entry);

	return size;
}
//------------------------------------------------------------------------------
status_t BMessage::Flatten(char *address, size_t numBytes) const
{
	if (numBytes != NULL && numBytes < FlattenedSize())
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
status_t BMessage::Flatten(BDataIO *object, size_t *numBytes) const
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
						   size_t numBytes, bool fixedSize, int32 numItems)
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
	size_t numBytes;

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
	size_t numBytes;

	ret = FindData(name, object->TypeCode(), index, &data, &numBytes);

	if (ret != B_OK)
		return ret;

	if (object->Unflatten(object->TypeCode(), data, numBytes) != B_OK)
		return B_BAD_VALUE;

	return B_OK;
}
//------------------------------------------------------------------------------
status_t BMessage::FindData(const char *name, type_code type, const void **data,
							size_t *numBytes) const
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
							const void **data, size_t *numBytes) const
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
							   const void *data, size_t numBytes)
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
	BMessage::BMessage(*a_message);
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
//------------------------------------------------------------------------------
char *BMessage::stack_flatten(char *stack_ptr, ssize_t stack_size,
							  bool incl_reply, ssize_t *size) const
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
	_ASSERT(_CrtCheckMemory());

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

	_ASSERT(_CrtCheckMemory());

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
