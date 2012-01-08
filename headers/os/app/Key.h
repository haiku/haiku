/*
 * Copyright 2011, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _KEY_H
#define _KEY_H


#include <DataIO.h>
#include <Message.h>
#include <ObjectList.h>
#include <String.h>


enum BKeyPurpose {
	B_KEY_PURPOSE_ANY,
	B_KEY_PURPOSE_GENERIC,
	B_KEY_PURPOSE_KEYRING,
	B_KEY_PURPOSE_WEB,
	B_KEY_PURPOSE_NETWORK,
	B_KEY_PURPOSE_VOLUME
};


enum BKeyType {
	B_KEY_TYPE_ANY,
	B_KEY_TYPE_GENERIC,
	B_KEY_TYPE_PASSWORD,
	B_KEY_TYPE_CERTIFICATE
};


class BKey {
public:
								BKey();
								BKey(BKeyPurpose purpose,
									const char* identifier,
									const char* secondaryIdentifier = NULL,
									const uint8* data = NULL,
									size_t length = 0);
								BKey(BKey& other);
	virtual						~BKey();

	virtual	BKeyType			Type() const { return B_KEY_TYPE_GENERIC; };

			void				Unset();

			status_t			SetTo(BKeyPurpose purpose,
									const char* identifier,
									const char* secondaryIdentifier = NULL,
									const uint8* data = NULL,
									size_t length = 0);

			void				SetPurpose(BKeyPurpose purpose);
			BKeyPurpose			Purpose() const;

			void				SetIdentifier(const char* identifier);
			const char*			Identifier() const;

			void				SetSecondaryIdentifier(const char* identifier);
			const char*			SecondaryIdentifier() const;

			status_t			SetData(const uint8* data, size_t length);
			size_t				DataLength() const;
			const uint8*		Data() const;
			status_t			GetData(uint8* buffer, size_t bufferSize) const;

			const char*			Owner() const;
			bigtime_t			CreationTime() const;

	virtual	status_t			Flatten(BMessage& message) const;
	virtual	status_t			Unflatten(const BMessage& message);

			BKey&				operator=(const BKey& other);

			bool				operator==(const BKey& other) const;
			bool				operator!=(const BKey& other) const;

	virtual	void				PrintToStream();

private:
			friend class BKeyStore;

			BKeyPurpose			fPurpose;
			BString				fIdentifier;
			BString				fSecondaryIdentifier;
			BString				fOwner;
			bigtime_t			fCreationTime;
	mutable	BMallocIO			fData;
};


class BPasswordKey : public BKey {
public:
								BPasswordKey();
								BPasswordKey(const char* password,
									BKeyPurpose purpose, const char* identifier,
									const char* secondaryIdentifier = NULL);
								BPasswordKey(BPasswordKey& other);
	virtual						~BPasswordKey();

	virtual	BKeyType			Type() const { return B_KEY_TYPE_PASSWORD; };

			status_t			SetTo(const char* password,
									BKeyPurpose purpose,
									const char* identifier,
									const char* secondaryIdentifier = NULL);

			status_t			SetPassword(const char* password);
			const char*			Password() const;

	virtual	void				PrintToStream();
};

#endif	// _KEY_H
