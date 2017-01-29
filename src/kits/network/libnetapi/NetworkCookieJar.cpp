/*
 * Copyright 2010-2014 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Christophe Huriaux, c.huriaux@gmail.com
 *		Hamish Morrison, hamishm53@gmail.com
 */


#include <new>
#include <stdio.h>

#include <HashMap.h>
#include <HashString.h>
#include <Message.h>
#include <NetworkCookieJar.h>

#include "NetworkCookieJarPrivate.h"


// #define TRACE_COOKIE
#ifdef TRACE_COOKIE
#	define TRACE(x...) printf(x)
#else
#	define TRACE(x...) ;
#endif


const char* kArchivedCookieMessageName = "be:cookie";


BNetworkCookieJar::BNetworkCookieJar()
	:
	fCookieHashMap(new(std::nothrow) PrivateHashMap())
{
}


BNetworkCookieJar::BNetworkCookieJar(const BNetworkCookieJar& other)
	:
	fCookieHashMap(new(std::nothrow) PrivateHashMap())
{
	*this = other;
}


BNetworkCookieJar::BNetworkCookieJar(const BNetworkCookieList& otherList)
	:
	fCookieHashMap(new(std::nothrow) PrivateHashMap())
{
	AddCookies(otherList);
}


BNetworkCookieJar::BNetworkCookieJar(BMessage* archive)
	:
	fCookieHashMap(new(std::nothrow) PrivateHashMap())
{
	BMessage extractedCookie;

	for (int32 i = 0; archive->FindMessage(kArchivedCookieMessageName, i,
			&extractedCookie) == B_OK; i++) {
		BNetworkCookie* heapCookie
			= new(std::nothrow) BNetworkCookie(&extractedCookie);

		if (heapCookie == NULL)
			break;

		if (AddCookie(heapCookie) != B_OK) {
			delete heapCookie;
			continue;
		}
	}
}


BNetworkCookieJar::~BNetworkCookieJar()
{
	for (Iterator it = GetIterator(); it.Next() != NULL;) {
		delete it.Remove();
	}

	fCookieHashMap->Lock();

	PrivateHashMap::Iterator it = fCookieHashMap->GetIterator();
	while(it.HasNext()) {
		BNetworkCookieList* list = *it.NextValue();
		it.Remove();
		list->LockForWriting();
		delete list;
	}

	delete fCookieHashMap;
}


// #pragma mark Add cookie to cookie jar


status_t
BNetworkCookieJar::AddCookie(const BNetworkCookie& cookie)
{
	BNetworkCookie* heapCookie = new(std::nothrow) BNetworkCookie(cookie);
	if (heapCookie == NULL)
		return B_NO_MEMORY;

	status_t result = AddCookie(heapCookie);
	if (result != B_OK)
		delete heapCookie;

	return result;
}


status_t
BNetworkCookieJar::AddCookie(const BString& cookie, const BUrl& referrer)
{
	BNetworkCookie* heapCookie = new(std::nothrow) BNetworkCookie(cookie,
		referrer);

	if (heapCookie == NULL)
		return B_NO_MEMORY;

	status_t result = AddCookie(heapCookie);

	if (result != B_OK)
		delete heapCookie;

	return result;
}


