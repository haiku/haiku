/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _CERTIFICATE_PRIVATE_H
#define _CERTIFICATE_PRIVATE_H


#ifdef OPENSSL_ENABLED
#	include <openssl/ssl.h>


class BCertificate::Private {
public:
	Private(X509* data);
	~Private();

public:
	X509* fX509;
};
#endif


#endif
