/*
 * Copyright 2014, Dancsó Róbert <dancso.robert@d-rendszer.hu>
 *
 * Distributed under terms of the MIT license.
 */


#include "EncryptionUtils.h"

#include <Catalog.h>
#include <File.h>
#include <Locale.h>
#include <String.h>


#define B_TRANSLATION_CONTEXT "Encryption utils"


const char*
EncryptionType(const char* path)
{
	char buffer[11];
	BString encrypter;
	off_t length = BFile(path, B_READ_ONLY).Read(&buffer, 11);
	if (length != 11)
		return NULL;
	encrypter.Append(buffer, 11);

	if (encrypter.FindFirst("-FVE-FS-") >= 0) {
		return B_TRANSLATE("BitLocker encrypted");
	} else if (encrypter.FindFirst("PGPGUARD") >= 0) {
		return B_TRANSLATE("PGP encrypted");
	} else if (encrypter.FindFirst("SafeBoot") >= 0) {
		return B_TRANSLATE("SafeBoot encrypted");
	} else if (encrypter.FindFirst("LUKS") >= 0) {
		return B_TRANSLATE("LUKS encrypted");
	}

	return NULL;
}


