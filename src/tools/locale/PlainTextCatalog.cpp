/*
** Copyright 2009 Adrien Destugues, pulkomandy@gmail.com. All rights reserved.
** Distributed under the terms of the MIT License.
*/

#include <PlainTextCatalog.h>

#include <assert.h>
#include <cstdio>
#include <iostream>
#include <fstream>
#include <new>
#include <sstream>
#include <string>

#include <syslog.h>

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

#include <Catalog.h>


using BPrivate::PlainTextCatalog;
using std::min;
using std::max;
using std::pair;


/*
 *	This file implements the plain text catalog-type for the Haiku
 *	locale kit. It is not meant to be used in applications like other add ons,
 *	but only as a way to get an easy to translate file for developers.
 */


namespace BPrivate {


const char *PlainTextCatalog::kCatMimeType
	= "locale/x-vnd.Be.locale-catalog.plaintext";

static int16 kCatArchiveVersion = 1;
	// version of the catalog archive structure, bump this if you change it!


// Note: \xNN is not replaced back, so you get the unescaped value in the catkey
// file. This is fine for regular unicode chars (B_UTF8_ELLIPSIS) but may be
// dangerous if you use \x10 as a newline...
void
escapeQuotedChars(BString& stringToEscape)
{
	stringToEscape.ReplaceAll("\\", "\\\\");
	stringToEscape.ReplaceAll("\n", "\\n");
	stringToEscape.ReplaceAll("\t", "\\t");
	stringToEscape.ReplaceAll("\"", "\\\"");
}


/*
 * constructs a PlainTextCatalog with given signature and language and reads
 * the catalog from disk.
 * InitCheck() will be B_OK if catalog could be loaded successfully, it will
 * give an appropriate error-code otherwise.
 */
PlainTextCatalog::PlainTextCatalog(const entry_ref &signature,
	const char *language, uint32 fingerprint)
	:
	HashMapCatalog("", language, fingerprint)
{
	// Look for the catalog in the directory we are going to use
	// This constructor is not used so lets disable that...

	fInitCheck = B_NOT_SUPPORTED;
	fprintf(stderr,
		"trying to load plaintext-catalog(lang=%s) results in %s\n",
		language, strerror(fInitCheck));
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


status_t
PlainTextCatalog::ReadFromFile(const char *path)
{
	std::fstream catalogFile;
	std::string  currentItem;

	if (!path)
		path = fPath.String();

	catalogFile.open(path, std::fstream::in);
	if (!catalogFile.is_open()) {
		fprintf(stderr, "couldn't open catalog at %s\n", path);
		return B_ENTRY_NOT_FOUND;
	}

	// Now read all the data from the file

	// The first line holds some info about the catalog :
	// ArchiveVersion \t LanguageName \t AppSignature \t FingerPrint
	if (std::getline(catalogFile, currentItem, '\t').good()) {
		// Get the archive version
		int arcver= -1;
		std::istringstream ss(currentItem);
		ss >> arcver;
		if (ss.fail()) {
	 		// can't convert to int
			fprintf(stderr,
				"Unable to extract archive version ( string: %s ) from %s\n",
				currentItem.c_str(), path);
			return B_ERROR;
		}

		if (arcver != kCatArchiveVersion) {
			// wrong version
			fprintf(stderr,
				"Wrong archive version ! Got %d instead of %d from %s\n", arcver,
				kCatArchiveVersion, path);
			return B_ERROR;
		}
	} else {
		fprintf(stderr, "Unable to read from catalog %s\n", path);
		return B_ERROR;
	}

	if (std::getline(catalogFile, currentItem, '\t').good()) {
		// Get the language
		fLanguageName = currentItem.c_str() ;
	} else {
		fprintf(stderr, "Unable to get language from %s\n", path);
		return B_ERROR;
	}

	if (std::getline(catalogFile, currentItem, '\t').good()) {
		// Get the signature
		fSignature = currentItem.c_str() ;
	} else {
		fprintf(stderr, "Unable to get signature from %s\n", path);
		return B_ERROR;
	}

	if (std::getline(catalogFile, currentItem).good()) {
		// Get the fingerprint
		std::istringstream ss(currentItem);
		uint32 foundFingerprint;
		ss >> foundFingerprint;
		if (ss.fail()) {
			fprintf(stderr, "Unable to get fingerprint (%s) from %s\n",
					currentItem.c_str(), path);
			return B_ERROR;
		}

		if (fFingerprint == 0)
			fFingerprint = foundFingerprint;

		if (fFingerprint != foundFingerprint) {
			return B_MISMATCHED_VALUES;
		}
	} else {
		fprintf(stderr, "Unable to get fingerprint from %s\n", path);
		return B_ERROR;
	}

	// We managed to open the file, so we remember it's the one we are using
	fPath = path;
	// fprintf(stderr, "LocaleKit Plaintext: found catalog at %s\n", path);

	std::string originalString;
	std::string context;
	std::string comment;
	std::string translated;

	while (std::getline(catalogFile, originalString,'\t').good()) {
		// Each line is : "original string \t context \t comment \t translation"

		if (!std::getline(catalogFile, context,'\t').good()) {
			fprintf(stderr, "Unable to get context for string %s from %s\n",
				originalString.c_str(), path);
			return B_ERROR;
		}

		if (!std::getline(catalogFile, comment,'\t').good()) {
			fprintf(stderr, "Unable to get comment for string %s from %s\n",
				originalString.c_str(), path);
			return B_ERROR;
		}

		if (!std::getline(catalogFile, translated).good()) {
			fprintf(stderr,
				"Unable to get translated text for string %s from %s\n",
				originalString.c_str(), path);
			return B_ERROR;
		}

		// We could do that :
		// SetString(key, translated.c_str());
		// but we want to keep the strings in the new catkey. Hash collisions
		// happen, you know. (and CatKey::== will fail)
		SetString(originalString.c_str(), translated.c_str(), context.c_str(),
			comment.c_str());
	}

	catalogFile.close();

#if 0
	uint32 checkFP = ComputeFingerprint();
	if (fFingerprint != checkFP) {
		fprintf(stderr, "plaintext-catalog(sig=%s, lang=%s) "
			"has wrong fingerprint after load (%u instead of %u). "
			"The catalog data may be corrupted, so this catalog is "
			"skipped.\n",
			fSignature.String(), fLanguageName.String(), checkFP,
			fFingerprint);
		return B_BAD_DATA;
	}
#endif

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
	// useless on the build-tool-side
}


void
PlainTextCatalog::UpdateAttributes(const char* path)
{
	BEntry entry(path);
	BFile node(&entry, B_READ_WRITE);
	UpdateAttributes(node);
}


BCatalogData *
PlainTextCatalog::Instantiate(const entry_ref &owner, const char *language,
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


} // namespace BPrivate


extern "C" BCatalogData *
instantiate_catalog(const entry_ref &owner, const char *language,
	uint32 fingerprint)
{
	return PlainTextCatalog::Instantiate(owner, language, fingerprint);
}


extern "C" BCatalogData *
create_catalog(const char *signature, const char *language)
{
	PlainTextCatalog *catalog
		= new(std::nothrow) PlainTextCatalog("emptycat", signature, language);
	return catalog;
}


uint8 gCatalogAddOnPriority = 99;
	// give low priority to this add on, we don't want it to be actually used
