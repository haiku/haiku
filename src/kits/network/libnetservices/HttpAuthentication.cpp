/*
 * Copyright 2010-2013 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <HttpAuthentication.h>

#include <stdlib.h>
#include <stdio.h>

#include <AutoLocker.h>

using namespace BPrivate::Network;


#if DEBUG > 0
#define PRINT(x) printf x
#else
#define PRINT(x)
#endif

#ifdef OPENSSL_ENABLED
extern "C" {
#include <openssl/md5.h>
};
#else
#include "md5.h"
#endif

#ifndef MD5_DIGEST_LENGTH
#define MD5_DIGEST_LENGTH 16
#endif

static const char* kBase64Symbols
	= "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_";


BHttpAuthentication::BHttpAuthentication()
	:
	fAuthenticationMethod(B_HTTP_AUTHENTICATION_NONE)
{
}


BHttpAuthentication::BHttpAuthentication(const BString& username, const BString& password)
	:
	fAuthenticationMethod(B_HTTP_AUTHENTICATION_NONE),
	fUserName(username),
	fPassword(password)
{
}


BHttpAuthentication::BHttpAuthentication(const BHttpAuthentication& other)
	:
	fAuthenticationMethod(other.fAuthenticationMethod),
	fUserName(other.fUserName),
	fPassword(other.fPassword),
	fRealm(other.fRealm),
	fDigestNonce(other.fDigestNonce),
	fDigestCnonce(other.fDigestCnonce),
	fDigestNc(other.fDigestNc),
	fDigestOpaque(other.fDigestOpaque),
	fDigestStale(other.fDigestStale),
	fDigestAlgorithm(other.fDigestAlgorithm),
	fDigestQop(other.fDigestQop),
	fAuthorizationString(other.fAuthorizationString)
{
}


BHttpAuthentication& BHttpAuthentication::operator=(
	const BHttpAuthentication& other)
{
	fAuthenticationMethod = other.fAuthenticationMethod;
	fUserName = other.fUserName;
	fPassword = other.fPassword;
	fRealm = other.fRealm;
	fDigestNonce = other.fDigestNonce;
	fDigestCnonce = other.fDigestCnonce;
	fDigestNc = other.fDigestNc;
	fDigestOpaque = other.fDigestOpaque;
	fDigestStale = other.fDigestStale;
	fDigestAlgorithm = other.fDigestAlgorithm;
	fDigestQop = other.fDigestQop;
	fAuthorizationString = other.fAuthorizationString;
	return *this;
}


// #pragma mark Field modification


void
BHttpAuthentication::SetUserName(const BString& username)
{
	fLock.Lock();
	fUserName = username;
	fLock.Unlock();
}


void
BHttpAuthentication::SetPassword(const BString& password)
{
	fLock.Lock();
	fPassword = password;
	fLock.Unlock();
}


void
BHttpAuthentication::SetMethod(BHttpAuthenticationMethod method)
{
	fLock.Lock();
	fAuthenticationMethod = method;
	fLock.Unlock();
}


status_t
BHttpAuthentication::Initialize(const BString& wwwAuthenticate)
{
	BPrivate::AutoLocker<BLocker> lock(fLock);

	fAuthenticationMethod = B_HTTP_AUTHENTICATION_NONE;
	fDigestQop = B_HTTP_QOP_NONE;

	if (wwwAuthenticate.Length() == 0)
		return B_BAD_VALUE;

	BString authRequired;
	BString additionalData;
	int32 firstSpace = wwwAuthenticate.FindFirst(' ');

	if (firstSpace == -1)
		wwwAuthenticate.CopyInto(authRequired, 0, wwwAuthenticate.Length());
	else {
		wwwAuthenticate.CopyInto(authRequired, 0, firstSpace);
		wwwAuthenticate.CopyInto(additionalData, firstSpace + 1,
			wwwAuthenticate.Length() - (firstSpace + 1));
	}

	authRequired.ToLower();
	if (authRequired == "basic")
		fAuthenticationMethod = B_HTTP_AUTHENTICATION_BASIC;
	else if (authRequired == "digest") {
		fAuthenticationMethod = B_HTTP_AUTHENTICATION_DIGEST;
		fDigestAlgorithm = B_HTTP_AUTHENTICATION_ALGORITHM_MD5;
	} else
		return B_ERROR;


	while (additionalData.Length()) {
		int32 firstComma = additionalData.FindFirst(',');
		if (firstComma == -1)
			firstComma = additionalData.Length();

		BString value;
		additionalData.MoveInto(value, 0, firstComma);
		additionalData.Remove(0, 1);
		additionalData.Trim();

		int32 equal = value.FindFirst('=');
		if (equal <= 0)
			continue;

		BString name;
		value.MoveInto(name, 0, equal);
		value.Remove(0, 1);
		name.ToLower();

		if (value.Length() > 0 && value[0] == '"') {
			value.Remove(0, 1);
			value.Remove(value.Length() - 1, 1);
		}

		PRINT(("HttpAuth: name=%s, value=%s\n", name.String(),
			value.String()));

		if (name == "realm")
			fRealm = value;
		else if (name == "nonce")
			fDigestNonce = value;
		else if (name == "opaque")
			fDigestOpaque = value;
		else if (name == "stale") {
			value.ToLower();
			fDigestStale = (value == "true");
		} else if (name == "algorithm") {
			value.ToLower();

			if (value == "md5")
				fDigestAlgorithm = B_HTTP_AUTHENTICATION_ALGORITHM_MD5;
			else if (value == "md5-sess")
				fDigestAlgorithm = B_HTTP_AUTHENTICATION_ALGORITHM_MD5_SESS;
			else
				fDigestAlgorithm = B_HTTP_AUTHENTICATION_ALGORITHM_NONE;
		} else if (name == "qop")
			fDigestQop = B_HTTP_QOP_AUTH;
	}

	if (fAuthenticationMethod == B_HTTP_AUTHENTICATION_BASIC)
		return B_OK;
	else if (fAuthenticationMethod == B_HTTP_AUTHENTICATION_DIGEST
			&& fDigestNonce.Length() > 0
			&& fDigestAlgorithm != B_HTTP_AUTHENTICATION_ALGORITHM_NONE) {
		return B_OK;
	} else
		return B_ERROR;
}


// #pragma mark Field access


const BString&
BHttpAuthentication::UserName() const
{
	BPrivate::AutoLocker<BLocker> lock(fLock);
	return fUserName;
}


const BString&
BHttpAuthentication::Password() const
{
	BPrivate::AutoLocker<BLocker> lock(fLock);
	return fPassword;
}


BHttpAuthenticationMethod
BHttpAuthentication::Method() const
{
	BPrivate::AutoLocker<BLocker> lock(fLock);
	return fAuthenticationMethod;
}


BString
BHttpAuthentication::Authorization(const BUrl& url, const BString& method) const
{
	BPrivate::AutoLocker<BLocker> lock(fLock);
	BString authorizationString;

	switch (fAuthenticationMethod) {
		case B_HTTP_AUTHENTICATION_NONE:
			break;

		case B_HTTP_AUTHENTICATION_BASIC:
		{
			BString basicEncode;
			basicEncode << fUserName << ':' << fPassword;
			authorizationString << "Basic " << Base64Encode(basicEncode);
			break;
		}

		case B_HTTP_AUTHENTICATION_DIGEST:
		case B_HTTP_AUTHENTICATION_IE_DIGEST:
			authorizationString << "Digest " << "username=\"" << fUserName
				<< "\", realm=\"" << fRealm << "\", nonce=\"" << fDigestNonce
				<< "\", algorithm=";

			if (fDigestAlgorithm == B_HTTP_AUTHENTICATION_ALGORITHM_MD5)
				authorizationString << "MD5";
			else
				authorizationString << "MD5-sess";

			if (fDigestOpaque.Length() > 0)
				authorizationString << ", opaque=\"" << fDigestOpaque << "\"";

			if (fDigestQop != B_HTTP_QOP_NONE) {
				if (fDigestCnonce.Length() == 0) {
					fDigestCnonce = _H(fDigestOpaque);
					//fDigestCnonce = "03c6790a055cbbac";
					fDigestNc = 0;
				}

				authorizationString << ", uri=\"" << url.Path() << "\"";
				authorizationString << ", qop=auth, cnonce=\"" << fDigestCnonce
					<< "\"";

				char strNc[9];
				snprintf(strNc, 9, "%08x", ++fDigestNc);
				authorizationString << ", nc=" << strNc;

			}

			authorizationString << ", response=\""
				<< _DigestResponse(url.Path(), method) << "\"";
			break;
	}

	return authorizationString;
}


// #pragma mark Base64 encoding


/*static*/ BString
BHttpAuthentication::Base64Encode(const BString& string)
{
	BString result;
	BString tmpString = string;

	while (tmpString.Length()) {
		char in[3] = { 0, 0, 0 };
		char out[4] = { 0, 0, 0, 0 };
		int8 remaining = tmpString.Length();

		tmpString.MoveInto(in, 0, 3);

		out[0] = (in[0] & 0xFC) >> 2;
		out[1] = ((in[0] & 0x03) << 4) | ((in[1] & 0xF0) >> 4);
		out[2] = ((in[1] & 0x0F) << 2) | ((in[2] & 0xC0) >> 6);
		out[3] = in[2] & 0x3F;

		for (int i = 0; i < 4; i++)
			out[i] = kBase64Symbols[(int)out[i]];

		//  Add padding if the input length is not a multiple
		// of 3
		switch (remaining) {
			case 1:
				out[2] = '=';
				// Fall through
			case 2:
				out[3] = '=';
				break;
		}

		result.Append(out, 4);
	}

	return result;
}


