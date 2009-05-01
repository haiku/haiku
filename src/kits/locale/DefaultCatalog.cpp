/*
 * Copyright 2003, Oliver Tappe, zooey@hirschkaefer.de. All rights reserved.
 * Distributed under the terms of the MIT License.
 */


#include <memory>
#include <syslog.h>

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

/*
 *	This file implements the default catalog-type for the opentracker locale kit.
 *  Alternatively, this could be used as a full add-on, but currently this
 *  is provided as part of liblocale.so.
 */

extern "C" uint32 adler32(uint32 adler, const uint8 *buf, uint32 len);
	// definition lives in adler32.c

#if B_BEOS_VERSION <= B_BEOS_VERSION_5 && !defined(__HAIKU__)
// B_BAD_DATA was introduced with DANO, so we define it for R5:
#	define B_BAD_DATA -2147483632L
#endif


/*
 * CatKey
 */
size_t hash<CatKey>::operator()(const CatKey &key) const 
{
	return key.fHashVal;
}


static const char kSeparator = '\01';


CatKey::CatKey(const char *str, const char *ctx, const char *cmt)
	:
	fFlags(0)
{
	uint32 strLen = str ? strlen(str) : 0;
	uint32 ctxLen = ctx ? strlen(ctx) : 0;
	uint32 cmtLen = cmt ? strlen(cmt) : 0;
	int32 keyLen = strLen + ctxLen + cmtLen + 2;
	char *keyBuf = fKey.LockBuffer(keyLen);
	if (!keyBuf)
		return;
	if (strLen) {
		memcpy(keyBuf, str, strLen);
		keyBuf += strLen;
	}
	*keyBuf++ = kSeparator;
	if (ctxLen) {
		memcpy(keyBuf, ctx, ctxLen);
		keyBuf += ctxLen;
	}
	*keyBuf++ = kSeparator;
	if (cmtLen) {
		memcpy(keyBuf, cmt, cmtLen);
		keyBuf += cmtLen;
	}
	*keyBuf = '\0';
	fKey.UnlockBuffer(keyLen);
	fHashVal = HashFun(fKey.String());
}


CatKey::CatKey(uint32 id)
	:
	fHashVal(id),
	fFlags(0)
{
}


CatKey::CatKey()
	:
	fHashVal(0),
	fFlags(0)
{
}


bool 
CatKey::operator== (const CatKey& right) const
{
	// Two keys are equal if their hashval and key (string,context,comment) 
	// are equal:
	return fHashVal == right.fHashVal
		&& fKey == right.fKey;
}


status_t
CatKey::GetStringParts(BString* str, BString* ctx, BString* cmt) const
{
	int32 pos1 = fKey.FindFirst(kSeparator);
	if (pos1 < B_OK) {
		if (str)
			str->SetTo(fKey);
		if (ctx)
			ctx->Truncate(0);
		if (cmt)
			cmt->Truncate(0);
	} else {
		if (str)
			fKey.CopyInto(*str, 0, pos1);
		int32 pos2 = fKey.FindFirst(kSeparator, pos1+1);
		if (pos2 < B_OK) {
			if (ctx)
				ctx->SetTo(fKey, pos1+1);
			if (cmt)
				cmt->Truncate(0);
		} else {
			if (ctx)
				fKey.CopyInto(*ctx, pos1+1, pos2-pos1-1);
			if (cmt)
				cmt->SetTo(fKey.String()+pos2+1);
		}
	}

	return B_OK;
}


size_t CatKey::HashFun(const char* s) { 
	unsigned long h = 0; 
	for ( ; *s; ++s) 
		h = 5*h + *s; 

	return size_t(h); 
}


static const char *kCatFolder = "catalogs";
static const char *kCatExtension = ".catalog";

const char *DefaultCatalog::kCatMimeType 
	= "locale/x-vnd.Be.locale-catalog.default";

static int16 kCatArchiveVersion = 1;
	// version of the catalog archive structure, bump this if you change it!


/* 
 * constructs a DefaultCatalog with given signature and language and reads
 * the catalog from disk.
 * InitCheck() will be B_OK if catalog could be loaded successfully, it will
 * give an appropriate error-code otherwise.
 */
DefaultCatalog::DefaultCatalog(const char *signature, const char *language,
	int32 fingerprint)
	:
	BCatalogAddOn(signature, language, fingerprint)
{
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
		find_directory(B_COMMON_ETC_DIRECTORY, &commonEtcPath);
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
	log_team(LOG_DEBUG, 
		"trying to load default-catalog(sig=%s, lang=%s) results in %s",
		signature, language, strerror(fInitCheck));
}


/*
 * constructs a DefaultCatalog and reads it from the resources of the
 * given entry-ref (which usually is an app- or add-on-file).
 * InitCheck() will be B_OK if catalog could be loaded successfully, it will
 * give an appropriate error-code otherwise.
 */
