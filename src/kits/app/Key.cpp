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


// #pragma mark - Generic BKey


BKey::BKey()
{
}


BKey::BKey(BKeyPurpose purpose, const char* identifier,
	const char* secondaryIdentifier, const uint8* data, size_t length)
{
	SetTo(purpose, identifier, secondaryIdentifier, data, length);
}


BKey::BKey(BKey& other)
{
}


BKey::~BKey()
{
}


status_t
BKey::SetTo(BKeyPurpose purpose, const char* identifier,
	const char* secondaryIdentifier, const uint8* data, size_t length)
{
	SetPurpose(purpose);
	SetIdentifier(identifier);
	SetSecondaryIdentifier(secondaryIdentifier);
	return SetData(data, length);
}


void
BKey::SetPurpose(BKeyPurpose purpose)
{
	fPurpose = purpose;
}


BKeyPurpose
BKey::Purpose() const
{
	return fPurpose;
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


status_t
BKey::SetData(const uint8* data, size_t length)
{
	fData.SetSize(0);
	ssize_t bytesWritten = fData.WriteAt(0, data, length);
	if (bytesWritten < 0)
		return (status_t)bytesWritten;

	return (size_t)bytesWritten == length ? B_OK : B_NO_MEMORY;
}


size_t
BKey::DataLength() const
{
	return fData.BufferLength();
}


const uint8*
BKey::Data() const
{
	return (const uint8*)fData.Buffer();
}


status_t
BKey::GetData(uint8* buffer, size_t bufferSize) const
{
	ssize_t bytesRead = fData.ReadAt(0, buffer, bufferSize);
	if (bytesRead < 0)
		return (status_t)bytesRead;

	return B_OK;
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


#if 0
// To be moved to BKeyStore
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
#endif


BKey&
BKey::operator=(const BKey& other)
{
	SetPurpose(other.Purpose());
	SetData((const uint8*)other.Data(), other.DataLength());

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
	return Type() == other.Type()
		&& DataLength() == other.DataLength()
		&& Purpose() == other.Purpose()
		&& fOwner == other.fOwner
		&& fIdentifier == other.fIdentifier
		&& fSecondaryIdentifier == other.fSecondaryIdentifier
		&& memcmp(Data(), other.Data(), DataLength()) == 0
		&& CompareLists(fApplications, other.fApplications);
}


bool
BKey::operator!=(const BKey& other) const
{
	return !(*this == other);
}


// #pragma mark - BPasswordKey


BPasswordKey::BPasswordKey()
{
}


BPasswordKey::BPasswordKey(const char* password, BKeyPurpose purpose,
	const char* identifier, const char* secondaryIdentifier)
	:
	BKey(purpose, identifier, secondaryIdentifier, (const uint8*)password,
		strlen(password) + 1)
{
}


BPasswordKey::BPasswordKey(BPasswordKey& other)
{
}


BPasswordKey::~BPasswordKey()
{
}


status_t
BPasswordKey::SetTo(const char* password, BKeyPurpose purpose,
	const char* identifier, const char* secondaryIdentifier)
{
	return BKey::SetTo(purpose, identifier, secondaryIdentifier,
		(const uint8*)password, strlen(password) + 1);
}


status_t
BPasswordKey::SetPassword(const char* password)
{
	return SetData((const uint8*)password, strlen(password) + 1);
}


const char*
BPasswordKey::Password() const
{
	return (const char*)Data();
}
