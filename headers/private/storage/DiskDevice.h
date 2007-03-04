//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------

#ifndef _DISK_DEVICE_H
#define _DISK_DEVICE_H

#include <Partition.h>

struct user_disk_device_data;

class BDiskDevice : public BPartition {
public:
	BDiskDevice();
	virtual ~BDiskDevice();

	bool HasMedia() const;
	bool IsRemovableMedia() const;
	bool IsReadOnlyMedia() const;
	bool IsWriteOnceMedia() const;

	status_t Eject(bool update = false);

	status_t SetTo(partition_id id);
	status_t Update(bool *updated = NULL);
	void Unset();
	status_t InitCheck() const;

	virtual status_t GetPath(BPath *path) const;

	bool IsModified() const;
	status_t PrepareModifications();
	status_t CommitModifications(bool synchronously = true,
		BMessenger progressMessenger = BMessenger(),
		bool receiveCompleteProgressUpdates = true);
	status_t CancelModifications();

private:
	friend class BDiskDeviceList;
	friend class BDiskDeviceRoster;

	BDiskDevice(const BDiskDevice &);
	BDiskDevice &operator=(const BDiskDevice &);

	static status_t _GetData(partition_id id, bool deviceOnly, bool shadow,
							 size_t neededSize, user_disk_device_data **data);

	status_t _SetTo(partition_id id, bool deviceOnly, bool shadow,
					size_t neededSize);
	status_t _SetTo(user_disk_device_data *data);
	status_t _Update(bool shadow, bool *updated);
	status_t _Update(user_disk_device_data *data, bool *updated);

	virtual bool _AcceptVisitor(BDiskDeviceVisitor *visitor, int32 level);

	user_disk_device_data	*fDeviceData;
};

#endif	// _DISK_DEVICE_H
