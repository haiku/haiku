//----------------------------------------------------------------------
//  This software is part of the OpenBeOS distribution and is covered 
//  by the OpenBeOS license.
//---------------------------------------------------------------------
#ifndef _DISK_SCANNER_ADD_ON
#define _DISK_SCANNER_ADD_ON

#include <SupportDefs.h>

struct partition_info;
struct session_info;

class BDiskScannerParameterEditor;
class BString;

// BDiskScannerPartitionAddOn
class BDiskScannerPartitionAddOn {
public:
	BDiskScannerPartitionAddOn();
	virtual ~BDiskScannerPartitionAddOn();

	virtual const char *ShortName() = 0;
	virtual const char *LongName() = 0;

	virtual BDiskScannerParameterEditor *CreateEditor(
		const session_info *sessionInfo, const char *parameters) = 0;

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
		const partition_info *partitionInfo, const char *parameters) = 0;

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
