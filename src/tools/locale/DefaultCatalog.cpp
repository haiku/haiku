/*
 * Copyright 2003-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe, zooey@hirschkaefer.de
 *		Adrien Destugues, pulkomandy@gmail.com
 */


#include <memory>
#include <new>
#include <syslog.h>

#include <AppFileInfo.h>
#include <Application.h>
#include <DataIO.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <Message.h>
#include <Mime.h>
#include <Path.h>
#include <Resources.h>
#include <Roster.h>

#include <DefaultCatalog.h>
#include <LocaleRoster.h>

#include <cstdio>


using BPrivate::DefaultCatalog;
using std::auto_ptr;


/*!	This file implements the default catalog-type for the opentracker locale
	kit. Alternatively, this could be used as a full add-on, but currently this
	is provided as part of liblocale.so.
*/


namespace BPrivate {


// several attributes/resource-IDs used within the Locale Kit:

const char *kCatLangAttr = "BEOS:LOCALE_LANGUAGE";
	// name of catalog language, lives in every catalog file
const char *kCatSigAttr = "BEOS:LOCALE_SIGNATURE";
	// catalog signature, lives in every catalog file
const char *kCatFingerprintAttr = "BEOS:LOCALE_FINGERPRINT";
	// catalog fingerprint, may live in catalog file

const char *DefaultCatalog::kCatMimeType
	= "locale/x-vnd.Be.locale-catalog.default";

static int16 kCatArchiveVersion = 1;
	// version of the catalog archive structure, bump this if you change it!


/*!	Constructs a DefaultCatalog with given signature and language and reads
	the catalog from disk.
	InitCheck() will be B_OK if catalog could be loaded successfully, it will
	give an appropriate error-code otherwise.
*/
DefaultCatalog::DefaultCatalog(const entry_ref &catalogOwner,
	const char *language, uint32 fingerprint)
	:
	HashMapCatalog("", language, fingerprint)
{
	fInitCheck = B_NOT_SUPPORTED;
	fprintf(stderr,
		"trying to load default-catalog(sig=%s, lang=%s) results in %s",
		"", language, strerror(fInitCheck));
}


/*!	Constructs a DefaultCatalog and reads it from the resources of the
	given entry-ref (which usually is an app- or add-on-file).
	InitCheck() will be B_OK if catalog could be loaded successfully, it will
	give an appropriate error-code otherwise.
*/
DefaultCatalog::DefaultCatalog(entry_ref *appOrAddOnRef)
	:
	HashMapCatalog("", "", 0)
{
	fInitCheck = ReadFromResource(*appOrAddOnRef);
	// fprintf(stderr,
	//	"trying to load embedded catalog from resources results in %s",
	//	strerror(fInitCheck));
}


/*!	Constructs an empty DefaultCatalog with given sig and language.
	This is used for editing/testing purposes.
	InitCheck() will always be B_OK.
*/
DefaultCatalog::DefaultCatalog(const char *path, const char *signature,
	const char *language)
	:
	HashMapCatalog(signature, language, 0),
	fPath(path)
{
	fInitCheck = B_OK;
}


DefaultCatalog::~DefaultCatalog()
{
}


void
DefaultCatalog::SetSignature(const entry_ref &catalogOwner)
{
	// Not allowed for the build-tool version.
	return;
}


status_t
DefaultCatalog::SetRawString(const CatKey& key, const char *translated)
{
	return fCatMap.Put(key, translated);
}


status_t
DefaultCatalog::ReadFromFile(const char *path)
{
	if (!path)
		path = fPath.String();

	BFile catalogFile;
	status_t res = catalogFile.SetTo(path, B_READ_ONLY);
	if (res != B_OK) {
		fprintf(stderr, "no catalog at %s\n", path);
		return B_ENTRY_NOT_FOUND;
	}

	fPath = path;
	fprintf(stderr, "found catalog at %s\n", path);

	off_t sz = 0;
	res = catalogFile.GetSize(&sz);
	if (res != B_OK) {
		fprintf(stderr, "couldn't get size for catalog-file %s\n", path);
		return res;
	}

	auto_ptr<char> buf(new(std::nothrow) char [sz]);
	if (buf.get() == NULL) {
		fprintf(stderr, "couldn't allocate array of %Ld chars\n", sz);
		return B_NO_MEMORY;
	}
	res = catalogFile.Read(buf.get(), sz);
	if (res < B_OK) {
		fprintf(stderr, "couldn't read from catalog-file %s\n", path);
		return res;
	}
	if (res < sz) {
		fprintf(stderr,
			"only got %u instead of %Lu bytes from catalog-file %s\n", res, sz,
			path);
		return res;
	}
	BMemoryIO memIO(buf.get(), sz);
	res = Unflatten(&memIO);

	if (res == B_OK) {
		// some information living in member variables needs to be copied
		// to attributes. Although these attributes should have been written
		// when creating the catalog, we make sure that they exist there:
		UpdateAttributes(catalogFile);
	}

	return res;
}


/*!	This method is not currently being used, but it may be useful in the
	future...
*/
status_t
DefaultCatalog::ReadFromAttribute(const entry_ref &appOrAddOnRef)
{
	return B_NOT_SUPPORTED;
}


status_t
DefaultCatalog::ReadFromResource(const entry_ref &appOrAddOnRef)
{
	return B_NOT_SUPPORTED;
}


status_t
DefaultCatalog::WriteToFile(const char *path)
{
	BFile catalogFile;
	if (path)
		fPath = path;
	status_t status = catalogFile.SetTo(fPath.String(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (status != B_OK)
		return status;

	BMallocIO mallocIO;
	mallocIO.SetBlockSize(max_c(fCatMap.Size() * 20, 256));
		// set a largish block-size in order to avoid reallocs
	status = Flatten(&mallocIO);
	if (status != B_OK)
		return status;

	ssize_t bytesWritten
		= catalogFile.Write(mallocIO.Buffer(), mallocIO.BufferLength());
	if (bytesWritten < 0)
		return bytesWritten;
	if (bytesWritten != (ssize_t)mallocIO.BufferLength())
		return B_IO_ERROR;

	// set mimetype-, language- and signature-attributes:
	UpdateAttributes(catalogFile);

	return B_OK;
}


/*!	This method is not currently being used, but it may be useful in the
	future...
*/
status_t
DefaultCatalog::WriteToAttribute(const entry_ref &appOrAddOnRef)
{
	return B_NOT_SUPPORTED;
}


status_t
DefaultCatalog::WriteToResource(const entry_ref &appOrAddOnRef)
{
	return B_NOT_SUPPORTED;
}


/*!	Writes mimetype, language-name and signature of catalog into the
	catalog-file.
*/
void
DefaultCatalog::UpdateAttributes(BFile& catalogFile)
{
	static const int bufSize = 256;
	char buf[bufSize];
	uint32 temp;
	if (catalogFile.ReadAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0, &buf,
			bufSize) <= 0
		|| strcmp(kCatMimeType, buf) != 0) {
		catalogFile.WriteAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0,
			kCatMimeType, strlen(kCatMimeType)+1);
	}
	if (catalogFile.ReadAttr(kCatLangAttr, B_STRING_TYPE, 0,
			&buf, bufSize) <= 0
		|| fLanguageName != buf) {
		catalogFile.WriteAttr(kCatLangAttr, B_STRING_TYPE, 0,
			fLanguageName.String(), fLanguageName.Length()+1);
	}
	if (catalogFile.ReadAttr(kCatSigAttr, B_STRING_TYPE, 0,
			&buf, bufSize) <= 0
		|| fSignature != buf) {
		catalogFile.WriteAttr(kCatSigAttr, B_STRING_TYPE, 0,
			fSignature.String(), fSignature.Length()+1);
	}
	if (catalogFile.ReadAttr(kCatFingerprintAttr, B_UINT32_TYPE,
		0, &temp, sizeof(uint32)) <= 0) {
		catalogFile.WriteAttr(kCatFingerprintAttr, B_UINT32_TYPE,
			0, &fFingerprint, sizeof(uint32));
	}
}


status_t
DefaultCatalog::Flatten(BDataIO *dataIO)
{
	UpdateFingerprint();
		// make sure we have the correct fingerprint before we flatten it

	status_t res;
	BMessage archive;
	uint32 count = fCatMap.Size();
	res = archive.AddString("class", "DefaultCatalog");
	if (res == B_OK)
		res = archive.AddInt32("c:sz", count);
	if (res == B_OK)
		res = archive.AddInt16("c:ver", kCatArchiveVersion);
	if (res == B_OK)
		res = archive.AddString("c:lang", fLanguageName.String());
	if (res == B_OK)
		res = archive.AddString("c:sig", fSignature.String());
	if (res == B_OK)
		res = archive.AddInt32("c:fpr", fFingerprint);
	if (res == B_OK)
		res = archive.Flatten(dataIO);

	CatMap::Iterator iter = fCatMap.GetIterator();
	CatMap::Entry entry;
	while (res == B_OK && iter.HasNext()) {
		entry = iter.Next();
		archive.MakeEmpty();
		res = archive.AddString("c:ostr", entry.key.fString.String());
		if (res == B_OK)
			res = archive.AddString("c:ctxt", entry.key.fContext.String());
		if (res == B_OK)
			res = archive.AddString("c:comt", entry.key.fComment.String());
		if (res == B_OK)
			res = archive.AddInt32("c:hash", entry.key.fHashVal);
		if (res == B_OK)
			res = archive.AddString("c:tstr", entry.value.String());
		if (res == B_OK)
			res = archive.Flatten(dataIO);
	}
	return res;
}


status_t
DefaultCatalog::Unflatten(BDataIO *dataIO)
{
	fCatMap.Clear();
	int32 count = 0;
	int16 version;
	BMessage archiveMsg;
	status_t res = archiveMsg.Unflatten(dataIO);

	if (res == B_OK) {
		res = archiveMsg.FindInt16("c:ver", &version)
			|| archiveMsg.FindInt32("c:sz", &count);
	}
	if (res == B_OK) {
		fLanguageName = archiveMsg.FindString("c:lang");
		fSignature = archiveMsg.FindString("c:sig");
		uint32 foundFingerprint = archiveMsg.FindInt32("c:fpr");

		// if a specific fingerprint has been requested and the catalog does in
		// fact have a fingerprint, both are compared. If they mismatch, we do
		// not accept this catalog:
		if (foundFingerprint != 0 && fFingerprint != 0
			&& foundFingerprint != fFingerprint) {
			fprintf(stderr, "default-catalog(sig=%s, lang=%s) "
				"has mismatching fingerprint (%d instead of the requested %d)"
				", so this catalog is skipped.\n",
				fSignature.String(), fLanguageName.String(), foundFingerprint,
				fFingerprint);
			res = B_MISMATCHED_VALUES;
		} else
			fFingerprint = foundFingerprint;
	}

	if (res == B_OK && count > 0) {
		CatKey key;
		const char *keyStr;
		const char *keyCtx;
		const char *keyCmt;
		const char *translated;

		// fCatMap.resize(count);
			// There is no resize method in Haiku Hash Map to prealloc space
		for (int i=0; res == B_OK && i < count; ++i) {
			res = archiveMsg.Unflatten(dataIO);
			if (res == B_OK)
				res = archiveMsg.FindString("c:ostr", &keyStr);
			if (res == B_OK)
				res = archiveMsg.FindString("c:ctxt", &keyCtx);
			if (res == B_OK)
				res = archiveMsg.FindString("c:comt", &keyCmt);
			if (res == B_OK)
				res = archiveMsg.FindInt32("c:hash", (int32*)&key.fHashVal);
			if (res == B_OK)
				res = archiveMsg.FindString("c:tstr", &translated);
			if (res == B_OK) {
				key.fString = keyStr;
				key.fContext = keyCtx;
				key.fComment = keyCmt;
				fCatMap.Put(key, translated);
			}
		}
		uint32 checkFP = ComputeFingerprint();
		if (fFingerprint != checkFP) {
			fprintf(stderr, "default-catalog(sig=%s, lang=%s) "
				"has wrong fingerprint after load (%d instead of %d). "
				"The catalog data may be corrupted, so this catalog is "
				"skipped.\n",
				fSignature.String(), fLanguageName.String(), checkFP,
				fFingerprint);
			return B_BAD_DATA;
		}
	}
	return res;
}


BCatalogData *
DefaultCatalog::Instantiate(const entry_ref &catalogOwner, const char *language,
	uint32 fingerprint)
{
	DefaultCatalog *catalog
		= new(std::nothrow) DefaultCatalog(catalogOwner, language, fingerprint);
	if (catalog && catalog->InitCheck() != B_OK) {
		delete catalog;
		return NULL;
	}
	return catalog;
}


BCatalogData *
DefaultCatalog::Create(const char *signature, const char *language)
{
	DefaultCatalog *catalog
		= new(std::nothrow) DefaultCatalog("", signature, language);
	if (catalog && catalog->InitCheck() != B_OK) {
		delete catalog;
		return NULL;
	}
	return catalog;
}


const uint8 DefaultCatalog::kDefaultCatalogAddOnPriority = 1;
	// give highest priority to our embedded catalog-add-on


} // namespace BPrivate
