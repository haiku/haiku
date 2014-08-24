/*
 * Copyright 2014, Augustin Cavalier (waddlesplash)
 * Distributed under the terms of the MIT License.
 */


#include <MessageBuilder.h>

#include <AutoDeleter.h>
#include <String.h>


namespace BPrivate {

// #pragma mark - BMessageBuilder


BMessageBuilder::BMessageBuilder(BMessage& message)
	:
	fNameStack(20, true),
	fCurrentMessage(&message)
{
}


/*! Creates a new BMessage, makes it a child of the
    current one with "name", and then pushes the current
    Message onto the stack and makes the new Message the
    current one.
*/
status_t
BMessageBuilder::PushObject(const char* name)
{
	BMessage* newMessage = new(std::nothrow) BMessage;
	if (newMessage == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<BMessage> messageDeleter(newMessage);
	
	BString* nameString = new(std::nothrow) BString(name);
	if (nameString == NULL)
		return B_NO_MEMORY;
	ObjectDeleter<BString> stringDeleter(nameString);
	
	if (!fNameStack.AddItem(nameString))
		return B_NO_MEMORY;
	stringDeleter.Detach();

	if (!fStack.AddItem(fCurrentMessage))
		return B_NO_MEMORY;
	messageDeleter.Detach();

	fCurrentMessage = newMessage;
	return B_OK;
}


/*! Convenience function that converts "name"
	to a string and calls PushObject(const char*)
	with it.
*/
status_t
BMessageBuilder::PushObject(uint32 name)
{
	BString nameString;
	nameString.SetToFormat("%" B_PRIu32, name);
	return PushObject(nameString.String());
}


/*! Pops the last BMessage off the stack and makes it
    the current one.
*/
status_t
BMessageBuilder::PopObject()
{
	if (fStack.CountItems() < 1)
		return B_ERROR;

	BMessage* previousMessage = fStack.LastItem();
	previousMessage->AddMessage(fNameStack.LastItem()->String(),
		fCurrentMessage);

	delete fCurrentMessage;
	fCurrentMessage = previousMessage;

	fStack.RemoveItemAt(fStack.CountItems() - 1);
	fNameStack.RemoveItemAt(fNameStack.CountItems() - 1);
	return B_OK;
}


/*! Gets the "what" of the current message.
*/
uint32
BMessageBuilder::What()
{
	return fCurrentMessage->what;
}


/*! Sets the "what" of the current message.
*/
void
BMessageBuilder::SetWhat(uint32 what)
{
	fCurrentMessage->what = what;
}


/*! Gets the value of CountNames() from the current message.
*/
uint32
BMessageBuilder::CountNames(type_code type)
{
	return fCurrentMessage->CountNames(type);
}


// #pragma mark - BMessageBuilder::Add (to fCurrentMessage)


status_t
BMessageBuilder::AddString(const char* name, const char* string)
{
	return fCurrentMessage->AddString(name, string);
}


status_t
BMessageBuilder::AddString(const char* name, const BString& string)
{
	return fCurrentMessage->AddString(name, string);
}


status_t
BMessageBuilder::AddInt8(const char* name, int8 value)
{
	return fCurrentMessage->AddInt8(name, value);
}


status_t
BMessageBuilder::AddUInt8(const char* name, uint8 value)
{
	return fCurrentMessage->AddUInt8(name, value);
}


status_t
BMessageBuilder::AddInt16(const char* name, int16 value)
{
	return fCurrentMessage->AddInt16(name, value);
}


status_t
BMessageBuilder::AddUInt16(const char* name, uint16 value)
{
	return fCurrentMessage->AddUInt16(name, value);
}


status_t
BMessageBuilder::AddInt32(const char* name, int32 value)
{
	return fCurrentMessage->AddInt32(name, value);
}


status_t
BMessageBuilder::AddUInt32(const char* name, uint32 value)
{
	return fCurrentMessage->AddUInt32(name, value);
}


status_t
BMessageBuilder::AddInt64(const char* name, int64 value)
{
	return fCurrentMessage->AddInt64(name, value);
}


status_t
BMessageBuilder::AddUInt64(const char* name, uint64 value)
{
	return fCurrentMessage->AddUInt64(name, value);
}


status_t
BMessageBuilder::AddBool(const char* name, bool value)
{
	return fCurrentMessage->AddBool(name, value);
}


status_t
BMessageBuilder::AddFloat(const char* name, float value)
{
	return fCurrentMessage->AddFloat(name, value);
}


status_t
BMessageBuilder::AddDouble(const char* name, double value)
{
	return fCurrentMessage->AddDouble(name, value);
}


status_t
BMessageBuilder::AddPointer(const char* name, const void* pointer)
{
	return fCurrentMessage->AddPointer(name, pointer);
}


} // namespace BPrivate
