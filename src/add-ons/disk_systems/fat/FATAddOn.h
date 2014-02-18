/*
 * Copyright 2015, François Revol <revol@free.fr>
 * Copyright 2007, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Copyright 2008-2012, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2012, Gerasim Troeglazov (3dEyes**), 3dEyes@gmail.com
 *
 * Distributed under the terms of the MIT License.
 */

#ifndef _FAT_ADD_ON_H
#define _FAT_ADD_ON_H

#include <DiskSystemAddOn.h>

#ifndef MAX_PATH
#define MAX_PATH 1024
#endif

class FATAddOn : public BDiskSystemAddOn {
public:
								FATAddOn();
	virtual						~FATAddOn();

	virtual	status_t			CreatePartitionHandle(
									BMutablePartition* partition,
									BPartitionHandle** handle);
	virtual	status_t			GetParameterEditor(
									B_PARAMETER_EDITOR_TYPE type,
									BPartitionParameterEditor** editor);

	virtual	bool				CanInitialize(
									const BMutablePartition* partition);
	virtual	status_t			ValidateInitialize(
									const BMutablePartition* partition,
									BString* name, const char* parameters);
	virtual	status_t			Initialize(BMutablePartition* partition,
									const char* name, const char* parameters,
									BPartitionHandle** handle);
};


class FATPartitionHandle : public BPartitionHandle {
public:
								FATPartitionHandle(
									BMutablePartition* partition);
								~FATPartitionHandle();

			status_t			Init();

	virtual	uint32				SupportedOperations(uint32 mask);
};


#endif	// _FAT_ADD_ON_H
