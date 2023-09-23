/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef ACCESS_TOKEN_H
#define ACCESS_TOKEN_H


#include <Archivable.h>
#include <String.h>

class BPositionIO;

/*!	When a user authenticates with the HDS system, the authentication API will
    return a JWT access token which can then be later used with other APIs. This
    object models the token. The reason why the token is modelled like
    this is that the access token is not an opaque string; it contains a number
    of key-value pairs that are known as "claims". Some of the claims are used to
    detect, for example, when the access token has expired.
*/

class AccessToken : public BArchivable {
public:
								AccessToken(BMessage* from);
								AccessToken();
	virtual						~AccessToken();

			AccessToken&		operator=(const AccessToken& other);
			bool				operator==(const AccessToken& other) const;
			bool				operator!=(const AccessToken& other) const;

	const	BString&			Token() const;
			uint64				ExpiryTimestamp() const;

			void				SetToken(const BString& value);
			void				SetExpiryTimestamp(uint64 value);

			bool 				IsValid() const;
			bool				IsValid(uint64 currentTimestamp) const;

			void				Clear();

			status_t			Archive(BMessage* into, bool deep = true) const;
private:
			BString				fToken;
			uint64				fExpiryTimestamp;
				// milliseconds since epoc UTC
};


#endif // ACCESS_TOKEN_H
