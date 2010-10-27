/*
 * Copyright 2010 Haiku Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _B_NETWORK_COOKIE_JAR_H_
#define _B_NETWORK_COOKIE_JAR_H_

#include <Archivable.h>
#include <Flattenable.h>
#include <List.h>
#include <Message.h>
#include <NetworkCookie.h>
#include <String.h>
#include <Url.h>


typedef BList BNetworkCookieList;


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

			bool				AddCookie(const BNetworkCookie& cookie);
			bool				AddCookie(BNetworkCookie* cookie);			
			bool				AddCookies(
									const BNetworkCookieList& cookies);

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

			bool 				HasNext() const;
			BNetworkCookie*		Next();
			BNetworkCookie*		NextDomain();
			BNetworkCookie*		Remove();
			void				RemoveDomain();
			Iterator& 			operator=(const Iterator& other);

private:
								Iterator(const BNetworkCookieJar* map);

			void 				_FindNext();

private:
	friend 	class BNetworkCookieJar;

			BNetworkCookieJar*	fCookieJar;
			PrivateIterator*	fIterator;
			BNetworkCookieList* fLastList;
			BNetworkCookieList*	fList;
			BNetworkCookie*		fElement;
			BNetworkCookie*		fLastElement;
			int32				fIndex;
};


class BNetworkCookieJar::UrlIterator {
public:
								UrlIterator(const UrlIterator& other);
								~UrlIterator();

			bool 				HasNext() const;
			BNetworkCookie*		Next();
			BNetworkCookie*		Remove();
			UrlIterator& 		operator=(const UrlIterator& other);

private:
								UrlIterator(const BNetworkCookieJar* map,
									const BUrl& url);

			bool				_SupDomain();
			void 				_FindNext();
			void				_FindDomain();
			bool				_FindPath();

private:
	friend 	class BNetworkCookieJar;

			BNetworkCookieJar*	fCookieJar;
			PrivateIterator*	fIterator;
			BNetworkCookieList*	fList;
			BNetworkCookieList* fLastList;
			BNetworkCookie*		fElement;
			BNetworkCookie*		fLastElement;
			
			int32				fIndex;
			int32				fLastIndex;
			
			BUrl				fUrl;
};

#endif // _B_NETWORK_COOKIE_JAR_
