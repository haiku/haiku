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
#include <MutableLocaleRoster.h>

#include <cstdio>


using std::auto_ptr;
using std::min;
using std::max;
using std::pair;


/*!	This file implements the default catalog-type for the opentracker locale
	kit. Alternatively, this could be used as a full add-on, but currently this
	is provided as part of liblocale.so.
*/


static const char *kCatFolder = "catalogs";
static const char *kCatExtension = ".catalog";

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
	BHashMapCatalog("", language, fingerprint)
{
	// We created the catalog with an invalid signature, but we fix that now.
	SetSignature(catalogOwner);
	status_t status;

	app_info appInfo;
	be_app->GetAppInfo(&appInfo);

	// give highest priority to catalog embedded as resource in application
	// executable:
	status = ReadFromResource(&appInfo.ref);

	// search for catalog living in sub-folder of app's folder:
	if (status != B_OK) {
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
		status = ReadFromFile(catalogPath.Path());
	}

	if (status != B_OK) {
		// search in data folders

		directory_which which[] = {
			B_USER_DATA_DIRECTORY,
			B_COMMON_DATA_DIRECTORY,
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

	fInitCheck = status;
	log_team(LOG_DEBUG,
		"trying to load default-catalog(sig=%s, lang=%s) results in %s",
		fSignature.String(), language, strerror(fInitCheck));
}


/*!	Constructs a DefaultCatalog and reads it from the resources of the
	given entry-ref (which usually is an app- or add-on-file).
	InitCheck() will be B_OK if catalog could be loaded successfully, it will
	give an appropriate error-code otherwise.
*/
DefaultCatalog::DefaultCatalog(entry_ref *appOrAddOnRef)
	:
	BHashMapCatalog("", "", 0)
{
	fInitCheck = ReadFromResource(appOrAddOnRef);
	log_team(LOG_DEBUG,
		"trying to load embedded catalog from resources results in %s",
		strerror(fInitCheck));
}


/*!	Constructs an empty DefaultCatalog with given sig and language.
	This is used for editing/testing purposes.
	InitCheck() will always be B_OK.
*/
DefaultCatalog::DefaultCatalog(const char *path, const char *signature,
	const char *language)
	:
	BHashMapCatalog(signature, language, 0),
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
		log_team(LOG_ERR, "File %s has no mimesignature, so it can't use"
			" localization.", catalogOwner.name);
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

	log_team(LOG_DEBUG, "Image %s requested catalog with mimetype %s",
		catalogOwner.name, stripSignature);
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
	if (res != B_OK) {
		log_team(LOG_DEBUG, "LocaleKit DefaultCatalog: no catalog at %s", path);
		return B_ENTRY_NOT_FOUND;
	}

	fPath = path;
	log_team(LOG_DEBUG, "LocaleKit DefaultCatalog: found catalog at %s", path);

	off_t sz = 0;
	res = catalogFile.GetSize(&sz);
	if (res != B_OK) {
		log_team(LOG_ERR, "LocaleKit DefaultCatalog: couldn't get size for "
			"catalog-file %s", path);
		return res;
	}

	auto_ptr<char> buf(new(std::nothrow) char [sz]);
	if (buf.get() == NULL) {
		log_team(LOG_ERR, "LocaleKit DefaultCatalog: couldn't allocate array "
			"of %d chars", sz);
		return B_NO_MEMORY;
	}
	res = catalogFile.Read(buf.get(), sz);
	if (res < B_OK) {
		log_team(LOG_ERR, "LocaleKit DefaultCatalog: couldn't read from "
			"catalog-file %s", path);
		return res;
	}
	if (res < sz) {
		log_team(LOG_ERR,
			"LocaleKit DefaultCatalog: only got %lu instead of %Lu bytes from "
			"catalog-file %s", res, sz, path);
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


/*
 * this method is not currently being used, but it may be useful in the future...
 */
status_t
DefaultCatalog::ReadFromAttribute(entry_ref *appOrAddOnRef)
{
	BNode node;
	status_t res = node.SetTo(appOrAddOnRef);
	if (res != B_OK) {
		log_team(LOG_ERR,
			"couldn't find app or add-on (dev=%lu, dir=%Lu, name=%s)",
			appOrAddOnRef->device, appOrAddOnRef->directory,
			appOrAddOnRef->name);
		return B_ENTRY_NOT_FOUND;
	}

	log_team(LOG_DEBUG,
		"looking for embedded catalog-attribute in app/add-on"
		"(dev=%lu, dir=%Lu, name=%s)", appOrAddOnRef->device,
		appOrAddOnRef->directory, appOrAddOnRef->name);

	attr_info attrInfo;
	res = node.GetAttrInfo(BLocaleRoster::kEmbeddedCatAttr, &attrInfo);
	if (res != B_OK) {
		log_team(LOG_DEBUG, "no embedded catalog found");
		return B_NAME_NOT_FOUND;
	}
	if (attrInfo.type != B_MESSAGE_TYPE) {
		log_team(LOG_ERR, "attribute %s has incorrect type and is ignored!",
			BLocaleRoster::kEmbeddedCatAttr);
		return B_BAD_TYPE;
	}

	size_t size = attrInfo.size;
	auto_ptr<char> buf(new(std::nothrow) char [size]);
	if (buf.get() == NULL) {
		log_team(LOG_ERR, "couldn't allocate array of %d chars", size);
		return B_NO_MEMORY;
	}
	res = node.ReadAttr(BLocaleRoster::kEmbeddedCatAttr, B_MESSAGE_TYPE, 0,
		buf.get(), size);
	if (res < (ssize_t)size) {
		log_team(LOG_ERR, "unable to read embedded catalog from attribute");
		return res < B_OK ? res : B_BAD_DATA;
	}

	BMemoryIO memIO(buf.get(), size);
	res = Unflatten(&memIO);

	return res;
}


status_t
DefaultCatalog::ReadFromResource(entry_ref *appOrAddOnRef)
{
	BFile file;
	status_t res = file.SetTo(appOrAddOnRef, B_READ_ONLY);
	if (res != B_OK) {
		log_team(LOG_ERR,
			"couldn't find app or add-on (dev=%lu, dir=%Lu, name=%s)",
			appOrAddOnRef->device, appOrAddOnRef->directory,
			appOrAddOnRef->name);
		return B_ENTRY_NOT_FOUND;
	}

	log_team(LOG_DEBUG,
		"looking for embedded catalog-resource in app/add-on"
		"(dev=%lu, dir=%Lu, name=%s)", appOrAddOnRef->device,
		appOrAddOnRef->directory, appOrAddOnRef->name);

	BResources rsrc;
	res = rsrc.SetTo(&file);
	if (res != B_OK) {
		log_team(LOG_DEBUG, "file has no resources");
		return res;
	}

	size_t sz;
	const void *buf = rsrc.LoadResource(B_MESSAGE_TYPE,
		BLocaleRoster::kEmbeddedCatResId, &sz);
	if (!buf) {
		log_team(LOG_DEBUG, "file has no catalog-resource");
		return B_NAME_NOT_FOUND;
	}

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
	mallocIO.SetBlockSize(max(fCatMap.Size() * 20, 256L));
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
DefaultCatalog::WriteToAttribute(entry_ref *appOrAddOnRef)
{
	BNode node;
	status_t res = node.SetTo(appOrAddOnRef);
	if (res != B_OK)
		return res;

	BMallocIO mallocIO;
	mallocIO.SetBlockSize(max(fCatMap.Size() * 20, 256L));
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
DefaultCatalog::WriteToResource(entry_ref *appOrAddOnRef)
{
	BFile file;
	status_t res = file.SetTo(appOrAddOnRef, B_READ_WRITE);
	if (res != B_OK)
		return res;

	BResources rsrc;
	res = rsrc.SetTo(&file);
	if (res != B_OK)
		return res;

	BMallocIO mallocIO;
	mallocIO.SetBlockSize(max(fCatMap.Size() * 20, 256L));
		// set a largish block-size in order to avoid reallocs
	res = Flatten(&mallocIO);

	if (res == B_OK) {
		res = rsrc.AddResource(B_MESSAGE_TYPE, BLocaleRoster::kEmbeddedCatResId,
			mallocIO.Buffer(), mallocIO.BufferLength(), "embedded catalog");
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
			log_team(LOG_INFO, "default-catalog(sig=%s, lang=%s) "
				"has mismatching fingerprint (%ld instead of the requested %ld), "
				"so this catalog is skipped.",
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
		if (fFingerprint != checkFP) {
			log_team(LOG_WARNING, "default-catalog(sig=%s, lang=%s) "
				"has wrong fingerprint after load (%ld instead of the %ld). "
				"The catalog data may be corrupted, so this catalog is skipped.",
				fSignature.String(), fLanguageName.String(), checkFP,
				fFingerprint);
			return B_BAD_DATA;
		}
	}
	return res;
}


BCatalogAddOn *
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


BCatalogAddOn *
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

extern "C" status_t
default_catalog_get_available_languages(BMessage* availableLanguages,
	const char* sigPattern, const char* langPattern = NULL,
	int32 fingerprint = 0)
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
		B_USER_DATA_DIRECTORY,
		B_COMMON_DATA_DIRECTORY,
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
