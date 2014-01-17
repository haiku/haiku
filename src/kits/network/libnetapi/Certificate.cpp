/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <Certificate.h>

#include <String.h>

#include "CertificatePrivate.h"


#ifdef OPENSSL_ENABLED


static time_t
parse_ASN1(ASN1_GENERALIZEDTIME *asn1)
{
	// Get the raw string data out of the ASN1 container. It looks like this:
	// "YYMMDDHHMMSSZ"
	struct tm time;

	if (sscanf((char*)asn1->data, "%2d%2d%2d%2d%2d%2d", &time.tm_year,
			&time.tm_mon, &time.tm_mday, &time.tm_hour, &time.tm_min,
			&time.tm_sec) == 6)
		return mktime(&time);

	return B_BAD_DATA;
}


static BString
decode_X509_NAME(X509_NAME* name)
{
	int len = X509_NAME_get_text_by_NID(name, 0, NULL, 0);
	char buffer[len];
	X509_NAME_get_text_by_NID(name, 0, buffer, len);

	return BString(buffer);
}


// #pragma mark - BCertificate


BCertificate::BCertificate(Private* data)
{
	fPrivate = data;
}


BCertificate::~BCertificate()
{
	delete fPrivate;
}


BString
BCertificate::String()
{
	BIO *buffer = BIO_new(BIO_s_mem());
	X509_print_ex(buffer, fPrivate->fX509, XN_FLAG_COMPAT, X509_FLAG_COMPAT);

	char* pointer;
	long length = BIO_get_mem_data(buffer, &pointer);
	BString result(pointer, length);

	BIO_free(buffer);
	return result;
}


time_t
BCertificate::StartDate()
{
	return parse_ASN1(X509_get_notBefore(fPrivate->fX509));
}


time_t
BCertificate::ExpirationDate()
{
	return parse_ASN1(X509_get_notAfter(fPrivate->fX509));
}


BString
BCertificate::Issuer()
{
	X509_NAME* name = X509_get_issuer_name(fPrivate->fX509);
	return decode_X509_NAME(name);
}


BString
BCertificate::Subject()
{
	X509_NAME* name = X509_get_subject_name(fPrivate->fX509);
	return decode_X509_NAME(name);
}


// #pragma mark - BCertificate::Private


BCertificate::Private::Private(X509* data)
	: fX509(data)
{
}

#else


BCertificate::BCertificate(Private* data)
{
}


BCertificate::~BCertificate()
{
}


BString
BCertificate::String()
{
	return BString();
}


time_t
BCertificate::StartDate()
{
	return B_NOT_SUPPORTED;
}


time_t
BCertificate::ExpirationDate()
{
	return B_NOT_SUPPORTED;
}


BString
BCertificate::Issuer()
{
	return BString();
}


BString
BCertificate::Subject()
{
	return BString();
}


#endif
