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


BCertificate::BCertificate(const BCertificate& other)
{
	fPrivate = new(std::nothrow) BCertificate::Private(other.fPrivate->fX509);
}


BCertificate::~BCertificate()
{
	delete fPrivate;
}


int
BCertificate::Version() const
{
	return X509_get_version(fPrivate->fX509) + 1;
}


time_t
BCertificate::StartDate() const
{
	return parse_ASN1(X509_get_notBefore(fPrivate->fX509));
}


time_t
BCertificate::ExpirationDate() const
{
	return parse_ASN1(X509_get_notAfter(fPrivate->fX509));
}


bool
BCertificate::IsValidAuthority() const
{
	return X509_check_ca(fPrivate->fX509) > 0;
}


bool
BCertificate::IsSelfSigned() const
{
	return X509_check_issued(fPrivate->fX509, fPrivate->fX509) == X509_V_OK;
}


BString
BCertificate::Issuer() const
{
	X509_NAME* name = X509_get_issuer_name(fPrivate->fX509);
	return decode_X509_NAME(name);
}


BString
BCertificate::Subject() const
{
	X509_NAME* name = X509_get_subject_name(fPrivate->fX509);
	return decode_X509_NAME(name);
}


BString
BCertificate::SignatureAlgorithm() const
{
	int algorithmIdentifier;
	if (!X509_get_signature_info(fPrivate->fX509, NULL, &algorithmIdentifier,
			NULL, NULL)) {
		return BString("invalid");
	}

	if (algorithmIdentifier == NID_undef)
		return BString("undefined");

	const char* buffer = OBJ_nid2ln(algorithmIdentifier);
	return BString(buffer);
}


BString
BCertificate::String() const
{
	BIO *buffer = BIO_new(BIO_s_mem());
	X509_print_ex(buffer, fPrivate->fX509, XN_FLAG_COMPAT, X509_FLAG_COMPAT);

	char* pointer;
	long length = BIO_get_mem_data(buffer, &pointer);
	BString result(pointer, length);

	BIO_free(buffer);
	return result;
}


bool
BCertificate::operator==(const BCertificate& other) const
{
	return X509_cmp(fPrivate->fX509, other.fPrivate->fX509) == 0;
}


// #pragma mark - BCertificate::Private


BCertificate::Private::Private(X509* data)
	: fX509(X509_dup(data))
{
}


BCertificate::Private::~Private()
{
	X509_free(fX509);
}


#else


BCertificate::BCertificate(const BCertificate& other)
{
}


BCertificate::BCertificate(Private* data)
{
}


BCertificate::~BCertificate()
{
}


time_t
BCertificate::StartDate() const
{
	return B_NOT_SUPPORTED;
}


time_t
BCertificate::ExpirationDate() const
{
	return B_NOT_SUPPORTED;
}


bool
BCertificate::IsValidAuthority() const
{
	return false;
}


int
BCertificate::Version() const
{
	return B_NOT_SUPPORTED;
}


BString
BCertificate::Issuer() const
{
	return BString();
}


BString
BCertificate::Subject() const
{
	return BString();
}


BString
BCertificate::SignatureAlgorithm() const
{
	return BString();
}


BString
BCertificate::String() const
{
	return BString();
}


bool
BCertificate::operator==(const BCertificate& other) const
{
	return false;
}

#endif
