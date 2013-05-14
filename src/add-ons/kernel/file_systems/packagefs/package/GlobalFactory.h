/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef GLOBAL_FACTORY_H
#define GLOBAL_FACTORY_H


#include <package/hpkg/PackageDataReader.h>
#include <package/hpkg/v1/PackageDataReader.h>

#include "BlockBufferPoolKernel.h"


using BPackageKit::BHPKG::BDataReader;
using BPackageKit::BHPKG::BPackageData;
using BPackageKit::BHPKG::BAbstractBufferedDataReader;

typedef BPackageKit::BHPKG::V1::BPackageDataReaderFactory
	BPackageDataReaderFactoryV1;
typedef BPackageKit::BHPKG::BPackageDataReaderFactory
	BPackageDataReaderFactoryV2;

class PackageData;


class GlobalFactory {
private:
								GlobalFactory();
								~GlobalFactory();

public:
	static	status_t			CreateDefault();
	static	void				DeleteDefault();
	static	GlobalFactory*		Default();

			status_t			CreatePackageDataReader(BDataReader* dataReader,
									const PackageData& data,
									BAbstractBufferedDataReader*& _reader);

private:
			status_t			_Init();

private:
	static	GlobalFactory*		sDefaultInstance;

			BlockBufferPoolKernel fBufferPool;
			BPackageDataReaderFactoryV1 fPackageDataReaderFactoryV1;
			BPackageDataReaderFactoryV2 fPackageDataReaderFactoryV2;
};

#endif	// GLOBAL_FACTORY_H
