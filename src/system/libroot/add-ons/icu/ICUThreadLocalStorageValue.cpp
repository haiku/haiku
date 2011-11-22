/*
 * Copyright 2011, Oliver Tappe, zooey@hirschkaefer.de.
 * Distributed under the terms of the MIT License.
 */


#include "ICUThreadLocalStorageValue.h"

#include <new>

#include "ICUConverterManager.h"


namespace BPrivate {
namespace Libroot {


ICUThreadLocalStorageValue::ICUThreadLocalStorageValue()
	: converterID(0)
{
}


ICUThreadLocalStorageValue::~ICUThreadLocalStorageValue()
{
	if (converterID != 0)
		ICUConverterManager::Instance()->DropConverter(converterID);
}


status_t
ICUThreadLocalStorageValue::GetInstanceForKey(pthread_key_t tlsKey,
	ICUThreadLocalStorageValue*& instanceOut)
{
	ICUThreadLocalStorageValue* tlsValue = NULL;
	void* value = pthread_getspecific(tlsKey);
	if (value == NULL) {
		tlsValue = new (std::nothrow) ICUThreadLocalStorageValue();
		if (tlsValue == NULL)
			return B_NO_MEMORY;
		pthread_setspecific(tlsKey, tlsValue);
	} else
		tlsValue = static_cast<ICUThreadLocalStorageValue*>(value);

	instanceOut = tlsValue;

	return B_OK;
}


}	// namespace Libroot
}	// namespace BPrivate
