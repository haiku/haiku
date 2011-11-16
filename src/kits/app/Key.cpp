/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <Key.h>


static bool
CompareLists(BObjectList<BString> a, BObjectList<BString> b)
{
	if (a.CountItems() != b.CountItems())
		return false;

	for (int32 i = 0; i < a.CountItems(); i++) {
		if (*a.ItemAt(i) != *b.ItemAt(i))
			return false;
	}

	return true;
}


// #pragma mark -


BKey::BKey()
{
}


BKey::BKey(BPasswordType type, const char* identifier,
	const char* password)
{
	SetTo(type, identifier, NULL, password);
}


BKey::BKey(BPasswordType type, const char* identifier,
	const char* secondaryIdentifier, const char* password)
{
	SetTo(type, identifier, secondaryIdentifier, password);
}


BKey::BKey(BKey& other)
{
}


BKey::~BKey()
{
}


status_t
BKey::SetTo(BPasswordType type, const char* identifier,
	const char* password)
{
	return SetTo(type, identifier, NULL, password);
}


status_t
BKey::SetTo(BPasswordType type, const char* identifier,
	const char* secondaryIdentifier, const char* password)
{
	SetType(type);
	SetIdentifier(identifier);
	SetSecondaryIdentifier(secondaryIdentifier);
	return SetPassword(password);
}


status_t
BKey::SetPassword(const char* password)
{
	return SetKey((const uint8*)password, strlen(password) + 1);
}


const char*
BKey::Password() const
{
	return (const char*)fPassword.Buffer();
}


status_t
BKey::SetKey(const uint8* data, size_t length)
{
	fPassword.SetSize(0);
	ssize_t bytesWritten = fPassword.WriteAt(0, data, length);
	if (bytesWritten < 0)
		return (status_t)bytesWritten;

	return (size_t)bytesWritten == length ? B_OK : B_NO_MEMORY;
}


size_t
BKey::KeyLength() const
{
	return fPassword.BufferLength();
}


status_t
BKey::GetKey(uint8* buffer, size_t bufferLength) const
{
	ssize_t bytesRead = fPassword.ReadAt(0, buffer, bufferLength);
	if (bytesRead < 0)
		return (status_t)bytesRead;

	return B_OK;
}


void
BKey::SetIdentifier(const char* identifier)
{
	fIdentifier = identifier;
}


const char*
BKey::Identifier() const
{
	return fIdentifier.String();
}


void
BKey::SetSecondaryIdentifier(const char* identifier)
{
	fSecondaryIdentifier = identifier;
}


const char*
BKey::SecondaryIdentifier() const
{
	return fSecondaryIdentifier.String();
}


void
BKey::SetType(BPasswordType type)
{
	fType = type;
}


BPasswordType
BKey::Type() const
{
	return fType;
}


void
BKey::SetData(const BMessage& data)
{
	fData = data;
}


const BMessage&
BKey::Data() const
{
	return fData;
}


const char*
BKey::Owner() const
{
	return fOwner.String();
}


bigtime_t
BKey::CreationTime() const
{
	return fCreationTime;
}


bool
BKey::IsRegistered() const
{
	return fRegistered;
}


status_t
BKey::GetNextApplication(uint32& cookie, BString& signature) const
{
	BString* item = fApplications.ItemAt(cookie++);
	if (item == NULL)
		return B_ENTRY_NOT_FOUND;

	signature = *item;
	return B_OK;
}


status_t
BKey::RemoveApplication(const char* signature)
{
	for (int32 i = 0; i < fApplications.CountItems(); i++) {
		if (*fApplications.ItemAt(i) == signature) {
			fApplications.RemoveItemAt(i);
			return B_OK;
		}
	}
	return B_ENTRY_NOT_FOUND;
}


BKey&
BKey::operator=(const BKey& other)
{
	SetKey((const uint8*)other.Password(), other.KeyLength());
	SetType(other.Type());
	SetData(other.Data());

	fIdentifier = other.fIdentifier;
	fSecondaryIdentifier = other.fSecondaryIdentifier;
	fOwner = other.fOwner;
	fCreationTime = other.CreationTime();
	fRegistered = other.IsRegistered();
	fApplications = other.fApplications;

	return *this;
}


bool
BKey::operator==(const BKey& other) const
{
	return KeyLength() == other.KeyLength()
		&& fIdentifier == other.fIdentifier
		&& fSecondaryIdentifier == other.fSecondaryIdentifier
		&& !memcmp(Password(), other.Password(), KeyLength())
		&& fOwner == other.fOwner
		&& Data().HasSameData(other.Data())
		&& Type() == other.Type()
		&& CompareLists(fApplications, other.fApplications);
}


bool
BKey::operator!=(const BKey& other) const
{
	return !(*this == other);
}