/*static*/ BString
BHttpAuthentication::Base64Decode(const BString& string)
{
	BString result;

	// Check for invalid input
	if (string.Length() % 4 != 0)
		return result;

	BString base64Reverse(kBase64Symbols);

	BString tmpString(string);
	while (tmpString.Length()) {
		char in[4] = { 0, 0, 0, 0 };
		char out[3] = { 0, 0, 0 };

		tmpString.MoveInto(in, 0, 4);

		for (int i = 0; i < 4; i++) {
			if (in[i] == '=')
				in[i] = 0;
			else
				in[i] = base64Reverse.FindFirst(in[i], 0);
		}

		out[0] = (in[0] << 2) | ((in[1] & 0x30) >> 4);
		out[1] = ((in[1] & 0x0F) << 4) | ((in[2] & 0x3C) >> 2);
		out[2] = ((in[2] & 0x03) << 6) | in[3];

		result.Append(out, 3);
	}

	return result;
}


BString
BHttpAuthentication::_DigestResponse(const BString& uri, const BString& method) const
{
	PRINT(("HttpAuth: Computing digest response: \n"));
	PRINT(("HttpAuth: > username  = %s\n", fUserName.String()));
	PRINT(("HttpAuth: > password  = %s\n", fPassword.String()));
	PRINT(("HttpAuth: > realm     = %s\n", fRealm.String()));
	PRINT(("HttpAuth: > nonce     = %s\n", fDigestNonce.String()));
	PRINT(("HttpAuth: > cnonce    = %s\n", fDigestCnonce.String()));
	PRINT(("HttpAuth: > nc        = %08x\n", fDigestNc));
	PRINT(("HttpAuth: > uri       = %s\n", uri.String()));
	PRINT(("HttpAuth: > method    = %s\n", method.String()));
	PRINT(("HttpAuth: > algorithm = %d (MD5:%d, MD5-sess:%d)\n",
		fDigestAlgorithm, B_HTTP_AUTHENTICATION_ALGORITHM_MD5,
		B_HTTP_AUTHENTICATION_ALGORITHM_MD5_SESS));

	BString A1;
	A1 << fUserName << ':' << fRealm << ':' << fPassword;

	if (fDigestAlgorithm == B_HTTP_AUTHENTICATION_ALGORITHM_MD5_SESS) {
		A1 = _H(A1);
		A1 << ':' << fDigestNonce << ':' << fDigestCnonce;
	}


	BString A2;
	A2 << method << ':' << uri;

	PRINT(("HttpAuth: > A1        = %s\n", A1.String()));
	PRINT(("HttpAuth: > A2        = %s\n", A2.String()));
	PRINT(("HttpAuth: > H(A1)     = %s\n", _H(A1).String()));
	PRINT(("HttpAuth: > H(A2)     = %s\n", _H(A2).String()));

	char strNc[9];
	snprintf(strNc, 9, "%08x", fDigestNc);

	BString secretResp;
	secretResp << fDigestNonce << ':' << strNc << ':' << fDigestCnonce
		<< ":auth:" << _H(A2);

	PRINT(("HttpAuth: > R2        = %s\n", secretResp.String()));

	BString response = _KD(_H(A1), secretResp);
	PRINT(("HttpAuth: > response  = %s\n", response.String()));

	return response;
}


BString
BHttpAuthentication::_H(const BString& value) const
{
	MD5_CTX context;
	uchar hashResult[MD5_DIGEST_LENGTH];
	MD5_Init(&context);
	MD5_Update(&context, (void *)(value.String()), value.Length());
	MD5_Final(hashResult, &context);

	BString result;
	// Preallocate the string
	char* resultChar = result.LockBuffer(MD5_DIGEST_LENGTH * 2);
	if (resultChar == NULL)
		return BString();

	for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
		char c = ((hashResult[i] & 0xF0) >> 4);
		c += (c > 9) ? 'a' - 10 : '0';
		resultChar[0] = c;
		resultChar++;

		c = hashResult[i] & 0x0F;
		c += (c > 9) ? 'a' - 10 : '0';
		resultChar[0] = c;
		resultChar++;
	}
	result.UnlockBuffer(MD5_DIGEST_LENGTH * 2);

	return result;
}


BString
BHttpAuthentication::_KD(const BString& secret, const BString& data) const
{
	BString encode;
	encode << secret << ':' << data;

	return _H(encode);
}
