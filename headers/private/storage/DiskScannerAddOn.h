//----------------------------------------------------------------------
//  This software is part of the Haiku distribution and is covered
//  by the MIT License.
//---------------------------------------------------------------------
#ifndef _DISK_SCANNER_ADD_ON
#define _DISK_SCANNER_ADD_ON

#include <SupportDefs.h>

class BDiskScannerParameterEditor;
class BPartition;
class BSession;
class BString;

// BDiskScannerPartitionAddOn
class BDiskScannerPartitionAddOn {
public:
	BDiskScannerPartitionAddOn();
	virtual ~BDiskScannerPartitionAddOn();

	virtual const char *ShortName() = 0;
	virtual const char *LongName() = 0;

	virtual BDiskScannerParameterEditor *CreateEditor(const BSession *session,
		const char *parameters) = 0;

private:
	virtual	void _ReservedDiskScannerPartitionAddOn1();
	virtual	void _ReservedDiskScannerPartitionAddOn2();
	virtual	void _ReservedDiskScannerPartitionAddOn3();
	virtual	void _ReservedDiskScannerPartitionAddOn4();
	virtual	void _ReservedDiskScannerPartitionAddOn5();

	uint32 _reserved[8];
};

// BDiskScannerFSAddOn
class BDiskScannerFSAddOn {
public:
	BDiskScannerFSAddOn();
	virtual ~BDiskScannerFSAddOn();

	virtual const char *ShortName() = 0;
	virtual const char *LongName() = 0;

	virtual BDiskScannerParameterEditor *CreateEditor(
		const BPartition *partition, const char *parameters) = 0;

private:
	virtual	void _ReservedDiskScannerFSAddOn1();
	virtual	void _ReservedDiskScannerFSAddOn2();
	virtual	void _ReservedDiskScannerFSAddOn3();
	virtual	void _ReservedDiskScannerFSAddOn4();
	virtual	void _ReservedDiskScannerFSAddOn5();

	uint32 _reserved[8];
};

// BDiskScannerParameterEditor
class BDiskScannerParameterEditor {
public:
	BDiskScannerParameterEditor();
	virtual ~BDiskScannerParameterEditor();

	virtual BView *View();
	virtual bool EditingDone();
	virtual status_t GetParameters(BString *parameters);

private:
	virtual	void _ReservedDiskScannerParameterEditor1();
	virtual	void _ReservedDiskScannerParameterEditor2();
	virtual	void _ReservedDiskScannerParameterEditor3();
	virtual	void _ReservedDiskScannerParameterEditor4();
	virtual	void _ReservedDiskScannerParameterEditor5();

	uint32 _reserved[8];
};

// partition add-ons
extern "C" BDiskScannerPartitionAddOn *create_ds_partition_add_on();

// fs add-ons
extern "C" BDiskScannerFSAddOn *create_ds_fs_add_on();

#endif	// _DISK_SCANNER_ADD_ON