status_t
BNetworkCookieJar::AddCookie(BNetworkCookie* cookie)
{
	if (fCookieHashMap == NULL)
		return B_NO_MEMORY;

	if (cookie == NULL || !cookie->IsValid())
		return B_BAD_VALUE;

	HashString key(cookie->Domain());

	if (!fCookieHashMap->Lock())
		return B_ERROR;

	// Get the cookies for the requested domain, or create a new list if there
	// isn't one yet.
	BNetworkCookieList* list = fCookieHashMap->Get(key);
	if (list == NULL) {
		list = new(std::nothrow) BNetworkCookieList();

		if (list == NULL) {
			fCookieHashMap->Unlock();
			return B_NO_MEMORY;
		}

		if (fCookieHashMap->Put(key, list) != B_OK) {
			fCookieHashMap->Unlock();
			delete list;
			return B_NO_MEMORY;
		}
	}

	if (list->LockForWriting() != B_OK) {
		fCookieHashMap->Unlock();
		return B_ERROR;
	}

	fCookieHashMap->Unlock();

	// Remove any cookie with the same key as the one we're trying to add (it
	// replaces/updates them)
	for (int32 i = 0; i < list->CountItems(); i++) {
		const BNetworkCookie* c = list->ItemAt(i);

		if (c->Name() == cookie->Name() && c->Path() == cookie->Path()) {
			list->RemoveItemAt(i);
			break;
		}
	}

	// If the cookie has an expiration date in the past, stop here: we
	// effectively deleted a cookie.
	if (cookie->ShouldDeleteNow()) {
		TRACE("Remove cookie: %s\n", cookie->RawCookie(true).String());
		delete cookie;
	} else {
		// Make sure the cookie has cached the raw string and expiration date
		// string, so it is now actually immutable. This way we can safely
		// read the cookie data from multiple threads without any locking.
		const BString& raw = cookie->RawCookie(true);
		(void)raw;

		TRACE("Add cookie: %s\n", raw.String());

		// Keep the list sorted by path length (longest first). This makes sure
		// that cookies for most specific paths are returned first when
		// iterating the cookie jar.
		int32 i;
		for (i = 0; i < list->CountItems(); i++) {
			const BNetworkCookie* current = list->ItemAt(i);
			if (current->Path().Length() < cookie->Path().Length())
				break;
		}
		list->AddItem(cookie, i);
	}

	list->Unlock();

	return B_OK;
}


status_t
BNetworkCookieJar::AddCookies(const BNetworkCookieList& cookies)
{
	for (int32 i = 0; i < cookies.CountItems(); i++) {
		const BNetworkCookie* cookiePtr = cookies.ItemAt(i);

		// Using AddCookie by reference in order to avoid multiple
		// cookie jar share the same cookie pointers
		status_t result = AddCookie(*cookiePtr);
		if (result != B_OK)
			return result;
	}

	return B_OK;
}


// #pragma mark Purge useless cookies


uint32
BNetworkCookieJar::DeleteOutdatedCookies()
{
	int32 deleteCount = 0;
	const BNetworkCookie* cookiePtr;

	for (Iterator it = GetIterator(); (cookiePtr = it.Next()) != NULL;) {
		if (cookiePtr->ShouldDeleteNow()) {
			delete it.Remove();
			deleteCount++;
		}
	}

	return deleteCount;
}


uint32
BNetworkCookieJar::PurgeForExit()
{
	int32 deleteCount = 0;
	const BNetworkCookie* cookiePtr;

	for (Iterator it = GetIterator(); (cookiePtr = it.Next()) != NULL;) {
		if (cookiePtr->ShouldDeleteAtExit()) {
			delete it.Remove();
			deleteCount++;
		}
	}

	return deleteCount;
}


// #pragma mark BArchivable interface


status_t
BNetworkCookieJar::Archive(BMessage* into, bool deep) const
{
	status_t error = BArchivable::Archive(into, deep);

	if (error == B_OK) {
		const BNetworkCookie* cookiePtr;

		for (Iterator it = GetIterator(); (cookiePtr = it.Next()) != NULL;) {
			BMessage subArchive;

			error = cookiePtr->Archive(&subArchive, deep);
			if (error != B_OK)
				return error;

			error = into->AddMessage(kArchivedCookieMessageName, &subArchive);
			if (error != B_OK)
				return error;
		}
	}

	return error;
}


BArchivable*
BNetworkCookieJar::Instantiate(BMessage* archive)
{
	if (archive->HasMessage(kArchivedCookieMessageName))
		return new(std::nothrow) BNetworkCookieJar(archive);

	return NULL;
}


// #pragma mark BFlattenable interface


bool
BNetworkCookieJar::IsFixedSize() const
{
	// Flattened size vary
	return false;
}


