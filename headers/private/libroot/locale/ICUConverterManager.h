/*
 * Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ICU_CONVERTER_MANAGER_H
#define _ICU_CONVERTER_MANAGER_H


#include <pthread.h>

#include <map>

#include <unicode/ucnv.h>

#include <SupportDefs.h>

#include <locks.h>
#include <Referenceable.h>
#include <util/DoublyLinkedList.h>
//#include <util/OpenHashTable.h>

#include "ICUThreadLocalStorageValue.h"


namespace BPrivate {
namespace Libroot {


class ICUConverterInfo : public BReferenceable {
public:
								ICUConverterInfo(UConverter* converter,
									const char* charset, ICUConverterID id);
	virtual						~ICUConverterInfo();

			UConverter*			Converter() const
									{ return fConverter; }

			const char*			Charset() const
									{ return fCharset; }

			ICUConverterID		ID() const
									{ return fID; }

private:
			UConverter*			fConverter;
			char				fCharset[UCNV_MAX_CONVERTER_NAME_LENGTH];
			ICUConverterID		fID;
};


typedef BReference<ICUConverterInfo> ICUConverterRef;


class ICUConverterManager {
public:
								ICUConverterManager();
								~ICUConverterManager();

			status_t			CreateConverter(const char* charset,
									ICUConverterRef& converterRefOut,
									ICUConverterID& idOut);

			status_t			GetConverter(ICUConverterID id,
									ICUConverterRef& converterRefOut);

			status_t			DropConverter(ICUConverterID id);

	static	ICUConverterManager*	Instance();

private:
	static	void				_CreateInstance();

	static	ICUConverterManager*	sInstance;

	static	const size_t		skMaxConvertersPerProcess = 1024;

private:
			class LinkedConverterInfo
				:
				public ICUConverterInfo,
				public DoublyLinkedListLinkImpl<LinkedConverterInfo>
			{
			public:
				LinkedConverterInfo(UConverter* converter, const char* charset,
					ICUConverterID id)
					:
					ICUConverterInfo(converter, charset, id)
				{
				}
			};
			typedef	std::map<ICUConverterID, LinkedConverterInfo*> ConverterMap;
			typedef DoublyLinkedList<LinkedConverterInfo> ConverterList;

private:
			ConverterMap		fConverterMap;
			ConverterList		fLRUConverters;
			mutex				fMutex;
			ICUConverterID		fNextConverterID;
};


}	// namespace Libroot
}	// namespace BPrivate


#endif	// _ICU_CONVERTER_MANAGER_H
