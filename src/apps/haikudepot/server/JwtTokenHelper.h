/*
 * Copyright 2023, Andrew Lindesay <apl@lindesay.co.nz>.
 * All rights reserved. Distributed under the terms of the MIT License.
 */
#ifndef JWT_TOKEN_HELPER_H
#define JWT_TOKEN_HELPER_H


#include <String.h>


class BMessage;


/*! The term "JWT" is short for "Java Web Token" and is a token format typically used to convey
    proof of an authentication. The token is structured and contains three parts separated by the
    full-stop character ".". The first part is a header, the middle part contains some data (which
    is termed the "claims") and the last part is a signature proving the authenticity of the claims.

    The claims are base-64 encoded JSON and the JSON data typically conveys some key-value pairs
    with a number of the key names being well-known through standards.

    This class contains a number of helper methods for working with JWT tokens.
 */

 class JwtTokenHelper {
 public:
 	static	bool				IsValid(const BString& value);

 	static	status_t			ParseClaims(const BString& token,
 									BMessage& message);

 private:
 	static	bool				_IsBase64(char ch);
 };

 #endif // JWT_TOKEN_HELPER_H
