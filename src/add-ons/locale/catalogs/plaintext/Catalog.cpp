/*
 * Copyright 2009 Adrien Destugues, pulkomandy@gmail.com. All rights reserved.
 * Distributed under the terms of the MIT License.
 */

#include <PlainTextCatalog.h>

#include <assert.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <new>
#include <sstream>
#include <string>

#include <AppFileInfo.h>
#include <Application.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <Message.h>
#include <Mime.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>
#include <String.h>

#include <LocaleRoster.h>
#include <Catalog.h>


using BPrivate::HashMapCatalog;
using BPrivate::PlainTextCatalog;
using std::min;
using std::max;
using std::pair;


/*
 *	This file implements the plain text catalog-type for the Haiku
 *	locale kit. It is not meant to be used in applications like other add ons,
 *	but only as a way to get an easy to translate file for developers.
 */


extern "C" uint32 adler32(uint32 adler, const uint8 *buf, uint32 len);
	// definition lives in adler32.c

static const char *kCatFolder = "catalogs";
static const char *kCatExtension = ".catkeys";

const char *PlainTextCatalog::kCatMimeType
	= "locale/x-vnd.Be.locale-catalog.plaintext";

static int16 kCatArchiveVersion = 1;
	// version of the catalog archive structure, bump this if you change it!


void
escapeQuotedChars(BString& stringToEscape)
{
	stringToEscape.ReplaceAll("\\","\\\\");
	stringToEscape.ReplaceAll("\n","\\n");
	stringToEscape.ReplaceAll("\t","\\t");
	stringToEscape.ReplaceAll("\"","\\\"");
}


/*
 * constructs a PlainTextCatalog with given signature and language and reads
 * the catalog from disk.
 * InitCheck() will be B_OK if catalog could be loaded successfully, it will
 * give an appropriate error-code otherwise.
 */
PlainTextCatalog::PlainTextCatalog(const entry_ref &owner, const char *language,
	uint32 fingerprint)
	:
	HashMapCatalog("", language, fingerprint)
{
	// We created the catalog with an invalid signature, but we fix that now.
	SetSignature(owner);

	// give highest priority to catalog living in sub-folder of app's folder:
	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	node_ref nref;
	nref.device = appInfo.ref.device;
	nref.node = appInfo.ref.directory;
	BDirectory appDir(&nref);
	BString catalogName("locale/");
	catalogName << kCatFolder
		<< "/" << fSignature
		<< "/" << fLanguageName
		<< kCatExtension;
	BPath catalogPath(&appDir, catalogName.String());
	status_t status = ReadFromFile(catalogPath.Path());

	if (status != B_OK) {
		// look in common-etc folder (/boot/home/config/etc):
		BPath commonEtcPath;
		find_directory(B_SYSTEM_ETC_DIRECTORY, &commonEtcPath);
		if (commonEtcPath.InitCheck() == B_OK) {
			catalogName = BString(commonEtcPath.Path())
							<< "/locale/" << kCatFolder
							<< "/" << fSignature
							<< "/" << fLanguageName
							<< kCatExtension;
			status = ReadFromFile(catalogName.String());
		}
	}

	if (status != B_OK) {
		// look in system-etc folder (/boot/beos/etc):
		BPath systemEtcPath;
		find_directory(B_BEOS_ETC_DIRECTORY, &systemEtcPath);
		if (systemEtcPath.InitCheck() == B_OK) {
			catalogName = BString(systemEtcPath.Path())
							<< "/locale/" << kCatFolder
							<< "/" << fSignature
							<< "/" << fLanguageName
							<< kCatExtension;
			status = ReadFromFile(catalogName.String());
		}
	}

	fInitCheck = status;
}


/*
 * constructs an empty PlainTextCatalog with given sig and language.
 * This is used for editing/testing purposes.
 * InitCheck() will always be B_OK.
 */
PlainTextCatalog::PlainTextCatalog(const char *path, const char *signature,
	const char *language)
	:
	HashMapCatalog(signature, language, 0),
	fPath(path)
{
	fInitCheck = B_OK;
}