DefaultCatalog::DefaultCatalog(entry_ref *appOrAddOnRef)
	:
	BCatalogAddOn("", "", 0)
{
	fInitCheck = ReadFromResource(appOrAddOnRef);
	log_team(LOG_DEBUG, 
		"trying to load embedded catalog from resources results in %s",
		strerror(fInitCheck));
}


/*
 * constructs an empty DefaultCatalog with given sig and language.
 * This is used for editing/testing purposes.
 * InitCheck() will always be B_OK.
 */
DefaultCatalog::DefaultCatalog(const char *path, const char *signature, 
	const char *language)
	:
	BCatalogAddOn(signature, language, 0),
	fPath(path)
{
	fInitCheck = B_OK;
}


DefaultCatalog::~DefaultCatalog()
{
}


status_t
DefaultCatalog::ReadFromFile(const char *path)
{
	if (!path)
		path = fPath.String();

	BFile catalogFile;
	status_t res = catalogFile.SetTo(path, B_READ_ONLY);
	if (res != B_OK) {
		log_team(LOG_DEBUG, "no catalog at %s", path);
		return B_ENTRY_NOT_FOUND;
	}

	fPath = path;
	log_team(LOG_DEBUG, "found catalog at %s", path);

	off_t sz = 0;
	res = catalogFile.GetSize(&sz);
	if (res != B_OK) {
		log_team(LOG_ERR, "couldn't get size for catalog-file %s", path);
		return res;
	}
	
	auto_ptr<char> buf(new char [sz]);
	res = catalogFile.Read(buf.get(), sz);
	if (res < B_OK) {
		log_team(LOG_ERR, "couldn't read from catalog-file %s", path);
		return res;
	}
	if (res < sz) {
		log_team(LOG_ERR, "only got %lu instead of %Lu bytes from catalog-file %s", 
			res, sz, path);
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
		log_team(LOG_ERR, "couldn't find app or add-on (dev=%lu, dir=%Lu, name=%s)",
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
	auto_ptr<char> buf(new char [size]);
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
		log_team(LOG_ERR, "couldn't find app or add-on (dev=%lu, dir=%Lu, name=%s)",
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
	mallocIO.SetBlockSize(max(fCatMap.size()*20, 256UL));
		// set a largish block-size in order to avoid reallocs
	res = Flatten(&mallocIO);
	if (res == B_OK) {
		ssize_t wsz;
		wsz = catalogFile.Write(mallocIO.Buffer(), mallocIO.BufferLength());
		if (wsz != (ssize_t)mallocIO.BufferLength())
			return B_FILE_ERROR;

		// set mimetype-, language- and signature-attributes:
		UpdateAttributes(catalogFile);
		// finally write fingerprint:
		catalogFile.WriteAttr(BLocaleRoster::kCatFingerprintAttr, B_INT32_TYPE, 
			0, &fFingerprint, sizeof(int32));
	}
	if (res == B_OK)
		UpdateAttributes(catalogFile);	
	return res;
}


/*
 * this method is not currently being used, but it may be useful in the future...
 */
status_t
DefaultCatalog::WriteToAttribute(entry_ref *appOrAddOnRef)
{
	BNode node;
	status_t res = node.SetTo(appOrAddOnRef);
	if (res != B_OK)
		return res;
		
	BMallocIO mallocIO;
	mallocIO.SetBlockSize(max(fCatMap.size()*20, 256UL));
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
	mallocIO.SetBlockSize(max(fCatMap.size()*20, 256UL));
		// set a largish block-size in order to avoid reallocs
	res = Flatten(&mallocIO);

	if (res == B_OK)
		res = rsrc.AddResource(B_MESSAGE_TYPE, BLocaleRoster::kEmbeddedCatResId,
			mallocIO.Buffer(), mallocIO.BufferLength(), "embedded catalog");

	return res;
}


void
DefaultCatalog::MakeEmpty()
{
	fCatMap.clear();
}


int32
DefaultCatalog::CountItems() const
{
	return fCatMap.size();
}


const char *
DefaultCatalog::GetString(const char *string, const char *context, 
	const char *comment)
{
	CatKey key(string, context, comment);
	return GetString(key);
}


const char *
DefaultCatalog::GetString(uint32 id)
{
	CatKey key(id);
	return GetString(key);
}


const char *
DefaultCatalog::GetString(const CatKey& key)
{
	CatMap::const_iterator iter = fCatMap.find(key);
	if (iter != fCatMap.end())
		return iter->second.String();
	else
		return NULL;
}


status_t
DefaultCatalog::SetString(const char *string, const char *translated, 
	const char *context, const char *comment)
{
	CatKey key(string, context, comment);
	fCatMap[key] = translated;
		// overwrite existing element
	return B_OK;
}


status_t
DefaultCatalog::SetString(int32 id, const char *translated)
{
	CatKey key(id);
	fCatMap[key] = translated;
		// overwrite existing element
	return B_OK;
}


status_t
DefaultCatalog::SetString(const CatKey& key, const char *translated)
{
	fCatMap[key] = translated;
		// overwrite existing element
	return B_OK;
}


/*
 * computes an adler32-checksum (we call it fingerprint) on all the catalog-keys.
 * We do not include the values, since we want catalogs for different languages
 * of the same app to have the same fingerprint, since we use it to separate
 * different catalog-versions.
 */
int32
DefaultCatalog::ComputeFingerprint() const
{
	uint32 adler = adler32(0, NULL, 0);

	int32 hash;
	CatMap::const_iterator iter;
	for (iter = fCatMap.begin(); iter!=fCatMap.end(); ++iter) {
		hash = B_HOST_TO_LENDIAN_INT32(iter->first.fHashVal);
		adler = adler32(adler, reinterpret_cast<uint8*>(&hash), sizeof(int32));
	}
	return adler;
}


void
DefaultCatalog::UpdateFingerprint()
{
	fFingerprint = ComputeFingerprint();
}


/*
 * writes mimetype, language-name and signature of catalog into the catalog-file.
 */
void
DefaultCatalog::UpdateAttributes(BFile& catalogFile)
{
	static const int bufSize = 256;
	char buf[bufSize];
	if (catalogFile.ReadAttr("BEOS:TYPE", B_MIME_STRING_TYPE, 0, &buf, bufSize) <= 0
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
}


status_t
DefaultCatalog::Flatten(BDataIO *dataIO)
{
	UpdateFingerprint();
		// make sure we have the correct fingerprint before we flatten it

	status_t res;
	BMessage archive;
	int32 count = fCatMap.size();
	res = archive.AddString("class", "DefaultCatalog")
		|| archive.AddInt32("c:sz", count)
		|| archive.AddInt16("c:ver", kCatArchiveVersion)
		|| archive.AddString("c:lang", fLanguageName.String())
		|| archive.AddString("c:sig", fSignature.String())
		|| archive.AddInt32("c:fpr", fFingerprint);
	if (res == B_OK)
		res = archive.Flatten(dataIO);

	CatMap::const_iterator iter;
	for (iter = fCatMap.begin(); res==B_OK && iter!=fCatMap.end(); ++iter) {
		archive.MakeEmpty();
		res = archive.AddString("c:key", iter->first.fKey.String())
			|| archive.AddInt32("c:hash", iter->first.fHashVal)
			|| archive.AddString("c:tstr", iter->second.String());
		if (res == B_OK)
			res = archive.Flatten(dataIO);
	}
	return res;
}


status_t
DefaultCatalog::Unflatten(BDataIO *dataIO)
{
	fCatMap.clear();
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
		int32 foundFingerprint = archiveMsg.FindInt32("c:fpr");

		// if a specific fingerprint has been requested and the catalog does in fact
		// have a fingerprint, both are compared. If they mismatch, we do not accept
		// this catalog:
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
		const char *translated;
#ifdef __GCC
		fCatMap.resize(count);
#endif
		for (int i=0; res==B_OK && i<count; ++i) {
			res = archiveMsg.Unflatten(dataIO);
			if (res == B_OK) {
				res = archiveMsg.FindString("c:key", &keyStr)
					|| archiveMsg.FindInt32("c:hash", (int32*)&key.fHashVal)
					|| archiveMsg.FindString("c:tstr", &translated);
			}
			if (res == B_OK) {
				key.fKey = keyStr;
				fCatMap.insert(pair<const BPrivate::CatKey, BString>(key, translated));
			}
		}
		int32 checkFP = ComputeFingerprint();
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
DefaultCatalog::Instantiate(const char *signature, const char *language, 
	int32 fingerprint)
{
	DefaultCatalog *catalog = new DefaultCatalog(signature, language, fingerprint);
	if (catalog && catalog->InitCheck() != B_OK) {
		delete catalog;
		return NULL;
	}
	return catalog;
}


BCatalogAddOn *
DefaultCatalog::InstantiateEmbedded(entry_ref *appOrAddOnRef)
{
	DefaultCatalog *catalog = new DefaultCatalog(appOrAddOnRef);
	if (catalog && catalog->InitCheck() != B_OK) {
		delete catalog;
		return NULL;
	}
	return catalog;
}


BCatalogAddOn *
DefaultCatalog::Create(const char *signature, const char *language)
{
	DefaultCatalog *catalog = new DefaultCatalog("", signature, language);
	if (catalog && catalog->InitCheck() != B_OK) {
		delete catalog;
		return NULL;
	}
	return catalog;
}


const uint8 DefaultCatalog::kDefaultCatalogAddOnPriority = 1;
	// give highest priority to our embedded catalog-add-on
