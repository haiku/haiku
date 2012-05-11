/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 */


#include <HttpAuthentication.h>

#include <cstdlib>

#include <cstdio>
#define PRINT(x) printf x

#ifdef OPENSSL_ENABLED
#define HAVE_OPENSSL // Instruct md5.h to include the openssl version
#endif

extern "C" {
#include "md5.h"
};

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


// #pragma mark Field modification


void
BHttpAuthentication::SetUserName(const BString& username)
{
	fUserName = username;
}


void
BHttpAuthentication::SetPassword(const BString& password)
{
	fPassword = password;
}


void
BHttpAuthentication::SetMethod(BHttpAuthenticationMethod method)
{
	fAuthenticationMethod = method;
}


status_t
BHttpAuthentication::Initialize(const BString& wwwAuthenticate)
{
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
		wwwAuthenticate.CopyInto(additionalData, firstSpace+1, 
			wwwAuthenticate.Length());
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
		int32 firstColon = additionalData.FindFirst(',');
		if (firstColon == -1)
			firstColon = additionalData.Length();
			
		BString value;
		additionalData.MoveInto(value, 0, firstColon);
		additionalData.Remove(0, 1);
		additionalData.Trim();
		
		int32 equal = value.FindFirst('=');
		if (equal == -1)
			continue;
			
		BString name;
		value.MoveInto(name, 0, equal);
		value.Remove(0, 1);
		name.ToLower();
		
		if (value[0] == '"') {
			value.Remove(0, 1);
			value.Remove(value.Length()-1, 1);
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
		&& fDigestAlgorithm != B_HTTP_AUTHENTICATION_ALGORITHM_NONE)
		return B_OK;
	else
		return B_ERROR;
}


// #pragma mark Field access


const BString&
BHttpAuthentication::UserName() const
{
	return fUserName;
}


const BString&
BHttpAuthentication::Password() const
{
	return fPassword;
}


BHttpAuthenticationMethod
BHttpAuthentication::Method() const
{
	return fAuthenticationMethod;
}


BString
BHttpAuthentication::Authorization(const BUrl& url, const BString& method) const
{
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
	BString base64Reverse(kBase64Symbols);
	BString result;
	
	// Invalid input
	if (string.Length() % 4 != 0)
		return BString("");
		
		
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
	static unsigned char hashResult[MD5_DIGEST_LENGTH];
	MD5_Init(&context);
	MD5_Update(&context, (void *)(value.String()), value.Length());
	MD5_Final(hashResult, &context);
	
	BString result;
	// TODO: This is slower than it needs to be. If we already know the
	// final hash string length, we can use
	// BString::LockBuffer(MD5_DIGEST_LENGTH * 2) to preallocate it.
	// I am not making the change since I can not test it right now (stippi).
	for (int i = 0; i < MD5_DIGEST_LENGTH; i++) {
		char c = ((hashResult[i] & 0xF0) >> 4);
		c += (c > 9) ? 'a' - 10 : '0';
		result << c;
		
		c = hashResult[i] & 0x0F;
		c += (c > 9) ? 'a' - 10 : '0';
		result << c;
	}
	
	return result;
}


BString
BHttpAuthentication::_KD(const BString& secret, const BString& data) const
{
	BString encode;
	encode << secret << ':' << data;
	
	return _H(encode);
}