PlainTextCatalog::~PlainTextCatalog()
{
}


void
PlainTextCatalog::SetSignature(const entry_ref &catalogOwner)
{
	// figure out mimetype from image
	BFile objectFile(&catalogOwner, B_READ_ONLY);
	BAppFileInfo objectInfo(&objectFile);
	char objectSignature[B_MIME_TYPE_LENGTH];
	if (objectInfo.GetSignature(objectSignature) != B_OK) {
		fSignature = "";
		return;
	}

	// drop supertype from mimetype (should be "application/"):
	char* stripSignature = objectSignature;
	while (*stripSignature != '/' && *stripSignature != '\0')
		stripSignature ++;

	if (*stripSignature == '\0')
		stripSignature = objectSignature;
	else
		stripSignature ++;

	fSignature = stripSignature;
}


status_t
PlainTextCatalog::ReadFromFile(const char *path)
{
	std::fstream catalogFile;
	std::string currentItem;

	if (!path)
		path = fPath.String();

	catalogFile.open(path, std::fstream::in);
	if (!catalogFile.is_open())
		return B_ENTRY_NOT_FOUND;

	// Now read all the data from the file

	// The first line holds some info about the catalog :
	// ArchiveVersion \t LanguageName \t Signature \t FingerPrint
	if (std::getline(catalogFile, currentItem, '\t').good()) {
		// Get the archive version
		int arcver= -1;
		std::istringstream ss(currentItem);
		ss >> arcver;
		if (ss.fail()) {
			// can't convert to int
			return B_ERROR;
		}

		if (arcver != kCatArchiveVersion) {
			// wrong version
			return B_ERROR;
		}
	} else
		return B_ERROR;

	if (std::getline(catalogFile, currentItem, '\t').good()) {
		// Get the language
		fLanguageName = currentItem.c_str() ;
	} else
		return B_ERROR;

	if (std::getline(catalogFile, currentItem, '\t').good()) {
		// Get the signature
		fSignature = currentItem.c_str() ;
	} else
		return B_ERROR;

	if (std::getline(catalogFile, currentItem).good()) {
		// Get the fingerprint
		std::istringstream ss(currentItem);
		uint32 foundFingerprint;
		ss >> foundFingerprint;
		if (ss.fail())
			return B_ERROR;

		if (fFingerprint == 0)
			fFingerprint = foundFingerprint;

		if (fFingerprint != foundFingerprint) {
			return B_MISMATCHED_VALUES;
		}
	} else
		return B_ERROR;

	// We managed to open the file, so we remember it's the one we are using
	fPath = path;

	std::string originalString;
	std::string context;
	std::string comment;
	std::string translated;

	while (std::getline(catalogFile, originalString,'\t').good()) {
		// Each line is : "original string \t context \t comment \t translation"

		if (!std::getline(catalogFile, context,'\t').good())
			return B_ERROR;

		if (!std::getline(catalogFile, comment,'\t').good())
			return B_ERROR;

		if (!std::getline(catalogFile, translated).good())
			return B_ERROR;

		// We could do that :
		// SetString(key, translated.c_str());
		// but we want to keep the strings in the new catkey. Hash collisions
		// happen, you know. (and CatKey::== will fail)
		SetString(originalString.c_str(), translated.c_str(), context.c_str(),
			comment.c_str());
	}

	catalogFile.close();

	uint32 checkFP = ComputeFingerprint();
	if (fFingerprint != checkFP)
		return B_BAD_DATA;

	// some information living in member variables needs to be copied
	// to attributes. Although these attributes should have been written
	// when creating the catalog, we make sure that they exist there:
	UpdateAttributes(path);
	return B_OK;
}


