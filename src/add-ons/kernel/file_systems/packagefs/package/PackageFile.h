/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_FILE_H
#define PACKAGE_FILE_H


#include "PackageData.h"
#include "PackageLeafNode.h"


class PackageFile : public PackageLeafNode {
public:
	static	void*				operator new(size_t size);
	static	void				operator delete(void* block);

								PackageFile(Package* package, mode_t mode,
									const PackageData& data);
	virtual						~PackageFile();

	virtual	status_t			VFSInit(dev_t deviceID, ino_t nodeID);
	virtual	void				VFSUninit();

	virtual	off_t				FileSize() const;

	virtual	status_t			Read(off_t offset, void* buffer,
									size_t* bufferSize);
	virtual	status_t			Read(io_request* request);

private:
			struct IORequestOutput;
			struct DataAccessor;

private:
			PackageData			fData;
			DataAccessor*		fDataAccessor;
};


#endif	// PACKAGE_FILE_H
