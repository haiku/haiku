/*
 * Copyright 2014, Augustin Cavalier (waddlesplash)
 * Distributed under the terms of the MIT License.
 */
#ifndef MESSAGE_BUILDER_H
#define MESSAGE_BUILDER_H


#include <Message.h>
#include <ObjectList.h>
#include <String.h>

namespace BPrivate {

class BMessageBuilder {
public:
								BMessageBuilder(BMessage& message);

			status_t 			PushObject(const char* name);
			status_t 			PushObject(uint32 name);
			status_t 			PopObject();
			
			uint32				What();
			void				SetWhat(uint32 what);
			
			uint32				CountNames(type_code type = B_ANY_TYPE);

			// Copied from Message.h
			status_t			AddString(const char* name, const char* string);
			status_t			AddString(const char* name,
									const BString& string);
			status_t			AddInt8(const char* name, int8 value);
			status_t			AddUInt8(const char* name, uint8 value);
			status_t			AddInt16(const char* name, int16 value);
			status_t			AddUInt16(const char* name, uint16 value);
			status_t			AddInt32(const char* name, int32 value);
			status_t			AddUInt32(const char* name, uint32 value);
			status_t			AddInt64(const char* name, int64 value);
			status_t			AddUInt64(const char* name, uint64 value);
			status_t			AddBool(const char* name, bool value);
			status_t			AddFloat(const char* name, float value);
			status_t			AddDouble(const char* name, double value);
			status_t			AddPointer(const char* name,
									const void* pointer);

private:
			BObjectList<BMessage>	fStack;
			BObjectList<BString>	fNameStack;
			BMessage*			fCurrentMessage;
};

} // namespace BPrivate

#endif	// MESSAGE_BUILDER_H
