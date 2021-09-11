/*
 * Copyright 2010-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_HTTP_AUTHENTICATION_H_
#define _B_HTTP_AUTHENTICATION_H_


#include <Locker.h>
#include <String.h>
#include <Url.h>


namespace BPrivate {

namespace Network {


// HTTP authentication method
enum BHttpAuthenticationMethod {
	B_HTTP_AUTHENTICATION_NONE = 0,
		// No authentication
	B_HTTP_AUTHENTICATION_BASIC = 1,
		// Basic base64 authentication method (unsecure)
	B_HTTP_AUTHENTICATION_DIGEST = 2,
		// Digest authentication
	B_HTTP_AUTHENTICATION_IE_DIGEST = 4
		// Slightly modified digest authentication to mimic old IE one
};


enum BHttpAuthenticationAlgorithm {
	B_HTTP_AUTHENTICATION_ALGORITHM_NONE,
	B_HTTP_AUTHENTICATION_ALGORITHM_MD5,
	B_HTTP_AUTHENTICATION_ALGORITHM_MD5_SESS
};


enum BHttpAuthenticationQop {
	B_HTTP_QOP_NONE,
	B_HTTP_QOP_AUTH,
	B_HTTP_QOP_AUTHINT
};


class BHttpAuthentication {
public:
								BHttpAuthentication();
								BHttpAuthentication(const BString& username,
									const BString& password);
								BHttpAuthentication(
									const BHttpAuthentication& other);
								BHttpAuthentication& operator=(
									const BHttpAuthentication& other);

	// Field modification
			void				SetUserName(const BString& username);
			void				SetPassword(const BString& password);
			void				SetMethod(
									BHttpAuthenticationMethod type);
			status_t			Initialize(const BString& wwwAuthenticate);

	// Field access
			const BString&		UserName() const;
			const BString&		Password() const;
			BHttpAuthenticationMethod Method() const;

			BString				Authorization(const BUrl& url,
									const BString& method) const;

	// Base64 encoding
	// TODO: Move to a common place. We may have multiple implementations
	// in the Haiku tree...
	static	BString				Base64Encode(const BString& string);
	static	BString				Base64Decode(const BString& string);


private:
			BString				_DigestResponse(const BString& uri,
									const BString& method) const;
			// TODO: Rename these? _H seems to return a hash value,
			// _KD returns a hash value of the "data" prepended by
			// the "secret" string...
			BString				_H(const BString& value) const;
			BString				_KD(const BString& secret,
									const BString& data) const;

private:
			BHttpAuthenticationMethod fAuthenticationMethod;
			BString				fUserName;
			BString				fPassword;

			BString				fRealm;
			BString				fDigestNonce;
	mutable	BString				fDigestCnonce;
	mutable int					fDigestNc;
			BString				fDigestOpaque;
			bool				fDigestStale;
			BHttpAuthenticationAlgorithm fDigestAlgorithm;
			BHttpAuthenticationQop fDigestQop;

			BString				fAuthorizationString;

	mutable	BLocker				fLock;
};


} // namespace Network

} // namespace BPrivate


#endif // _B_HTTP_AUTHENTICATION_H_
