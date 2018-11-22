/*
 * Copyright 2003-2009, Haiku.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Oliver Tappe, zooey@hirschkaefer.de
 *		Adrien Destugues, pulkomandy@gmail.com
 */


#include <algorithm>
#include <new>

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
#include <StackOrHeapArray.h>

#include <DefaultCatalog.h>
#include <MutableLocaleRoster.h>


#include <cstdio>


using std::min;
using std::max;
using std::pair;


/*!	This file implements the default catalog-type for the opentracker locale
	kit. Alternatively, this could be used as a full add-on, but currently this
	is provided as part of liblocale.so.
*/


static const char *kCatFolder = "catalogs";
static const char *kCatExtension = ".catalog";


namespace BPrivate {


const char *DefaultCatalog::kCatMimeType
	= "locale/x-vnd.Be.locale-catalog.default";

static int16 kCatArchiveVersion = 1;
	// version of the catalog archive structure, bump this if you change it!

const uint8 DefaultCatalog::kDefaultCatalogAddOnPriority = 1;
	// give highest priority to our embedded catalog-add-on


/*!	Constructs a DefaultCatalog with given signature and language and reads
	the catalog from disk.
	InitCheck() will be B_OK if catalog could be loaded successfully, it will
	give an appropriate error-code otherwise.
*/
DefaultCatalog::DefaultCatalog(const entry_ref &catalogOwner, const char *language,
	uint32 fingerprint)
	:
	HashMapCatalog("", language, fingerprint)
{
	// We created the catalog with an invalid signature, but we fix that now.
	SetSignature(catalogOwner);
	status_t status;

	// search for catalog living in sub-folder of app's folder:
	node_ref nref;
	nref.device = catalogOwner.device;
	nref.node = catalogOwner.directory;
	BDirectory appDir(&nref);
	BString catalogName("locale/");
	catalogName << kCatFolder
		<< "/" << fSignature
		<< "/" << fLanguageName
		<< kCatExtension;
	BPath catalogPath(&appDir, catalogName.String());
	status = ReadFromFile(catalogPath.Path());

	if (status != B_OK) {
		// search in data folders

		directory_which which[] = {
			B_USER_NONPACKAGED_DATA_DIRECTORY,
			B_USER_DATA_DIRECTORY,
			B_SYSTEM_NONPACKAGED_DATA_DIRECTORY,
			B_SYSTEM_DATA_DIRECTORY
		};

		for (size_t i = 0; i < sizeof(which) / sizeof(which[0]); i++) {
			BPath path;
			if (find_directory(which[i], &path) == B_OK) {
				BString catalogName(path.Path());
				catalogName << "/locale/" << kCatFolder
					<< "/" << fSignature
					<< "/" << fLanguageName
					<< kCatExtension;
				status = ReadFromFile(catalogName.String());
				if (status == B_OK)
					break;
			}
		}
	}

	if (status != B_OK) {
		// give lowest priority to catalog embedded as resource in application
		// executable, so they can be overridden easily.
		status = ReadFromResource(catalogOwner);
	}

	fInitCheck = status;
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
	if (res != B_OK)
		return B_ENTRY_NOT_FOUND;

	fPath = path;

	off_t sz = 0;
	res = catalogFile.GetSize(&sz);
	if (res != B_OK) {
		return res;
	}

	BStackOrHeapArray<char, 0> buf(sz);
	if (!buf.IsValid())
		return B_NO_MEMORY;
	res = catalogFile.Read(buf, sz);
	if (res < B_OK)
		return res;
	if (res < sz)
		return res;
	BMemoryIO memIO(buf, sz);
	res = Unflatten(&memIO);

	if (res == B_OK) {
		// some information living in member variables needs to be copied
		// to attributes. Although these attributes should have been written
		// when creating the catalog, we make sure that they exist there:
		UpdateAttributes(catalogFile);
	}

	return res;
}


/*
 * this method is not currently being used, but it may be useful in the future...
 */
status_t
DefaultCatalog::ReadFromAttribute(const entry_ref &appOrAddOnRef)
{
	BNode node;
	status_t res = node.SetTo(&appOrAddOnRef);
	if (res != B_OK)
		return B_ENTRY_NOT_FOUND;

	attr_info attrInfo;
	res = node.GetAttrInfo(BLocaleRoster::kEmbeddedCatAttr, &attrInfo);
	if (res != B_OK)
		return B_NAME_NOT_FOUND;
	if (attrInfo.type != B_MESSAGE_TYPE)
		return B_BAD_TYPE;

	size_t size = attrInfo.size;
	BStackOrHeapArray<char, 0> buf(size);
	if (!buf.IsValid())
		return B_NO_MEMORY;
	res = node.ReadAttr(BLocaleRoster::kEmbeddedCatAttr, B_MESSAGE_TYPE, 0,
		buf, size);
	if (res < (ssize_t)size)
		return res < B_OK ? res : B_BAD_DATA;

	BMemoryIO memIO(buf, size);
	res = Unflatten(&memIO);

	return res;
}


status_t
DefaultCatalog::ReadFromResource(const entry_ref &appOrAddOnRef)
{
	BFile file;
	status_t res = file.SetTo(&appOrAddOnRef, B_READ_ONLY);
	if (res != B_OK)
		return B_ENTRY_NOT_FOUND;

	BResources rsrc;
	res = rsrc.SetTo(&file);
	if (res != B_OK)
		return res;

	size_t sz;
	const void *buf = rsrc.LoadResource('CADA', fLanguageName, &sz);
	if (!buf)
		return B_NAME_NOT_FOUND;

	BMemoryIO memIO(buf, sz);
	res = Unflatten(&memIO);

	return res;
}


status_t
DefaultCatalog::WriteToFile(const char *path)
{
	BFile catalogFile;
	if (path)
		fPath = path;
	status_t res = catalogFile.SetTo(fPath.String(),
		B_WRITE_ONLY | B_CREATE_FILE | B_ERASE_FILE);
	if (res != B_OK)
		return res;

	BMallocIO mallocIO;
	mallocIO.SetBlockSize(max(fCatMap.Size() * 20, (int32)256));
		// set a largish block-size in order to avoid reallocs
	res = Flatten(&mallocIO);
	if (res == B_OK) {
		ssize_t wsz;
		wsz = catalogFile.Write(mallocIO.Buffer(), mallocIO.BufferLength());
		if (wsz != (ssize_t)mallocIO.BufferLength())
			return B_FILE_ERROR;

		// set mimetype-, language- and signature-attributes:
		UpdateAttributes(catalogFile);
	}
	if (res == B_OK)
		UpdateAttributes(catalogFile);
	return res;
}


/*
 * this method is not currently being used, but it may be useful in the
 * future...
 */
status_t
DefaultCatalog::WriteToAttribute(const entry_ref &appOrAddOnRef)
{
	BNode node;
	status_t res = node.SetTo(&appOrAddOnRef);
	if (res != B_OK)
		return res;

	BMallocIO mallocIO;
	mallocIO.SetBlockSize(max(fCatMap.Size() * 20, (int32)256));
		// set a largish block-size in order to avoid reallocs
	res = Flatten(&mallocIO);

	if (res == B_OK) {
		ssize_t wsz;
		wsz = node.WriteAttr(BLocaleRoster::kEmbeddedCatAttr, B_MESSAGE_TYPE, 0,
			mallocIO.Buffer(), mallocIO.BufferLength());
		if (wsz < B_OK)
			res = wsz;
		else if (wsz != (ssize_t)mallocIO.BufferLength())
			res = B_ERROR;
	}
	return res;
}


status_t
DefaultCatalog::WriteToResource(const entry_ref &appOrAddOnRef)
{
	BFile file;
	status_t res = file.SetTo(&appOrAddOnRef, B_READ_WRITE);
	if (res != B_OK)
		return res;

	BResources rsrc;
	res = rsrc.SetTo(&file);
	if (res != B_OK)
		return res;

	BMallocIO mallocIO;
	mallocIO.SetBlockSize(max(fCatMap.Size() * 20, (int32)256));
		// set a largish block-size in order to avoid reallocs
	res = Flatten(&mallocIO);

	int mangledLanguage = CatKey::HashFun(fLanguageName.String(), 0);

	if (res == B_OK) {
		res = rsrc.AddResource('CADA', mangledLanguage,
			mallocIO.Buffer(), mallocIO.BufferLength(),
			BString(fLanguageName));
	}

	return res;
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
	if (catalogFile.ReadAttr(BLocaleRoster::kCatLangAttr, B_STRING_TYPE, 0,
			&buf, bufSize) <= 0
		|| fLanguageName != buf) {
		catalogFile.WriteAttr(BLocaleRoster::kCatLangAttr, B_STRING_TYPE, 0,
			fLanguageName.String(), fLanguageName.Length()+1);
	}
	if (catalogFile.ReadAttr(BLocaleRoster::kCatSigAttr, B_STRING_TYPE, 0,
			&buf, bufSize) <= 0
		|| fSignature != buf) {
		catalogFile.WriteAttr(BLocaleRoster::kCatSigAttr, B_STRING_TYPE, 0,
			fSignature.String(), fSignature.Length()+1);
	}
	if (catalogFile.ReadAttr(BLocaleRoster::kCatFingerprintAttr, B_UINT32_TYPE,
		0, &temp, sizeof(uint32)) <= 0) {
		catalogFile.WriteAttr(BLocaleRoster::kCatFingerprintAttr, B_UINT32_TYPE,
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
	int32 count = fCatMap.Size();
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
			// There is no resize method in Haiku's HashMap to preallocate
			// memory.
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
		if (fFingerprint != checkFP)
			return B_BAD_DATA;
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


} // namespace BPrivate


extern "C" status_t
default_catalog_get_available_languages(BMessage* availableLanguages,
	const char* sigPattern, const char* langPattern, int32 fingerprint)
{
	if (availableLanguages == NULL || sigPattern == NULL)
		return B_BAD_DATA;

	app_info appInfo;
	be_app->GetAppInfo(&appInfo);
	node_ref nref;
	nref.device = appInfo.ref.device;
	nref.node = appInfo.ref.directory;
	BDirectory appDir(&nref);
	BString catalogName("locale/");
	catalogName << kCatFolder
		<< "/" << sigPattern ;
	BPath catalogPath(&appDir, catalogName.String());
	BEntry file(catalogPath.Path());
	BDirectory dir(&file);

	char fileName[B_FILE_NAME_LENGTH];
	while(dir.GetNextEntry(&file) == B_OK) {
		file.GetName(fileName);
		BString langName(fileName);
		langName.Replace(kCatExtension, "", 1);
		availableLanguages->AddString("language", langName);
	}

	// search in data folders

	directory_which which[] = {
		B_USER_NONPACKAGED_DATA_DIRECTORY,
		B_USER_DATA_DIRECTORY,
		B_SYSTEM_NONPACKAGED_DATA_DIRECTORY,
		B_SYSTEM_DATA_DIRECTORY
	};

	for (size_t i = 0; i < sizeof(which) / sizeof(which[0]); i++) {
		BPath path;
		if (find_directory(which[i], &path) == B_OK) {
			catalogName = BString("locale/")
				<< kCatFolder
				<< "/" << sigPattern;

			BPath catalogPath(path.Path(), catalogName.String());
			BEntry file(catalogPath.Path());
			BDirectory dir(&file);

			char fileName[B_FILE_NAME_LENGTH];
			while(dir.GetNextEntry(&file) == B_OK) {
				file.GetName(fileName);
				BString langName(fileName);
				langName.Replace(kCatExtension, "", 1);
				availableLanguages->AddString("language", langName);
			}
		}
	}

	return B_OK;
}
