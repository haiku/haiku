/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <Key.h>

#include <stdio.h>


#if 0
// TODO: move this to the KeyStore or the registrar backend if needed
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
#endif


// #pragma mark - Generic BKey


BKey::BKey()
{
	Unset();
}


BKey::BKey(BKeyPurpose purpose, const char* identifier,
	const char* secondaryIdentifier, const uint8* data, size_t length)
{
	SetTo(purpose, identifier, secondaryIdentifier, data, length);
}


BKey::BKey(BKey& other)
{
	*this = other;
}


BKey::~BKey()
{
}


void
BKey::Unset()
{
	SetTo(B_KEY_PURPOSE_GENERIC, "", "", NULL, 0);
}


status_t
BKey::SetTo(BKeyPurpose purpose, const char* identifier,
	const char* secondaryIdentifier, const uint8* data, size_t length)
{
	fCreationTime = 0;
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


status_t
BKey::Flatten(BMessage& message) const
{
	if (message.MakeEmpty() != B_OK
		|| message.AddUInt32("type", Type()) != B_OK
		|| message.AddUInt32("purpose", fPurpose) != B_OK
		|| message.AddString("identifier", fIdentifier) != B_OK
		|| message.AddString("secondaryIdentifier", fSecondaryIdentifier)
			!= B_OK
		|| message.AddString("owner", fOwner) != B_OK
		|| message.AddInt64("creationTime", fCreationTime) != B_OK
		|| message.AddData("data", B_RAW_TYPE, fData.Buffer(),
			fData.BufferLength()) != B_OK) {
		return B_ERROR;
	}

	return B_OK;
}


status_t
BKey::Unflatten(const BMessage& message)
{
	BKeyType type;
	if (message.FindUInt32("type", (uint32*)&type) != B_OK || type != Type())
		return B_BAD_VALUE;

	const void* data = NULL;
	ssize_t dataLength = 0;
	if (message.FindUInt32("purpose", (uint32*)&fPurpose) != B_OK
		|| message.FindString("identifier", &fIdentifier) != B_OK
		|| message.FindString("secondaryIdentifier", &fSecondaryIdentifier)
			!= B_OK
		|| message.FindString("owner", &fOwner) != B_OK
		|| message.FindInt64("creationTime", &fCreationTime) != B_OK
		|| message.FindData("data", B_RAW_TYPE, &data, &dataLength) != B_OK
		|| dataLength < 0) {
		return B_ERROR;
	}

	return SetData((const uint8*)data, (size_t)dataLength);
}


BKey&
BKey::operator=(const BKey& other)
{
	SetPurpose(other.Purpose());
	SetData((const uint8*)other.Data(), other.DataLength());

	fIdentifier = other.fIdentifier;
	fSecondaryIdentifier = other.fSecondaryIdentifier;
	fOwner = other.fOwner;
	fCreationTime = other.fCreationTime;

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
		&& memcmp(Data(), other.Data(), DataLength()) == 0;
}


bool
BKey::operator!=(const BKey& other) const
{
	return !(*this == other);
}


void
BKey::PrintToStream()
{
	if (Type() == B_KEY_TYPE_GENERIC)
		printf("generic key:\n");

	const char* purposeString = "unknown";
	switch (fPurpose) {
		case B_KEY_PURPOSE_ANY:
			purposeString = "any";
			break;
		case B_KEY_PURPOSE_GENERIC:
			purposeString = "generic";
			break;
		case B_KEY_PURPOSE_KEYRING:
			purposeString = "keyring";
			break;
		case B_KEY_PURPOSE_WEB:
			purposeString = "web";
			break;
		case B_KEY_PURPOSE_NETWORK:
			purposeString = "network";
			break;
		case B_KEY_PURPOSE_VOLUME:
			purposeString = "volume";
			break;
	}

	printf("\tpurpose: %s\n", purposeString);
	printf("\tidentifier: \"%s\"\n", fIdentifier.String());
	printf("\tsecondary identifier: \"%s\"\n", fSecondaryIdentifier.String());
	printf("\towner: \"%s\"\n", fOwner.String());
	printf("\tcreation time: %" B_PRIu64 "\n", fCreationTime);
	printf("\traw data length: %" B_PRIuSIZE "\n", fData.BufferLength());
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


void
BPasswordKey::PrintToStream()
{
	printf("password key:\n");
	BKey::PrintToStream();
	printf("\tpassword: \"%s\"\n", Password());
}