type_code
BNetworkCookieJar::TypeCode() const
{
	// TODO: Add a B_COOKIEJAR_TYPE
	return B_ANY_TYPE;
}


ssize_t
BNetworkCookieJar::FlattenedSize() const
{
	_DoFlatten();
	return fFlattened.Length() + 1;
}


status_t
BNetworkCookieJar::Flatten(void* buffer, ssize_t size) const
{
	if (FlattenedSize() > size)
		return B_ERROR;

	fFlattened.CopyInto(reinterpret_cast<char*>(buffer), 0,
		fFlattened.Length());
	reinterpret_cast<char*>(buffer)[fFlattened.Length()] = 0;

	return B_OK;
}


bool
BNetworkCookieJar::AllowsTypeCode(type_code) const
{
	// TODO
	return false;
}


status_t
BNetworkCookieJar::Unflatten(type_code, const void* buffer, ssize_t size)
{
	BString flattenedCookies;
	flattenedCookies.SetTo(reinterpret_cast<const char*>(buffer), size);

	while (flattenedCookies.Length() > 0) {
		BNetworkCookie tempCookie;
		BString tempCookieLine;

		int32 endOfLine = flattenedCookies.FindFirst('\n', 0);
		if (endOfLine == -1)
			tempCookieLine = flattenedCookies;
		else {
			flattenedCookies.MoveInto(tempCookieLine, 0, endOfLine);
			flattenedCookies.Remove(0, 1);
		}

		if (tempCookieLine.Length() != 0 && tempCookieLine[0] != '#') {
			for (int32 field = 0; field < 7; field++) {
				BString tempString;

				int32 endOfField = tempCookieLine.FindFirst('\t', 0);
				if (endOfField == -1)
					tempString = tempCookieLine;
				else {
					tempCookieLine.MoveInto(tempString, 0, endOfField);
					tempCookieLine.Remove(0, 1);
				}

				switch (field) {
					case 0:
						tempCookie.SetDomain(tempString);
						break;

					case 1:
						// TODO: Useless field ATM
						break;

					case 2:
						tempCookie.SetPath(tempString);
						break;

					case 3:
						tempCookie.SetSecure(tempString == "TRUE");
						break;

					case 4:
						tempCookie.SetExpirationDate(atoi(tempString));
						break;

					case 5:
						tempCookie.SetName(tempString);
						break;

					case 6:
						tempCookie.SetValue(tempString);
						break;
				} // switch
			} // for loop

			AddCookie(tempCookie);
		}
	}

	return B_OK;
}


BNetworkCookieJar&
BNetworkCookieJar::operator=(const BNetworkCookieJar& other)
{
	if (&other == this)
		return *this;

	for (Iterator it = GetIterator(); it.Next() != NULL;)
		delete it.Remove();

	BArchivable::operator=(other);
	BFlattenable::operator=(other);

	fFlattened = other.fFlattened;

	delete fCookieHashMap;
	fCookieHashMap = new(std::nothrow) PrivateHashMap();

	for (Iterator it = other.GetIterator(); it.HasNext();) {
		const BNetworkCookie* cookie = it.Next();
		AddCookie(*cookie); // Pass by reference so the cookie is copied.
	}

	return *this;
}


// #pragma mark Iterators


BNetworkCookieJar::Iterator
BNetworkCookieJar::GetIterator() const
{
	return BNetworkCookieJar::Iterator(this);
}


BNetworkCookieJar::UrlIterator
BNetworkCookieJar::GetUrlIterator(const BUrl& url) const
{
	if (!url.HasPath()) {
		BUrl copy(url);
		copy.SetPath("/");
		return BNetworkCookieJar::UrlIterator(this, copy);
	}

	return BNetworkCookieJar::UrlIterator(this, url);
}


