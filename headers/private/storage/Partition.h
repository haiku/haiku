//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _PARTITION_H
#define _PARTITION_H

#include <fs_device.h>
#include <Mime.h>
#include <ObjectList.h>
#include <Point.h>
#include <String.h>

class BDiskDevice;
class BSession;
class BVolume;

class BPartition {
public:
	BPartition();
	~BPartition();

	BSession *Session() const;
	BDiskDevice *Device() const;

	off_t Offset() const;
	off_t Size() const;
	int32 BlockSize() const;
	int32 Index() const;

	uint32 Flags() const;
	bool IsHidden() const;
	bool IsVirtual() const;
	bool IsEmpty() const;

	const char *Name() const;
	const char *Type() const;
	const char *FileSystemShortName() const;
	const char *FileSystemLongName() const;
	const char *VolumeName() const;

	uint32 FileSystemFlags() const;

	bool IsMounted() const;

	int32 UniqueID() const;

	status_t GetVolume(BVolume *volume) const;

	status_t GetIcon(BBitmap *icon, icon_size which) const;
 
	status_t Mount(uint32 mountflags = 0, const char *parameters = NULL);
	status_t Unmount();

	status_t GetInitializationParameters(const char *fileSystem,
										 BPoint dialogCenter,
										 BString *parameters,
										 bool *cancelled = NULL);
	status_t Initialize(const char *fileSystem, const char *parameters);
	status_t Initialize(const char *fileSystem = "bfs",
						BPoint dialogCenter = BPoint(-1, -1),
						bool *cancelled = NULL);

	static status_t GetFileSystemList(BObjectList<BString*> *list);
	
private:
	BPartition(const BPartition &);
	BPartition &operator=(const BPartition &);

private:
	BSession				*fSession;
// TODO: Also contains the device name, which we can get from Device() anyway.
//		 We could either remove it from extended_partition_info or list
//		 the individual fields here.
	extended_partition_info	fInfo;
	int32					fUniqueID;
	dev_t					fVolumeID;
	node_ref				fMountPoint;
};

#endif	// _PARTITION_H