status_t
PlainTextCatalog::WriteToFile(const char *path)
{
	BString textContent;
	BFile catalogFile;

	if (path)
		fPath = path;
	status_t res = catalogFile.SetTo(fPath.String(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (res != B_OK)
		return res;

	UpdateFingerprint();
		// make sure we have the correct fingerprint before we flatten it

	textContent << kCatArchiveVersion << "\t" << fLanguageName.String() << "\t"
		<< fSignature.String() << "\t" << fFingerprint << "\n";

	res = catalogFile.Write(textContent.String(),textContent.Length());
	if (res != textContent.Length())
		return res;

	CatMap::Iterator iter = fCatMap.GetIterator();
	CatMap::Entry entry;
	BString original;
	BString comment;
	BString translated;

	while (iter.HasNext()) {
		entry = iter.Next();
		original = entry.key.fString;
		comment = entry.key.fComment;
		translated = entry.value;

		escapeQuotedChars(original);
		escapeQuotedChars(comment);
		escapeQuotedChars(translated);

		textContent.Truncate(0);
		textContent << original.String() << "\t"
			<< entry.key.fContext.String() << "\t"
			<< comment << "\t"
			<< translated.String() << "\n";
		res = catalogFile.Write(textContent.String(),textContent.Length());
		if (res != textContent.Length())
			return res;
	}

	// set mimetype-, language- and signature-attributes:
	UpdateAttributes(catalogFile);

	return B_OK;
}


/*
 * writes mimetype, language-name and signature of catalog into the
 * catalog-file.
 */
void
PlainTextCatalog::UpdateAttributes(BFile& catalogFile)
{
	static const int bufSize = 256;
	char buf[bufSize];
	uint32 temp;
	if (catalogFile.ReadAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0, &buf, bufSize)
			<= 0
		|| strcmp(kCatMimeType, buf) != 0) {
		catalogFile.WriteAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0,
			kCatMimeType, strlen(kCatMimeType)+1);
	}
	if (catalogFile.ReadAttr(BLocaleRoster::kCatLangAttr, B_STRING_TYPE, 0,
			&buf, bufSize) <= 0 || fLanguageName != buf) {
		catalogFile.WriteAttrString(BLocaleRoster::kCatLangAttr, &fLanguageName);
	}
	if (catalogFile.ReadAttr(BLocaleRoster::kCatSigAttr, B_STRING_TYPE, 0,
			&buf, bufSize) <= 0 || fSignature != buf) {
		catalogFile.WriteAttrString(BLocaleRoster::kCatSigAttr, &fSignature);
	}
	if (catalogFile.ReadAttr(BLocaleRoster::kCatFingerprintAttr, B_UINT32_TYPE,
		0, &temp, sizeof(uint32)) <= 0) {
		catalogFile.WriteAttr(BLocaleRoster::kCatFingerprintAttr, B_UINT32_TYPE,
			0, &fFingerprint, sizeof(uint32));
	}
}


void
PlainTextCatalog::UpdateAttributes(const char* path)
{
	BEntry entry(path);
	BFile node(&entry, B_READ_WRITE);
	UpdateAttributes(node);
}


BCatalogData *
PlainTextCatalog::Instantiate(const entry_ref& owner, const char *language,
	uint32 fingerprint)
{
	PlainTextCatalog *catalog
		= new(std::nothrow) PlainTextCatalog(owner, language, fingerprint);
	if (catalog && catalog->InitCheck() != B_OK) {
		delete catalog;
		return NULL;
	}
	return catalog;
}


extern "C" BCatalogData *
instantiate_catalog(const entry_ref& owner, const char *language,
	uint32 fingerprint)
{
	PlainTextCatalog *catalog
		= new(std::nothrow) PlainTextCatalog(owner, language, fingerprint);
	if (catalog && catalog->InitCheck() != B_OK) {
		delete catalog;
		return NULL;
	}
	return catalog;
}


extern "C" BCatalogData *
create_catalog(const char *signature, const char *language)
{
	PlainTextCatalog *catalog
		= new(std::nothrow) PlainTextCatalog("emptycat", signature, language);
	return catalog;
}


extern "C" status_t
get_available_languages(BMessage* availableLanguages,
	const char* sigPattern = NULL, const char* langPattern = NULL,
	int32 fingerprint = 0)
{
	// TODO
	return B_ERROR;
}


uint8 gCatalogAddOnPriority = 99;
	// give low priority to this add on, we don't want it to be actually used
