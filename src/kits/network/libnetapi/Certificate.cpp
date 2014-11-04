/*
 * Copyright 2014 Haiku, Inc.
 * Distributed under the terms of the MIT License.
 */


#include <Certificate.h>

#include <String.h>

#include "CertificatePrivate.h"


#ifdef OPENSSL_ENABLED


#include <openssl/x509v3.h>


static time_t
parse_ASN1(ASN1_GENERALIZEDTIME *asn1)
{
	// Get the raw string data out of the ASN1 container. It looks like this:
	// "YYMMDDHHMMSSZ"
	struct tm time;

	if (sscanf((char*)asn1->data, "%2d%2d%2d%2d%2d%2d", &time.tm_year,
			&time.tm_mon, &time.tm_mday, &time.tm_hour, &time.tm_min,
			&time.tm_sec) == 6) {

		// Month is 0 based, and year is 1900-based for mktime.
		time.tm_year += 100;
		time.tm_mon -= 1;

		return mktime(&time);
	}
	return B_BAD_DATA;
}


static BString
decode_X509_NAME(X509_NAME* name)
{
	char* buffer = X509_NAME_oneline(name, NULL, 0);

	BString result(buffer);
	OPENSSL_free(buffer);
	return result;
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


int
BCertificate::Version()
{
	return X509_get_version(fPrivate->fX509) + 1;
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


bool
BCertificate::IsValidAuthority()
{
	return X509_check_ca(fPrivate->fX509) > 0;
}


bool
BCertificate::IsSelfSigned()
{
	return X509_check_issued(fPrivate->fX509, fPrivate->fX509) == X509_V_OK;
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


BString
BCertificate::SignatureAlgorithm()
{
	int algorithmIdentifier = OBJ_obj2nid(
		fPrivate->fX509->cert_info->key->algor->algorithm);

	if (algorithmIdentifier == NID_undef)
		return BString("undefined");

	const char* buffer = OBJ_nid2ln(algorithmIdentifier);
	return BString(buffer);
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


bool
BCertificate::IsValidAuthority()
{
	return false;
}


int
BCertificate::Version()
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


BString
BCertificate::SignatureAlgorithm()
{
	return BString();
}


BString
BCertificate::String()
{
	return BString();
}


#endif
