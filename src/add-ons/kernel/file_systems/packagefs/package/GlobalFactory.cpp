/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */


#include "GlobalFactory.h"

#include <new>

#include <package/hpkg/HPKGDefsPrivate.h>


static const uint32 kMaxCachedBuffers = 32;

/*static*/ GlobalFactory* GlobalFactory::sDefaultInstance = NULL;


GlobalFactory::GlobalFactory()
	:
	fBufferCache(B_HPKG_DEFAULT_DATA_CHUNK_SIZE_ZLIB, kMaxCachedBuffers),
	fPackageDataReaderFactory(&fBufferCache)
{
}


GlobalFactory::~GlobalFactory()
{
}


/*static*/ status_t
GlobalFactory::CreateDefault()
{
	if (sDefaultInstance != NULL)
		return B_OK;

	GlobalFactory* factory = new(std::nothrow) GlobalFactory;
	if (factory == NULL)
		return B_NO_MEMORY;

	status_t error = factory->_Init();
	if (error != B_OK) {
		delete factory;
		return error;
	}

	sDefaultInstance = factory;
	return B_OK;
}


/*static*/ void
GlobalFactory::DeleteDefault()
{
	delete sDefaultInstance;
	sDefaultInstance = NULL;
}


/*static*/ GlobalFactory*
GlobalFactory::Default()
{
	return sDefaultInstance;
}


status_t
GlobalFactory::CreatePackageDataReader(BDataReader* dataReader,
	const BPackageData& data, BPackageDataReader*& _reader)
{
	return fPackageDataReaderFactory.CreatePackageDataReader(dataReader, data,
		_reader);
}


status_t
GlobalFactory::_Init()
{
	status_t error = fBufferCache.Init();
	if (error != B_OK)
		return error;

	return B_OK;
}
