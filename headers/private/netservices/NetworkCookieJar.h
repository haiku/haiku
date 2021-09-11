/*
 * Copyright 2010-2013 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_NETWORK_COOKIE_JAR_H_
#define _B_NETWORK_COOKIE_JAR_H_

#include <pthread.h>

#include <Archivable.h>
#include <Flattenable.h>
#include <Message.h>
#include <NetworkCookie.h>
#include <ObjectList.h>
#include <String.h>
#include <Url.h>


namespace BPrivate {

namespace Network {


class BNetworkCookieList: public BObjectList<const BNetworkCookie> {
public:
								BNetworkCookieList();
								~BNetworkCookieList();

			status_t			LockForReading();
			status_t			LockForWriting();
			status_t			Unlock();
private:
			pthread_rwlock_t	fLock;
};


class BNetworkCookieJar : public BArchivable, public BFlattenable {
public:
			// Nested types
			class Iterator;
			class UrlIterator;
			struct PrivateIterator;
			struct PrivateHashMap;

public:
								BNetworkCookieJar();
								BNetworkCookieJar(
									const BNetworkCookieJar& other);
								BNetworkCookieJar(
									const BNetworkCookieList& otherList);
								BNetworkCookieJar(BMessage* archive);
	virtual						~BNetworkCookieJar();

			status_t			AddCookie(const BNetworkCookie& cookie);
			status_t			AddCookie(const BString& cookie,
									const BUrl& url);
			status_t			AddCookie(BNetworkCookie* cookie);
			status_t			AddCookies(const BNetworkCookieList& cookies);

			uint32				DeleteOutdatedCookies();
			uint32				PurgeForExit();

	// BArchivable members
	virtual	status_t			Archive(BMessage* into,
									bool deep = true) const;
	static	BArchivable*		Instantiate(BMessage* archive);

	// BFlattenable members
	virtual	bool				IsFixedSize() const;
	virtual	type_code			TypeCode() const;
	virtual	ssize_t				FlattenedSize() const;
	virtual	status_t			Flatten(void* buffer, ssize_t size)
									const;
	virtual	bool				AllowsTypeCode(type_code code) const;
	virtual	status_t			Unflatten(type_code code,
									const void* buffer, ssize_t size);

			BNetworkCookieJar&	operator=(const BNetworkCookieJar& other);

	// Iterators
			Iterator			GetIterator() const;
			UrlIterator			GetUrlIterator(const BUrl& url) const;


private:
			void				_DoFlatten() const;

private:
	friend	class Iterator;
	friend	class UrlIterator;

			PrivateHashMap*		fCookieHashMap;
	mutable	BString				fFlattened;
};


class BNetworkCookieJar::Iterator {
public:
								Iterator(const Iterator& other);
								~Iterator();

			Iterator& 			operator=(const Iterator& other);

			bool 				HasNext() const;
	const	BNetworkCookie*		Next();
	const	BNetworkCookie*		NextDomain();
	const	BNetworkCookie*		Remove();
			void				RemoveDomain();

private:
								Iterator(const BNetworkCookieJar* map);

			void 				_FindNext();

private:
	friend 	class BNetworkCookieJar;

			BNetworkCookieJar*	fCookieJar;
			PrivateIterator*	fIterator;
			BNetworkCookieList* fLastList;
			BNetworkCookieList*	fList;
	const	BNetworkCookie*		fElement;
	const	BNetworkCookie*		fLastElement;
			int32				fIndex;
};


// The copy constructor and assignment operator create new iterators for the
// same cookie jar and url. Iteration will start over.
class BNetworkCookieJar::UrlIterator {
public:
								UrlIterator(const UrlIterator& other);
								~UrlIterator();

			bool 				HasNext() const;
	const	BNetworkCookie*		Next();
	const	BNetworkCookie*		Remove();
			UrlIterator& 		operator=(const UrlIterator& other);

private:
								UrlIterator(const BNetworkCookieJar* map,
									const BUrl& url);

			void				_Initialize();
			bool				_SuperDomain();
			void 				_FindNext();
			void				_FindDomain();
			bool				_FindPath();

private:
	friend 	class BNetworkCookieJar;

			BNetworkCookieJar*	fCookieJar;
			PrivateIterator*	fIterator;
			BNetworkCookieList*	fList;
			BNetworkCookieList* fLastList;
	const	BNetworkCookie*		fElement;
	const	BNetworkCookie*		fLastElement;

			int32				fIndex;
			int32				fLastIndex;

			BUrl				fUrl;
};


} // namespace Network

} // namespace BPrivate

#endif // _B_NETWORK_COOKIE_JAR_