void
BNetworkCookieJar::_DoFlatten() const
{
	fFlattened.Truncate(0);

	const BNetworkCookie* cookiePtr;
	for (Iterator it = GetIterator(); (cookiePtr = it.Next()) != NULL;) {
		fFlattened 	<< cookiePtr->Domain() << '\t' << "TRUE" << '\t'
			<< cookiePtr->Path() << '\t'
			<< (cookiePtr->Secure()?"TRUE":"FALSE") << '\t'
			<< (int32)cookiePtr->ExpirationDate() << '\t'
			<< cookiePtr->Name() << '\t' << cookiePtr->Value() << '\n';
	}
}


// #pragma mark Iterator


BNetworkCookieJar::Iterator::Iterator(const Iterator& other)
	:
	fCookieJar(other.fCookieJar),
	fIterator(NULL),
	fLastList(NULL),
	fList(NULL),
	fElement(NULL),
	fLastElement(NULL),
	fIndex(0)
{
	fIterator = new(std::nothrow) PrivateIterator(
		fCookieJar->fCookieHashMap->GetIterator());

	_FindNext();
}


BNetworkCookieJar::Iterator::Iterator(const BNetworkCookieJar* cookieJar)
	:
	fCookieJar(const_cast<BNetworkCookieJar*>(cookieJar)),
	fIterator(NULL),
	fLastList(NULL),
	fList(NULL),
	fElement(NULL),
	fLastElement(NULL),
	fIndex(0)
{
	fIterator = new(std::nothrow) PrivateIterator(
		fCookieJar->fCookieHashMap->GetIterator());

	// Locate first cookie
	_FindNext();
}


BNetworkCookieJar::Iterator::~Iterator()
{
	if (fList != NULL)
		fList->Unlock();
	if (fLastList != NULL)
		fLastList->Unlock();

	delete fIterator;
}


BNetworkCookieJar::Iterator&
BNetworkCookieJar::Iterator::operator=(const Iterator& other)
{
	if (this == &other)
		return *this;

	delete fIterator;
	if (fList != NULL)
		fList->Unlock();

	fCookieJar = other.fCookieJar;
	fIterator = NULL;
	fLastList = NULL;
	fList = NULL;
	fElement = NULL;
	fLastElement = NULL;
	fIndex = 0;

	fIterator = new(std::nothrow) PrivateIterator(
		fCookieJar->fCookieHashMap->GetIterator());

	_FindNext();

	return *this;
}


bool
BNetworkCookieJar::Iterator::HasNext() const
{
	return fElement;
}


const BNetworkCookie*
BNetworkCookieJar::Iterator::Next()
{
	if (!fElement)
		return NULL;

	const BNetworkCookie* result = fElement;
	_FindNext();
	return result;
}


const BNetworkCookie*
BNetworkCookieJar::Iterator::NextDomain()
{
	if (!fElement)
		return NULL;

	const BNetworkCookie* result = fElement;

	if (!fIterator->fCookieMapIterator.HasNext()) {
		fElement = NULL;
		return result;
	}

	if (fList != NULL)
		fList->Unlock();

	if (fCookieJar->fCookieHashMap->Lock()) {
		fList = *fIterator->fCookieMapIterator.NextValue();
		fList->LockForReading();

		while (fList->CountItems() == 0 && fIterator->fCookieMapIterator.HasNext()) {
			// Empty list. Skip it
			fList->Unlock();
			fList = *fIterator->fCookieMapIterator.NextValue();
			fList->LockForReading();
		}

		fCookieJar->fCookieHashMap->Unlock();
	}

	fIndex = 0;
	fElement = fList->ItemAt(fIndex);
	return result;
}


