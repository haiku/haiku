/*
 * Copyright 2009-2013, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef GLOBAL_FACTORY_H
#define GLOBAL_FACTORY_H


#include <package/hpkg/v1/PackageDataReader.h>

#include "BlockBufferPoolKernel.h"
#include "PackageData.h"


using BPackageKit::BHPKG::BBufferPool;
using BPackageKit::BHPKG::BDataReader;
using BPackageKit::BHPKG::BPackageData;
using BPackageKit::BHPKG::BAbstractBufferedDataReader;

typedef BPackageKit::BHPKG::V1::BPackageDataReaderFactory
	BPackageDataReaderFactoryV1;


class GlobalFactory {
private:
								GlobalFactory();
								~GlobalFactory();

public:
	static	status_t			CreateDefault();
	static	void				DeleteDefault();
	static	GlobalFactory*		Default();

			status_t			CreatePackageDataReader(BDataReader* dataReader,
									const PackageDataV1& data,
									BAbstractBufferedDataReader*& _reader);

private:
			status_t			_Init();

private:
	static	GlobalFactory*		sDefaultInstance;

			BlockBufferPoolKernel fBufferPool;
			BPackageDataReaderFactoryV1 fPackageDataReaderFactory;
};

#endif	// GLOBAL_FACTORY_H
