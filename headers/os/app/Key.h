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


enum BPasswordType {
	B_WEB_PASSWORD,
	B_NETWORK_PASSWORD,
	B_VOLUME_PASSWORD,
	B_GENERIC_PASSWORD
};


class BKey {
public:
								BKey();
								BKey(BPasswordType type,
									const char* identifier,
									const char* password);
								BKey(BPasswordType type,
									const char* identifier,
									const char* secondaryIdentifier,
									const char* password);
								BKey(BKey& other);
	virtual						~BKey();

			void				Unset();

			status_t			SetTo(BPasswordType type,
									const char* identifier,
									const char* password);
			status_t			SetTo(BPasswordType type,
									const char* identifier,
									const char* secondaryIdentifier,
									const char* password);

			status_t			SetPassword(const char* password);
			const char*			Password() const;

			status_t			SetKey(const uint8* data, size_t length);
			size_t				KeyLength() const;
			status_t			GetKey(uint8* buffer,
									size_t bufferLength) const;

			void				SetIdentifier(const char* identifier);
			const char*			Identifier() const;

			void				SetSecondaryIdentifier(const char* identifier);
			const char*			SecondaryIdentifier() const;

			void				SetType(BPasswordType type);
			BPasswordType		Type() const;

			void				SetData(const BMessage& data);
			const BMessage&		Data() const;

			const char*			Owner() const;
			bigtime_t			CreationTime() const;
			bool				IsRegistered() const;

// TODO: move to BKeyStore
			status_t			GetNextApplication(uint32& cookie,
									BString& signature) const;
			status_t			RemoveApplication(const char* signature);

			BKey&				operator=(const BKey& other);

			bool				operator==(const BKey& other) const;
			bool				operator!=(const BKey& other) const;

private:
	mutable	BMallocIO			fPassword;
			BString				fIdentifier;
			BString				fSecondaryIdentifier;
			BString				fOwner;
			BMessage			fData;
			bigtime_t			fCreationTime;
			BPasswordType		fType;
			BObjectList<BString> fApplications;
			bool				fRegistered;
};


#endif	// _KEY_H