const BNetworkCookie*
BNetworkCookieJar::Iterator::Remove()
{
	if (!fLastElement)
		return NULL;

	const BNetworkCookie* result = fLastElement;

	if (fIndex == 0) {
		if (fLastList && fCookieJar->fCookieHashMap->Lock()) {
			// We are on the first item of fList, so we need to remove the
			// last of fLastList
			fLastList->Unlock();
			if (fLastList->LockForWriting() == B_OK)
			{
				fLastList->RemoveItemAt(fLastList->CountItems() - 1);
				// TODO if the list became empty, we could remove it from the
				// map, but this can be a problem if other iterators are still
				// referencing it. Is there a safe place and locking pattern
				// where we can do that?
				fLastList->Unlock();
				fLastList->LockForReading();
			}
			fCookieJar->fCookieHashMap->Unlock();
		}
	} else {
		fIndex--;

		if (fCookieJar->fCookieHashMap->Lock()) {
			// Switch to a write lock
			fList->Unlock();
			if (fList->LockForWriting() == B_OK)
			{
				fList->RemoveItemAt(fIndex);
				fList->Unlock();
			}
			fList->LockForReading();
			fCookieJar->fCookieHashMap->Unlock();
		}
	}

	fLastElement = NULL;
	return result;
}


void
BNetworkCookieJar::Iterator::_FindNext()
{
	fLastElement = fElement;

	fIndex++;
	if (fList && fIndex < fList->CountItems()) {
		// Get an element from the current list
		fElement = fList->ItemAt(fIndex);
		return;
	}

	if (fIterator == NULL || !fIterator->fCookieMapIterator.HasNext()) {
		// We are done iterating
		fElement = NULL;
		return;
	}

	// Get an element from the next list
	if (fLastList != NULL) {
		fLastList->Unlock();
	}
	fLastList = fList;

	if (fCookieJar->fCookieHashMap->Lock()) {
		fList = *(fIterator->fCookieMapIterator.NextValue());
		fList->LockForReading();

		while (fList->CountItems() == 0 && fIterator->fCookieMapIterator.HasNext()) {
			// Empty list. Skip it
			fList->Unlock();
			fList = *fIterator->fCookieMapIterator.NextValue();
			fList->LockForReading();
		}

		fCookieJar->fCookieHashMap->Unlock();
	}

	fIndex = 0;
	fElement = fList->ItemAt(fIndex);
}


// #pragma mark URL Iterator


BNetworkCookieJar::UrlIterator::UrlIterator(const UrlIterator& other)
	:
	fCookieJar(other.fCookieJar),
	fIterator(NULL),
	fList(NULL),
	fLastList(NULL),
	fElement(NULL),
	fLastElement(NULL),
	fIndex(0),
	fLastIndex(0),
	fUrl(other.fUrl)
{
	_Initialize();
}


BNetworkCookieJar::UrlIterator::UrlIterator(const BNetworkCookieJar* cookieJar,
	const BUrl& url)
	:
	fCookieJar(const_cast<BNetworkCookieJar*>(cookieJar)),
	fIterator(NULL),
	fList(NULL),
	fLastList(NULL),
	fElement(NULL),
	fLastElement(NULL),
	fIndex(0),
	fLastIndex(0),
	fUrl(url)
{
	_Initialize();
}


BNetworkCookieJar::UrlIterator::~UrlIterator()
{
	if (fList != NULL)
		fList->Unlock();
	if (fLastList != NULL)
		fLastList->Unlock();

	delete fIterator;
}


bool
BNetworkCookieJar::UrlIterator::HasNext() const
{
	return fElement;
}


const BNetworkCookie*
BNetworkCookieJar::UrlIterator::Next()
{
	if (!fElement)
		return NULL;

	const BNetworkCookie* result = fElement;
	_FindNext();
	return result;
}


const BNetworkCookie*
BNetworkCookieJar::UrlIterator::Remove()
{
	if (!fLastElement)
		return NULL;

	const BNetworkCookie* result = fLastElement;

	if (fCookieJar->fCookieHashMap->Lock()) {
		fLastList->Unlock();
		if (fLastList->LockForWriting() == B_OK)
		{
			fLastList->RemoveItemAt(fLastIndex);

			if (fLastList->CountItems() == 0) {
				fIterator->fCookieMapIterator.Remove();
				delete fLastList;
				fLastList = NULL;
			} else {
				fLastList->Unlock();
				fLastList->LockForReading();
			}
		}
		fCookieJar->fCookieHashMap->Unlock();
	}

	fLastElement = NULL;
	return result;
}


