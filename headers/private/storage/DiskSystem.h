/*
 * Copyright 2003-2008, Haiku Inc.
 * Distributed under the terms of the MIT License.
 */
#ifndef _DISK_SYSTEM_H
#define _DISK_SYSTEM_H


#include <DiskDeviceDefs.h>
#include <String.h>

class BPartition;
class BString;
struct user_disk_system_info;


class BDiskSystem {
public:
							BDiskSystem();
							BDiskSystem(const BDiskSystem& other);
							~BDiskSystem();

			status_t		InitCheck() const;

			const char*		Name() const;
			const char*		ShortName() const;
			const char*		PrettyName() const;

			bool			SupportsDefragmenting(bool* whileMounted) const;
			bool			SupportsRepairing(bool checkOnly,
								bool* whileMounted) const;
			bool			SupportsResizing(bool* whileMounted) const;
			bool			SupportsResizingChild() const;
			bool			SupportsMoving(bool* whileMounted) const;
			bool			SupportsMovingChild() const;
			bool			SupportsName() const;
			bool			SupportsContentName() const;
			bool			SupportsSettingName() const;
			bool			SupportsSettingContentName(
								bool* whileMounted) const;
			bool			SupportsSettingType() const;
			bool			SupportsSettingParameters() const;
			bool			SupportsSettingContentParameters(
								bool* whileMounted) const;
			bool			SupportsCreatingChild() const;
			bool			SupportsDeletingChild() const;
			bool			SupportsInitializing() const;
			bool			SupportsWriting() const;

			status_t		GetTypeForContentType(const char* contentType,
								BString* type) const;

			bool			IsPartitioningSystem() const;
			bool			IsFileSystem() const;

			BDiskSystem&	operator=(const BDiskSystem& other);

private:
			status_t		_SetTo(disk_system_id id);
			status_t		_SetTo(const user_disk_system_info* info);
			void			_Unset();

private:
	friend class BDiskDeviceRoster;
	friend class BPartition;

	disk_system_id			fID;
	BString					fName;
	BString					fShortName;
	BString					fPrettyName;
	uint32					fFlags;
};

#endif	// _DISK_SYSTEM_H
