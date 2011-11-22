/*
 * Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUConverterManager.h"

#include <pthread.h>
#include <string.h>

#include <new>


namespace BPrivate {
namespace Libroot {


static pthread_once_t sManagerInitOnce = PTHREAD_ONCE_INIT;


ICUConverterInfo::ICUConverterInfo(UConverter* converter, const char* charset,
	ICUConverterID id)
	:
	fConverter(converter),
	fID(id)
{
	strlcpy(fCharset, charset, sizeof(fCharset));
}


ICUConverterInfo::~ICUConverterInfo()
{
	if (fConverter != NULL)
		ucnv_close(fConverter);
}


ICUConverterManager* ICUConverterManager::sInstance = NULL;


ICUConverterManager::ICUConverterManager()
	:
	fNextConverterID(1)
{
	mutex_init(&fMutex, "ConverterManagerMutex");
}


ICUConverterManager::~ICUConverterManager()
{
	ConverterMap::iterator iter;
	for (iter = fConverterMap.begin(); iter != fConverterMap.end(); ++iter)
		iter->second->ReleaseReference();

	mutex_destroy(&fMutex);
}


status_t
ICUConverterManager::CreateConverter(const char* charset,
	ICUConverterRef& converterRefOut, ICUConverterID& idOut)
{
	MutexLocker lock(fMutex);
	if (!lock.IsLocked())
		return B_ERROR;

	UErrorCode icuStatus = U_ZERO_ERROR;
	UConverter* icuConverter = ucnv_open(charset, &icuStatus);
	if (icuConverter == NULL)
		return B_NAME_NOT_FOUND;

	LinkedConverterInfo* converterInfo = new (std::nothrow) LinkedConverterInfo(
		icuConverter, charset, fNextConverterID);
	if (converterInfo == NULL) {
		ucnv_close(icuConverter);
		return B_NO_MEMORY;
	}
	ICUConverterRef converterRef(converterInfo, true);

	// setup the new converter to stop upon any errors
	icuStatus = U_ZERO_ERROR;
	ucnv_setToUCallBack(icuConverter, UCNV_TO_U_CALLBACK_STOP, NULL, NULL, NULL,
		&icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;
	icuStatus = U_ZERO_ERROR;
	ucnv_setFromUCallBack(icuConverter, UCNV_FROM_U_CALLBACK_STOP, NULL, NULL,
		NULL, &icuStatus);
	if (!U_SUCCESS(icuStatus))
		return B_ERROR;

	// ok, we've got the converter, add it to our map
	try {
		if (fConverterMap.size() >= skMaxConvertersPerProcess) {
			// make room by dropping the least recently used converter
			LinkedConverterInfo* leastUsedConverter = fLRUConverters.Head();
			fLRUConverters.Remove(leastUsedConverter);
			fConverterMap.erase(leastUsedConverter->ID());
			leastUsedConverter->ReleaseReference();
		}
		fConverterMap[fNextConverterID] = converterInfo;
		fLRUConverters.Insert(converterInfo);
	} catch (...) {
		return B_NO_MEMORY;
	}

	converterRefOut = converterRef;
	idOut = fNextConverterID++;

	converterRef.Detach();

	return B_OK;
}


status_t
ICUConverterManager::GetConverter(ICUConverterID id,
	ICUConverterRef& converterRefOut)
{
	MutexLocker lock(fMutex);
	if (!lock.IsLocked())
		return B_ERROR;

	ConverterMap::iterator iter = fConverterMap.find(id);
	if (iter == fConverterMap.end())
		return B_NAME_NOT_FOUND;

	converterRefOut.SetTo(iter->second);

	return B_OK;
}


status_t
ICUConverterManager::DropConverter(ICUConverterID id)
{
	MutexLocker lock(fMutex);
	if (!lock.IsLocked())
		return B_ERROR;

	ConverterMap::iterator iter = fConverterMap.find(id);
	if (iter == fConverterMap.end())
		return B_NAME_NOT_FOUND;

	fLRUConverters.Remove(iter->second);
	fConverterMap.erase(iter);
	iter->second->ReleaseReference();

	return B_OK;
}


/*static*/ ICUConverterManager*
ICUConverterManager::Instance()
{
	if (sInstance != NULL)
		return sInstance;

	pthread_once(&sManagerInitOnce,
		&BPrivate::Libroot::ICUConverterManager::_CreateInstance);

	return sInstance;
}


/*static*/ void
ICUConverterManager::_CreateInstance()
{
	sInstance = new (std::nothrow) ICUConverterManager;
}


}	// namespace Libroot
}	// namespace BPrivate