BNetworkCookieJar::UrlIterator&
BNetworkCookieJar::UrlIterator::operator=(
	const BNetworkCookieJar::UrlIterator& other)
{
	if (this == &other)
		return *this;

	// Teardown
	if (fList)
		fList->Unlock();

	delete fIterator;

	// Init
	fCookieJar = other.fCookieJar;
	fIterator = NULL;
	fList = NULL;
	fLastList = NULL;
	fElement = NULL;
	fLastElement = NULL;
	fIndex = 0;
	fLastIndex = 0;
	fUrl = other.fUrl;

	_Initialize();

	return *this;
}


void
BNetworkCookieJar::UrlIterator::_Initialize()
{
	BString domain = fUrl.Host();

	if (!domain.Length()) {
		if (fUrl.Protocol() == "file")
			domain = "localhost";
		else
			return;
	}

	fIterator = new(std::nothrow) PrivateIterator(
		fCookieJar->fCookieHashMap->GetIterator());

	if (fIterator != NULL) {
		// Prepending a dot since _FindNext is going to call _SupDomain()
		domain.Prepend(".");
		fIterator->fKey.SetTo(domain, domain.Length());
		_FindNext();
	}
}


bool
BNetworkCookieJar::UrlIterator::_SuperDomain()
{
	BString domain(fIterator->fKey.GetString());
		// Makes a copy of the characters from the key. This is important,
		// because HashString doesn't like SetTo to be called with a substring
		// of its original string (use-after-free + memcpy overwrite).
	int32 firstDot = domain.FindFirst('.');
	if (firstDot < 0)
		return false;

	const char* nextDot = domain.String() + firstDot;

	fIterator->fKey.SetTo(nextDot + 1);
	return true;
}


void
BNetworkCookieJar::UrlIterator::_FindNext()
{
	fLastIndex = fIndex;
	fLastElement = fElement;
	if (fLastList != NULL)
		fLastList->Unlock();

	fLastList = fList;
	if (fCookieJar->fCookieHashMap->Lock()) {
		if (fLastList)
			fLastList->LockForReading();

		while (!_FindPath()) {
			if (!_SuperDomain()) {
				fElement = NULL;
				fCookieJar->fCookieHashMap->Unlock();
				return;
			}

			_FindDomain();
		}
		fCookieJar->fCookieHashMap->Unlock();
	}
}


void
BNetworkCookieJar::UrlIterator::_FindDomain()
{
	if (fList != NULL)
		fList->Unlock();

	if (fCookieJar->fCookieHashMap->Lock()) {
		fList = fCookieJar->fCookieHashMap->Get(fIterator->fKey);

		if (fList == NULL)
			fElement = NULL;
		else {
			fList->LockForReading();
		}
		fCookieJar->fCookieHashMap->Unlock();
	}

	fIndex = -1;
}


bool
BNetworkCookieJar::UrlIterator::_FindPath()
{
	fIndex++;
	while (fList && fIndex < fList->CountItems()) {
		fElement = fList->ItemAt(fIndex);

		if (fElement->IsValidForPath(fUrl.Path()))
			return true;

		fIndex++;
	}

	return false;
}


// #pragma mark - BNetworkCookieList


BNetworkCookieList::BNetworkCookieList()
{
	pthread_rwlock_init(&fLock, NULL);
}


BNetworkCookieList::~BNetworkCookieList()
{
	// Note: this is expected to be called with the write lock held.
	pthread_rwlock_destroy(&fLock);
}


status_t
BNetworkCookieList::LockForReading()
{
	return pthread_rwlock_rdlock(&fLock);
}


status_t
BNetworkCookieList::LockForWriting()
{
	return pthread_rwlock_wrlock(&fLock);
}


status_t
BNetworkCookieList::Unlock()
{
	return pthread_rwlock_unlock(&fLock);
}

