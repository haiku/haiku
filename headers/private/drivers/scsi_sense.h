/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 	Jérôme Duval
 * 
 * Generated based on http://www.t10.org/lists/asc-num.txt 20130919
 *  and http://www.t10.org/lists/2sensekey.htm
 */


// "standardized" error code to simplify handling of scsi errors
typedef enum {
	err_act_ok,					// executed successfully
	err_act_retry,				// failed, retry 3 times
	err_act_fail,				// failed, don't retry
	err_act_many_retries,		// failed, retry multiple times (currently 10 times)
	err_act_start,				// device requires a "start" command
	err_act_invalid_req,		// request is invalid
	err_act_tur,				// device requires a "test unit ready" command
	err_act_rescan				// device requires a rescan
} err_act;


typedef struct _scsi_sense_key_table
{
	uint8	key;
	const char *	label;
	err_act			action;
	status_t		err;
} scsi_sense_key_table;


scsi_sense_key_table sSCSISenseKeyTable[] =
{
	{ 0x00, "No sense", err_act_ok, B_OK },
	{ 0x01, "Recovered error", err_act_ok, B_OK },
	{ 0x02, "Not ready", err_act_ok, B_DEV_NOT_READY },
	{ 0x03, "Medium error", err_act_fail, B_DEV_RECALIBRATE_ERROR },
	{ 0x04, "Hardware error", err_act_fail, B_DEV_SEEK_ERROR },
	{ 0x05, "Illegal request", err_act_fail, B_DEV_INVALID_IOCTL },
	{ 0x06, "Unit attention", err_act_fail, B_DEV_NOT_READY },
	{ 0x07, "Data protect", err_act_fail, B_READ_ONLY_DEVICE },
	{ 0x08, "Blank check", err_act_fail, B_DEV_UNREADABLE },
	{ 0x09, "Vendor specific", err_act_fail, B_ERROR },
	{ 0x0A, "Copy aborted", err_act_fail, B_ERROR },
	{ 0x0B, "Aborted command", err_act_fail, B_CANCELED },
	{ 0x0D, "Volume overflow", err_act_fail, B_DEV_SEEK_ERROR },
	{ 0x0E, "Miscompare", err_act_ok, B_OK },
	{ 0x0F, "Completed", err_act_ok, B_OK },
};


typedef struct _scsi_sense_asc_table
{
	unsigned short	asc_ascq;
	const char *	label;
	err_act			action;
	status_t		err;
} scsi_sense_asc_table;


scsi_sense_asc_table sSCSISenseAscTable[] =
{
	{ 0x0000, "No additional sense information", err_act_ok, B_OK },	// DTLPWROMAEBKVF
	{ 0x0001, "Filemark detected", err_act_fail, B_ERROR },	// T
	{ 0x0002, "End-of-partition/medium detected", err_act_fail, B_ERROR },	// T
	{ 0x0003, "Setmark detected", err_act_fail, B_ERROR },	// T
	{ 0x0004, "Beginning-of-partition/medium detected", err_act_fail, B_ERROR },	// T
	{ 0x0005, "End-of-data detected", err_act_fail, B_ERROR },	// TL
	{ 0x0006, "I/O process terminated", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x0007, "Programmable early warning detected", err_act_fail, B_ERROR },	// T
	{ 0x0011, "Audio play operation in progress", err_act_fail, B_DEV_NOT_READY },	// R
	{ 0x0012, "Audio play operation paused", err_act_ok, B_OK },	// R
	{ 0x0013, "Audio play operation successfully completed", err_act_ok, B_OK },	// R
	{ 0x0014, "Audio play operation stopped due to error", err_act_fail, B_ERROR },	// R
	{ 0x0015, "No current audio status to return", err_act_ok, B_OK },	// R
	{ 0x0016, "Operation in progress", err_act_fail, B_DEV_NOT_READY },	// DTLPWROMAEBKVF
	{ 0x0017, "Cleaning requested", err_act_fail, B_ERROR },	// DTL WROMAEBKVF
	{ 0x0018, "Erase operation in progress", err_act_fail, B_ERROR },	// T
	{ 0x0019, "Locate operation in progress", err_act_fail, B_ERROR },	// T
	{ 0x001A, "Rewind operation in progress", err_act_fail, B_ERROR },	// T
	{ 0x001B, "Set capacity operation in progress", err_act_fail, B_ERROR },	// T
	{ 0x001C, "Verify operation in progress", err_act_fail, B_ERROR },	// T
	{ 0x001D, "ATA pass through information available", err_act_fail, B_ERROR },	// DT        B
	{ 0x001E, "Conflicting SA creation request", err_act_fail, B_ERROR },	// DT   R MAEBKV
	{ 0x001F, "Logical unit transitioning to another power condition", err_act_fail, B_ERROR },	// DT        B
	{ 0x0020, "Extended copy information available", err_act_fail, B_ERROR },	// DT P      B
	{ 0x0100, "No index/sector signal", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x0200, "No seek complete", err_act_fail, B_ERROR },	// D   WRO   BK
	{ 0x0300, "Peripheral device write fault", err_act_fail, B_ERROR },	// DTL W O   BK
	{ 0x0301, "No write current", err_act_fail, B_ERROR },	// T
	{ 0x0302, "Excessive write errors", err_act_fail, B_ERROR },	// T
	{ 0x0400, "Logical unit not ready, cause not reportable", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x0401, "Logical unit is in process of becoming ready", err_act_many_retries, B_DEV_NOT_READY },	// DTLPWROMAEBKVF
	{ 0x0402, "Logical unit not ready, initializing command required", err_act_start, B_NO_INIT },	// DTLPWROMAEBKVF
	{ 0x0403, "Logical unit not ready, manual intervention required", err_act_fail, B_DEV_NOT_READY },	// DTLPWROMAEBKVF
	{ 0x0404, "Logical unit not ready, format in progress", err_act_fail, B_DEV_NOT_READY },	// DTL  RO   B
	{ 0x0405, "Logical unit not ready, rebuild in progress", err_act_fail, B_DEV_NOT_READY },	// DT  W O A BK F
	{ 0x0406, "Logical unit not ready, recalculation in progress", err_act_fail, B_DEV_NOT_READY },	// DT  W O A BK
	{ 0x0407, "Logical unit not ready, operation in progress", err_act_fail, B_DEV_NOT_READY },	// DTLPWROMAEBKVF
	{ 0x0408, "Logical unit not ready, long write in progress", err_act_fail, B_DEV_NOT_READY },	// R
	{ 0x0409, "Logical unit not ready, self-test in progress", err_act_fail, B_DEV_NOT_READY },	// DTLPWROMAEBKVF
	{ 0x040A, "Logical unit not accessible, asymmetric access state transition", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x040B, "Logical unit not accessible, target port in standby state", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x040C, "Logical unit not accessible, target port in unavailable state", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x040D, "Logical unit not ready, structure check required", err_act_fail, B_ERROR },	// F
	{ 0x0410, "Logical unit not ready, auxiliary memory not accessible", err_act_fail, B_ERROR },	// DT  WROM  B
	{ 0x0411, "Logical unit not ready, notify (enable spinup) required", err_act_tur, B_DEV_NOT_READY },	// DT  WRO AEB VF
	{ 0x0412, "Logical unit not ready, offline", err_act_fail, B_ERROR },	// M    V
	{ 0x0413, "Logical unit not ready, SA creation in progress", err_act_fail, B_ERROR },	// DT   R MAEBKV
	{ 0x0414, "Logical unit not ready, space allocation in progress", err_act_fail, B_ERROR },	// D         B
	{ 0x0415, "Logical unit not ready, robotics disabled", err_act_fail, B_ERROR },	// M
	{ 0x0416, "Logical unit not ready, configuration required", err_act_fail, B_ERROR },	// M
	{ 0x0417, "Logical unit not ready, calibration required", err_act_fail, B_ERROR },	// M
	{ 0x0418, "Logical unit not ready, a door is open", err_act_fail, B_ERROR },	// M
	{ 0x0419, "Logical unit not ready, operating in sequential mode", err_act_fail, B_ERROR },	// M
	{ 0x041A, "Logical unit not ready, START STOP unit command in progress", err_act_fail, B_ERROR },	// D         B
	{ 0x041B, "Logical unit not ready, sanitize in progress", err_act_fail, B_ERROR },	// D         B
	{ 0x041C, "Logical unit not ready, additional power use not yet granted", err_act_fail, B_ERROR },	// DT     MAEB
	{ 0x041D, "Logical unit not ready, configuration in progress", err_act_fail, B_ERROR },	// D
	{ 0x041E, "Logical unit not ready, microcode activation required", err_act_fail, B_ERROR },	// D
	{ 0x0500, "Logical unit does not respond to selection", err_act_fail, B_ERROR },	// DTL WROMAEBKVF
	{ 0x0600, "No reference position found", err_act_fail, B_ERROR },	// D   WROM  BK
	{ 0x0700, "Multiple peripheral devices selected", err_act_fail, B_ERROR },	// DTL WROM  BK
	{ 0x0800, "Logical unit communication failure", err_act_fail, B_ERROR },	// DTL WROMAEBKVF
	{ 0x0801, "Logical unit communication time-out", err_act_fail, B_ERROR },	// DTL WROMAEBKVF
	{ 0x0802, "Logical unit communication parity error", err_act_fail, B_ERROR },	// DTL WROMAEBKVF
	{ 0x0803, "Logical unit communication CRC error (Ultra-DMA/32)", err_act_fail, B_ERROR },	// DT   ROM  BK
	{ 0x0804, "Unreachable copy target", err_act_fail, B_ERROR },	// DTLPWRO    K
	{ 0x0900, "Track following error", err_act_fail, B_ERROR },	// DT  WRO   B
	{ 0x0901, "Tracking servo failure", err_act_fail, B_ERROR },	// WRO    K
	{ 0x0902, "Focus servo failure", err_act_fail, B_ERROR },	// WRO    K
	{ 0x0903, "Spindle servo failure", err_act_fail, B_ERROR },	// WRO
	{ 0x0904, "Head select fault", err_act_fail, B_ERROR },	// DT  WRO   B
	{ 0x0A00, "Error log overflow", err_act_fail, ENOSPC },	// DTLPWROMAEBKVF
	{ 0x0B00, "Warning", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x0B01, "Warning - specified temperature exceeded", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x0B02, "Warning - enclosure degraded", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x0B03, "Warning - background self-test failed", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x0B04, "Warning - background pre-scan detected medium error", err_act_fail, B_ERROR },	// DTLPWRO AEBKVF
	{ 0x0B05, "Warning - background medium scan detected medium error", err_act_fail, B_ERROR },	// DTLPWRO AEBKVF
	{ 0x0B06, "Warning - non-volatile cache now volatile", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x0B07, "Warning - degraded power to non-volatile cache", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x0B08, "Warning - power loss expected", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x0B09, "Warning - device statistics notification active", err_act_fail, B_ERROR },	// D
	{ 0x0C00, "Write error", err_act_fail, B_ERROR },	// T   R
	{ 0x0C01, "Write error - recovered with auto reallocation", err_act_ok, B_OK },	// K
	{ 0x0C02, "Write error - auto reallocation failed", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x0C03, "Write error - recommend reassignment", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x0C04, "Compression check miscompare error", err_act_fail, B_ERROR },	// DT  W O   B
	{ 0x0C05, "Data expansion occurred during compression", err_act_fail, B_ERROR },	// DT  W O   B
	{ 0x0C06, "Block not compressible", err_act_fail, B_ERROR },	// DT  W O   B
	{ 0x0C07, "Write error - recovery needed", err_act_fail, B_ERROR },	// R
	{ 0x0C08, "Write error - recovery failed", err_act_fail, B_ERROR },	// R
	{ 0x0C09, "Write error - loss of streaming", err_act_fail, B_ERROR },	// R
	{ 0x0C0A, "Write error - padding blocks added", err_act_fail, B_ERROR },	// R
	{ 0x0C0B, "Auxiliary memory write error", err_act_fail, B_ERROR },	// DT  WROM  B
	{ 0x0C0C, "Write error - unexpected unsolicited data", err_act_fail, B_ERROR },	// DTLPWRO AEBKVF
	{ 0x0C0D, "Write error - not enough unsolicited data", err_act_fail, B_ERROR },	// DTLPWRO AEBKVF
	{ 0x0C0E, "Multiple write errors", err_act_fail, B_ERROR },	// DT  W O   BK
	{ 0x0C0F, "Defects in error window", err_act_fail, B_ERROR },	// R
	{ 0x0D00, "Error detected by third party temporary initiator", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x0D01, "Third party device failure", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x0D02, "Copy target device not reachable", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x0D03, "Incorrect copy target device type", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x0D04, "Copy target device data underrun", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x0D05, "Copy target device data overrun", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x0E00, "Invalid information unit", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x0E01, "Information unit too short", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x0E02, "Information unit too long", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x0E03, "Invalid field in command information unit", err_act_fail, B_ERROR },	// DT P R MAEBK F
	{ 0x1000, "ID CRC or ECC error", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x1001, "Logical block guard check failed", err_act_fail, B_ERROR },	// DT  W O
	{ 0x1002, "Logical block application tag check failed", err_act_fail, B_ERROR },	// DT  W O
	{ 0x1003, "Logical block reference tag check failed", err_act_fail, B_ERROR },	// DT  W O
	{ 0x1004, "Logical block protection error on recover buffered data", err_act_fail, B_ERROR },	// T
	{ 0x1005, "Logical block protection method error", err_act_fail, B_ERROR },	// T
	{ 0x1100, "Unrecovered read error", err_act_fail, B_IO_ERROR },	// DT  WRO   BK
	{ 0x1101, "Read retries exhausted", err_act_fail, B_IO_ERROR },	// DT  WRO   BK
	{ 0x1102, "Error too long to correct", err_act_fail, B_IO_ERROR },	// DT  WRO   BK
	{ 0x1103, "Multiple read errors", err_act_fail, B_IO_ERROR },	// DT  W O   BK
	{ 0x1104, "Unrecovered read error - auto reallocate failed", err_act_fail, B_IO_ERROR },	// D   W O   BK
	{ 0x1105, "L-EC uncorrectable error", err_act_fail, B_IO_ERROR },	// WRO   B
	{ 0x1106, "CIRC unrecovered error", err_act_fail, B_IO_ERROR },	// WRO   B
	{ 0x1107, "Data re-synchronization error", err_act_fail, B_ERROR },	// W O   B
	{ 0x1108, "Incomplete block read", err_act_fail, B_ERROR },	// T
	{ 0x1109, "No gap found", err_act_fail, B_ERROR },	// T
	{ 0x110A, "Miscorrected error", err_act_fail, B_ERROR },	// DT    O   BK
	{ 0x110B, "Unrecovered read error - recommend reassignment", err_act_fail, B_IO_ERROR },	// D   W O   BK
	{ 0x110C, "Unrecovered read error - recommend rewrite the data", err_act_fail, B_IO_ERROR },	// D   W O   BK
	{ 0x110D, "De-compression CRC error", err_act_fail, B_ERROR },	// DT  WRO   B
	{ 0x110E, "Cannot decompress using declared algorithm", err_act_fail, B_ERROR },	// DT  WRO   B
	{ 0x110F, "Error readingUPC/EAN number", err_act_fail, B_ERROR },	// R
	{ 0x1110, "Error reading ISRC number", err_act_fail, B_ERROR },	// R
	{ 0x1111, "Read error - loss of streaming", err_act_fail, B_ERROR },	// R
	{ 0x1112, "Auxiliary memory read error", err_act_fail, B_ERROR },	// DT  WROM  B
	{ 0x1113, "Read error - failed retransmission request", err_act_fail, B_ERROR },	// DTLPWRO AEBKVF
	{ 0x1114, "Read error - LBA marked bad by application client", err_act_fail, B_ERROR },	// D
	{ 0x1115, "Write after sanitize required", err_act_fail, B_ERROR },	// D
	{ 0x1200, "Address mark not found for ID field", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x1300, "Address mark not found for data field", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x1400, "Recorded entity not found", err_act_fail, B_ERROR },	// DTL WRO   BK
	{ 0x1401, "Record not found", err_act_fail, B_ERROR },	// DT  WRO   BK
	{ 0x1402, "Filemark or setmark not found", err_act_fail, B_ERROR },	// T
	{ 0x1403, "End-of-data not found", err_act_fail, B_ERROR },	// T
	{ 0x1404, "Block sequence error", err_act_fail, B_ERROR },	// T
	{ 0x1405, "Record not found - recommend reassignment", err_act_fail, B_ERROR },	// DT  W O   BK
	{ 0x1406, "Record not found - data auto-reallocated", err_act_fail, B_ERROR },	// DT  W O   BK
	{ 0x1407, "Locate operation failure", err_act_fail, B_ERROR },	// T
	{ 0x1500, "Random positioning error", err_act_fail, B_ERROR },	// DTL WROM  BK
	{ 0x1501, "Mechanical positioning error", err_act_fail, B_ERROR },	// DTL WROM  BK
	{ 0x1502, "Positioning error detected by read of medium", err_act_fail, B_ERROR },	// DT  WRO   BK
	{ 0x1600, "Data synchronization mark error", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x1601, "Data sync error - data rewritten", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x1602, "Data sync error - recommend rewrite", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x1603, "Data sync error - data auto-reallocated", err_act_ok, B_OK },	// D   W O   BK
	{ 0x1604, "Data sync error - recommend reassignment", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x1700, "Recovered data with no error correction applied", err_act_ok, B_OK },	// DT  WRO   BK
	{ 0x1701, "Recovered data with retries", err_act_ok, B_OK },	// DT  WRO   BK
	{ 0x1702, "Recovered data with positive head offset", err_act_ok, B_OK },	// DT  WRO   BK
	{ 0x1703, "Recovered data with negative head offset", err_act_ok, B_OK },	// DT  WRO   BK
	{ 0x1704, "Recovered data with retries and/or CIRC applied", err_act_ok, B_OK },	// WRO   B
	{ 0x1705, "Recovered data using previous sector ID", err_act_ok, B_OK },	// D   WRO   BK
	{ 0x1706, "Recovered data without ECC - data auto-reallocated", err_act_ok, B_OK },	// D   W O   BK
	{ 0x1707, "Recovered data without ECC - recommend reassignment", err_act_ok, B_OK },	// D   WRO   BK
	{ 0x1708, "Recovered data without ECC - recommend rewrite", err_act_ok, B_OK },	// D   WRO   BK
	{ 0x1709, "Recovered data without ECC - data rewritten", err_act_ok, B_OK },	// D   WRO   BK
	{ 0x1800, "Recovered data with error correction applied", err_act_ok, B_OK },	// DT  WRO   BK
	{ 0x1801, "Recovered data with error corr. & retries applied", err_act_ok, B_OK },	// D   WRO   BK
	{ 0x1802, "Recovered data - data auto-reallocated", err_act_ok, B_OK },	// D   WRO   BK
	{ 0x1803, "Recovered data with CIRC", err_act_ok, B_OK },	// R
	{ 0x1804, "Recovered data with l-EC", err_act_ok, B_OK },	// R
	{ 0x1805, "Recovered data - recommend reassignment", err_act_ok, B_OK },	// D   WRO   BK
	{ 0x1806, "Recovered data - recommend rewrite", err_act_ok, B_OK },	// D   WRO   BK
	{ 0x1807, "Recovered data with ECC - data rewritten", err_act_ok, B_OK },	// D   W O   BK
	{ 0x1808, "Recovered data with linking", err_act_fail, B_ERROR },	// R
	{ 0x1900, "Defect list error", err_act_fail, B_ERROR },	// D     O    K
	{ 0x1901, "Defect list not available", err_act_fail, B_ERROR },	// D     O    K
	{ 0x1902, "Defect list error in primary list", err_act_fail, B_ERROR },	// D     O    K
	{ 0x1903, "Defect list error in grown list", err_act_fail, B_ERROR },	// D     O    K
	{ 0x1A00, "Parameter list length error", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x1B00, "Synchronous data transfer error", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x1C00, "Defect list not found", err_act_fail, B_ERROR },	// D     O   BK
	{ 0x1C01, "Primary defect list not found", err_act_fail, B_ERROR },	// D     O   BK
	{ 0x1C02, "Grown defect list not found", err_act_fail, B_ERROR },	// D     O   BK
	{ 0x1D00, "Miscompare during verify operation", err_act_fail, B_ERROR },	// DT  WRO   BK
	{ 0x1D01, "Miscompare verify of unmapped LBA", err_act_fail, B_ERROR },	// D         B
	{ 0x1E00, "Recovered ID with ECC correction", err_act_ok, B_OK },	// D   W O   BK
	{ 0x1F00, "Partial defect list transfer", err_act_fail, B_ERROR },	// D     O    K
	{ 0x2000, "Invalid command operation code", err_act_fail, B_DEV_INVALID_IOCTL },	// DTLPWROMAEBKVF
	{ 0x2001, "Access denied - initiator pending-enrolled", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x2002, "Access denied - no access rights", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x2003, "Access denied - invalid mgmt ID key", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x2004, "Illegal command while in write capable state", err_act_fail, B_ERROR },	// T
	{ 0x2005, "Obsolete", err_act_fail, B_ERROR },	// T
	{ 0x2006, "Illegal command while in explicit address mode", err_act_fail, B_ERROR },	// T
	{ 0x2007, "Illegal command while in implicit address mode", err_act_fail, B_ERROR },	// T
	{ 0x2008, "Access denied - enrollment conflict", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x2009, "Access denied - invalid LU identifier", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x200A, "Access denied - invalid proxy token", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x200B, "Access denied - ACL LUN conflict", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x200C, "Illegal command when not in append-only mode", err_act_fail, B_DEV_INVALID_IOCTL },	// T
	{ 0x2100, "Logical block address out of range", err_act_fail, B_BAD_VALUE },	// DT  WRO   BK
	{ 0x2101, "Invalid element address", err_act_fail, B_BAD_VALUE },	// DT  WROM  BK
	{ 0x2102, "Invalid address for write", err_act_fail, B_ERROR },	// R
	{ 0x2103, "Invalid write crossing layer jump", err_act_fail, B_ERROR },	// R
	{ 0x2200, "Illegal function (use 20 00, 24 00, or 26 00)", err_act_fail, B_DEV_INVALID_IOCTL },	// D
	{ 0x2300, "Invalid token operation, cause not reportable", err_act_fail, B_ERROR },	// DT P      B
	{ 0x2301, "Invalid token operation, unsupported token type", err_act_fail, B_ERROR },	// DT P      B
	{ 0x2302, "Invalid token operation, remote token usage not supported", err_act_fail, B_ERROR },	// DT P      B
	{ 0x2303, "Invalid token operation, remote ROD token creation not supported", err_act_fail, B_ERROR },	// DT P      B
	{ 0x2304, "Invalid token operation, token unknown", err_act_fail, B_ERROR },	// DT P      B
	{ 0x2305, "Invalid token operation, token corrupt", err_act_fail, B_ERROR },	// DT P      B
	{ 0x2306, "Invalid token operation, token revoked", err_act_fail, B_ERROR },	// DT P      B
	{ 0x2307, "Invalid token operation, token expired", err_act_fail, B_ERROR },	// DT P      B
	{ 0x2308, "Invalid token operation, token cancelled", err_act_fail, B_ERROR },	// DT P      B
	{ 0x2309, "Invalid token operation, token deleted", err_act_fail, B_ERROR },	// DT P      B
	{ 0x230A, "Invalid token operation, invalid token length", err_act_fail, B_ERROR },	// DT P      B
	{ 0x2400, "Invalid field in CDB", err_act_fail, B_BAD_VALUE },	// DTLPWROMAEBKVF
	{ 0x2401, "CDB decryption error", err_act_fail, B_ERROR },	// DTLPWRO AEBKVF
	{ 0x2402, "Obsolete", err_act_fail, B_ERROR },	// T
	{ 0x2403, "Obsolete", err_act_fail, B_ERROR },	// T
	{ 0x2404, "Security audit value frozen", err_act_fail, B_ERROR },	// F
	{ 0x2405, "Security working key frozen", err_act_fail, B_ERROR },	// F
	{ 0x2406, "NONCE not unique", err_act_fail, B_ERROR },	// F
	{ 0x2407, "NONCE timestamp out of range", err_act_fail, B_ERROR },	// F
	{ 0x2408, "Invalid XCDB", err_act_fail, B_ERROR },	// DT   R MAEBKV
	{ 0x2500, "Logical unit not supported", err_act_fail, ENXIO },	// DTLPWROMAEBKVF
	{ 0x2600, "Invalid field in parameter list", err_act_fail, B_BAD_VALUE },	// DTLPWROMAEBKVF
	{ 0x2601, "Parameter not supported", err_act_fail, B_BAD_VALUE },	// DTLPWROMAEBKVF
	{ 0x2602, "Parameter value invalid", err_act_fail, B_BAD_VALUE },	// DTLPWROMAEBKVF
	{ 0x2603, "Threshold parameters not supported", err_act_fail, B_BAD_VALUE },	// DTLPWROMAE K
	{ 0x2604, "Invalid release of persistent reservation", err_act_fail, B_BAD_VALUE },	// DTLPWROMAEBKVF
	{ 0x2605, "Data decryption error", err_act_fail, B_ERROR },	// DTLPWRO A BK
	{ 0x2606, "Too many target descriptors", err_act_fail, B_ERROR },	// DTLPWRO    K
	{ 0x2607, "Unsupported target descriptor type code", err_act_fail, B_ERROR },	// DTLPWRO    K
	{ 0x2608, "Too many segment descriptors", err_act_fail, B_ERROR },	// DTLPWRO    K
	{ 0x2609, "Unsupported segment descriptor type code", err_act_fail, B_ERROR },	// DTLPWRO    K
	{ 0x260A, "Unexpected inexact segment", err_act_fail, B_ERROR },	// DTLPWRO    K
	{ 0x260B, "Inline data length exceeded", err_act_fail, B_ERROR },	// DTLPWRO    K
	{ 0x260C, "Invalid operation for copy source or destination", err_act_fail, B_ERROR },	// DTLPWRO    K
	{ 0x260D, "Copy segment granularity violation", err_act_fail, B_ERROR },	// DTLPWRO    K
	{ 0x260E, "Invalid parameter while port is enabled", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x260F, "Invalid data-out buffer integrity check value", err_act_fail, B_ERROR },	// F
	{ 0x2610, "Data decryption key fail limit reached", err_act_fail, B_ERROR },	// T
	{ 0x2611, "Incomplete key-associated data set", err_act_fail, B_ERROR },	// T
	{ 0x2612, "Vendor specific key reference not found", err_act_fail, B_ERROR },	// T
	{ 0x2700, "Write protected", err_act_fail, B_READ_ONLY_DEVICE },	// DT  WRO   BK
	{ 0x2701, "Hardware write protected", err_act_fail, B_READ_ONLY_DEVICE },	// DT  WRO   BK
	{ 0x2702, "Logical unit software write protected", err_act_fail, B_READ_ONLY_DEVICE },	// DT  WRO   BK
	{ 0x2703, "Associated write protect", err_act_fail, B_READ_ONLY_DEVICE },	// T   R
	{ 0x2704, "Persistent write protect", err_act_fail, B_READ_ONLY_DEVICE },	// T   R
	{ 0x2705, "Permanent write protect", err_act_fail, B_READ_ONLY_DEVICE },	// T   R
	{ 0x2706, "Conditional write protect", err_act_fail, B_READ_ONLY_DEVICE },	// R       F
	{ 0x2707, "Space allocation failed write protect", err_act_fail, B_READ_ONLY_DEVICE },	// D         B
	{ 0x2800, "Not ready to ready change, medium may have changed", err_act_fail, B_DEV_MEDIA_CHANGED },	// DTLPWROMAEBKVF
	{ 0x2801, "Import or export element accessed", err_act_fail, ENXIO },	// DT  WROM  B
	{ 0x2802, "Format-layer may have changed", err_act_fail, B_DEV_MEDIA_CHANGED },	// R
	{ 0x2803, "Import/export element accessed, medium changed", err_act_fail, B_ERROR },	// M
	{ 0x2900, "Power on, reset, or bus device reset occurred", err_act_retry, B_DEV_NOT_READY },	// DTLPWROMAEBKVF
	{ 0x2901, "Power on occurred", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2902, "SCSI bus reset occurred", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2903, "Bus device reset function occurred", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2904, "Device internal reset", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2905, "Transceiver mode changed to single-ended", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2906, "Transceiver mode changed to LVD", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2907, "I_T nexus loss occurred", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2A00, "Parameters changed", err_act_fail, B_ERROR },	// DTL WROMAEBKVF
	{ 0x2A01, "Mode parameters changed", err_act_fail, B_ERROR },	// DTL WROMAEBKVF
	{ 0x2A02, "Log parameters changed", err_act_fail, B_ERROR },	// DTL WROMAE K
	{ 0x2A03, "Reservations preempted", err_act_fail, B_ERROR },	// DTLPWROMAE K
	{ 0x2A04, "Reservations released", err_act_fail, B_ERROR },	// DTLPWROMAE
	{ 0x2A05, "Registrations preempted", err_act_fail, B_ERROR },	// DTLPWROMAE
	{ 0x2A06, "Asymmetric access state changed", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2A07, "Implicit asymmetric access state transition failed", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2A08, "Priority changed", err_act_fail, B_ERROR },	// DT  WROMAEBKVF
	{ 0x2A09, "Capacity data has changed", err_act_fail, B_ERROR },	// D
	{ 0x2A0A, "Error history I_T nexus cleared", err_act_fail, B_ERROR },	// DT
	{ 0x2A0B, "Error history snapshot released", err_act_fail, B_ERROR },	// DT
	{ 0x2A0C, "Error recovery attributes have changed", err_act_fail, B_ERROR },	// F
	{ 0x2A0D, "Data encryption capabilities changed", err_act_fail, B_ERROR },	// T
	{ 0x2A10, "Timestamp changed", err_act_fail, B_ERROR },	// DT     M E  V
	{ 0x2A11, "Data encryption parameters changed by another I_T nexus", err_act_fail, B_ERROR },	// T
	{ 0x2A12, "Data encryption parameters changed by vendor specific event", err_act_fail, B_ERROR },	// T
	{ 0x2A13, "Data encryption key instance counter has changed", err_act_fail, B_ERROR },	// T
	{ 0x2A14, "SA creation capabilities data has changed", err_act_fail, B_ERROR },	// DT   R MAEBKV
	{ 0x2A15, "Medium removal prevention preempted", err_act_fail, B_ERROR },	// T     M    V
	{ 0x2B00, "Copy cannot execute since host cannot disconnect", err_act_fail, B_ERROR },	// DTLPWRO    K
	{ 0x2C00, "Command sequence error", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2C01, "Too many windows specified", err_act_fail, B_ERROR },	//
	{ 0x2C02, "Invalid combination of windows specified", err_act_fail, B_ERROR },	//
	{ 0x2C03, "Current program area is not empty", err_act_fail, B_ERROR },	// R
	{ 0x2C04, "Current program area is empty", err_act_fail, B_ERROR },	// R
	{ 0x2C05, "Illegal power condition request", err_act_fail, B_ERROR },	// B
	{ 0x2C06, "Persistent prevent conflict", err_act_fail, B_ERROR },	// R
	{ 0x2C07, "Previous busy status", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2C08, "Previous task set full status", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2C09, "Previous reservation conflict status", err_act_fail, B_ERROR },	// DTLPWROM EBKVF
	{ 0x2C0A, "Partition or collection contains user objects", err_act_fail, B_ERROR },	// F
	{ 0x2C0B, "Not reserved", err_act_fail, B_ERROR },	// T
	{ 0x2C0C, "ORWRITE generation does not match", err_act_fail, B_ERROR },	// D
	{ 0x2D00, "Overwrite error on update in place", err_act_fail, B_ERROR },	// T
	{ 0x2E00, "Insufficient time for operation", err_act_fail, B_ERROR },	// D   WROM  B
	{ 0x2E01, "Command timeout before processing", err_act_fail, B_ERROR },	// D   W OM  B
	{ 0x2E02, "Command timeout during processing", err_act_fail, B_ERROR },	// D   W OM  B
	{ 0x2E03, "Command timeout during processing due to error recovery", err_act_fail, B_ERROR },	// D   W OM  B
	{ 0x2F00, "Commands cleared by another initiator", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2F01, "Commands cleared by power loss notification", err_act_fail, B_ERROR },	// D
	{ 0x2F02, "Commands cleared by device server", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x2F03, "Some commands cleared by queuing layer event", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3000, "Incompatible medium installed", err_act_fail, B_ERROR },	// DT  WROM  BK
	{ 0x3001, "Cannot read medium - unknown format", err_act_fail, B_ERROR },	// DT  WRO   BK
	{ 0x3002, "Cannot read medium - incompatible format", err_act_fail, B_ERROR },	// DT  WRO   BK
	{ 0x3003, "Cleaning cartridge installed", err_act_fail, B_ERROR },	// DT   R M   K
	{ 0x3004, "Cannot write medium - unknown format", err_act_fail, B_ERROR },	// DT  WRO   BK
	{ 0x3005, "Cannot write medium - incompatible format", err_act_fail, B_ERROR },	// DT  WRO   BK
	{ 0x3006, "Cannot format medium - incompatible medium", err_act_fail, B_ERROR },	// DT  WRO   B
	{ 0x3007, "Cleaning failure", err_act_fail, B_ERROR },	// DTL WROMAEBKVF
	{ 0x3008, "Cannot write - application code mismatch", err_act_fail, B_ERROR },	// R
	{ 0x3009, "Current session not fixated for append", err_act_fail, B_ERROR },	// R
	{ 0x300A, "Cleaning request rejected", err_act_fail, B_ERROR },	// DT  WRO AEBK
	{ 0x300C, "WORM medium - overwrite attempted", err_act_fail, B_ERROR },	// T
	{ 0x300D, "WORM medium - integrity check", err_act_fail, B_ERROR },	// T
	{ 0x3010, "Medium not formatted", err_act_fail, B_ERROR },	// R
	{ 0x3011, "Incompatible volume type", err_act_fail, B_ERROR },	// M
	{ 0x3012, "Incompatible volume qualifier", err_act_fail, B_ERROR },	// M
	{ 0x3013, "Cleaning volume expired", err_act_fail, B_ERROR },	// M
	{ 0x3100, "Medium format corrupted", err_act_fail, B_ERROR },	// DT  WRO   BK
	{ 0x3101, "Format command failed", err_act_fail, B_ERROR },	// D L  RO   B
	{ 0x3102, "Zoned formatting failed due to spare linking", err_act_fail, B_ERROR },	// R
	{ 0x3103, "SANITIZE command failed", err_act_fail, B_ERROR },	// D         B
	{ 0x3200, "No defect spare location available", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x3201, "Defect list update failure", err_act_fail, B_ERROR },	// D   W O   BK
	{ 0x3300, "Tape length error", err_act_fail, B_ERROR },	// T
	{ 0x3400, "Enclosure failure", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3500, "Enclosure services failure", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3501, "Unsupported enclosure function", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3502, "Enclosure services unavailable", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3503, "Enclosure services transfer failure", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3504, "Enclosure services transfer refused", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3505, "Enclosure services checksum error", err_act_fail, B_ERROR },	// DTL WROMAEBKVF
	{ 0x3600, "Ribbon, ink, or toner failure", err_act_fail, B_ERROR },	// L
	{ 0x3700, "Rounded parameter", err_act_fail, B_ERROR },	// DTL WROMAEBKVF
	{ 0x3800, "Event status notification", err_act_fail, B_ERROR },	// B
	{ 0x3802, "ESN - power management class event", err_act_fail, B_ERROR },	// B
	{ 0x3804, "ESN - media class event", err_act_fail, B_ERROR },	// B
	{ 0x3806, "ESN - device busy class event", err_act_fail, B_ERROR },	// B
	{ 0x3807, "Thin provisioning soft threshold reached", err_act_fail, B_ERROR },	// D
	{ 0x3900, "Saving parameters not supported", err_act_fail, B_ERROR },	// DTL WROMAE K
	{ 0x3A00, "Medium not present", err_act_fail,  B_DEV_NO_MEDIA },	// DTL WROM  BK
	{ 0x3A01, "Medium not present - tray closed", err_act_fail,  B_DEV_NO_MEDIA },	// DT  WROM  BK
	{ 0x3A02, "Medium not present - tray open", err_act_fail,  B_DEV_NO_MEDIA },	// DT  WROM  BK
	{ 0x3A03, "Medium not present - loadable", err_act_fail,  B_DEV_NO_MEDIA },	// DT  WROM  B
	{ 0x3A04, "Medium not present - medium auxiliary memory accessible", err_act_fail,  B_DEV_NO_MEDIA },	// DT  WRO   B
	{ 0x3B00, "Sequential positioning error", err_act_fail, B_ERROR },	// TL
	{ 0x3B01, "Tape position error at beginning-of-medium", err_act_fail, B_ERROR },	// T
	{ 0x3B02, "Tape position error at end-of-medium", err_act_fail, B_ERROR },	// T
	{ 0x3B03, "Tape or electronic vertical forms unit not ready", err_act_fail, B_ERROR },	// L
	{ 0x3B04, "Slew failure", err_act_fail, B_ERROR },	// L
	{ 0x3B05, "Paper jam", err_act_fail, B_ERROR },	// L
	{ 0x3B06, "Failed to sense top-of-form", err_act_fail, B_ERROR },	// L
	{ 0x3B07, "Failed to sense bottom-of-form", err_act_fail, B_ERROR },	// L
	{ 0x3B08, "Reposition error", err_act_fail, B_ERROR },	// T
	{ 0x3B09, "Read past end of medium", err_act_fail, B_ERROR },	//
	{ 0x3B0A, "Read past beginning of medium", err_act_fail, B_ERROR },	//
	{ 0x3B0B, "Position past end of medium", err_act_fail, B_ERROR },	//
	{ 0x3B0C, "Position past beginning of medium", err_act_fail, B_ERROR },	// T
	{ 0x3B0D, "Medium destination element full", err_act_fail, B_DEVICE_FULL },	// DT  WROM  BK
	{ 0x3B0E, "Medium source element empty", err_act_fail, B_ERROR },	// DT  WROM  BK
	{ 0x3B0F, "End of medium reached", err_act_fail, B_ERROR },	// R
	{ 0x3B11, "Medium magazine not accessible", err_act_fail, B_ERROR },	// DT  WROM  BK
	{ 0x3B12, "Medium magazine removed", err_act_fail, B_ERROR },	// DT  WROM  BK
	{ 0x3B13, "Medium magazine inserted", err_act_fail, B_ERROR },	// DT  WROM  BK
	{ 0x3B14, "Medium magazine locked", err_act_fail, B_ERROR },	// DT  WROM  BK
	{ 0x3B15, "Medium magazine unlocked", err_act_fail, B_ERROR },	// DT  WROM  BK
	{ 0x3B16, "Mechanical positioning or changer error", err_act_fail, B_ERROR },	// R
	{ 0x3B17, "Read past end of user object", err_act_fail, B_ERROR },	// F
	{ 0x3B18, "Element disabled", err_act_fail, B_ERROR },	// M
	{ 0x3B19, "Element enabled", err_act_fail, B_ERROR },	// M
	{ 0x3B1A, "Data transfer device removed", err_act_fail, B_ERROR },	// M
	{ 0x3B1B, "Data transfer device inserted", err_act_fail, B_ERROR },	// M
	{ 0x3B1C, "Too many logical objects on partition to support operation", err_act_fail, B_ERROR },	// T
	{ 0x3D00, "Invalid bits in IDENTIFY message", err_act_fail, B_ERROR },	// DTLPWROMAE K
	{ 0x3E00, "Logical unit has not self-configured yet", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3E01, "Logical unit failure", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3E02, "Timeout on logical unit", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3E03, "Logical unit failed self-test", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3E04, "Logical unit unable to update self-test log", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3F00, "Target operating conditions have changed", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3F01, "Microcode has been changed", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3F02, "Changed operating definition", err_act_fail, B_ERROR },	// DTLPWROM  BK
	{ 0x3F03, "INQUIRY data has changed", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3F04, "Component device attached", err_act_fail, B_ERROR },	// DT  WROMAEBK
	{ 0x3F05, "Device identifier changed", err_act_fail, B_ERROR },	// DT  WROMAEBK
	{ 0x3F06, "Redundancy group created or modified", err_act_fail, B_ERROR },	// DT  WROMAEB
	{ 0x3F07, "Redundancy group deleted", err_act_fail, B_ERROR },	// DT  WROMAEB
	{ 0x3F08, "Spare created or modified", err_act_fail, B_ERROR },	// DT  WROMAEB
	{ 0x3F09, "Spare deleted", err_act_fail, B_ERROR },	// DT  WROMAEB
	{ 0x3F0A, "Volume set created or modified", err_act_fail, B_ERROR },	// DT  WROMAEBK
	{ 0x3F0B, "Volume set deleted", err_act_fail, B_ERROR },	// DT  WROMAEBK
	{ 0x3F0C, "Volume set deassigned", err_act_fail, B_ERROR },	// DT  WROMAEBK
	{ 0x3F0D, "Volume set reassigned", err_act_fail, B_ERROR },	// DT  WROMAEBK
	{ 0x3F0E, "Reported LUNs data has changed", err_act_rescan, B_ERROR },	// DTLPWROMAE
	{ 0x3F0F, "Echo buffer overwritten", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x3F10, "Medium loadable", err_act_fail, B_ERROR },	// DT  WROM  B
	{ 0x3F11, "Medium auxiliary memory accessible", err_act_fail, B_ERROR },	// DT  WROM  B
	{ 0x3F12, "iSCSI IP address added", err_act_fail, B_ERROR },	// DTLPWR MAEBK F
	{ 0x3F13, "iSCSI IP address removed", err_act_fail, B_ERROR },	// DTLPWR MAEBK F
	{ 0x3F14, "iSCSI IP address changed", err_act_fail, B_ERROR },	// DTLPWR MAEBK F
	{ 0x3F15, "Inspect referrals sense descriptors", err_act_fail, B_ERROR },	// DTLPWR MAEBK
	{ 0x4000, "RAM failure", err_act_fail, B_ERROR },	// D
	{ 0x40ff, "Diagnostic failure on component ID ASQ", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4100, "Data path failure", err_act_fail, B_ERROR },	// D
	{ 0x4200, "Power-on or self-test failure", err_act_fail, B_ERROR },	// D
	{ 0x4300, "Message error", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4400, "Internal target failure", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4401, "Persistent reservation information lost", err_act_fail, B_ERROR },	// DT P   MAEBKVF
	{ 0x4471, "ATA device failed set features", err_act_fail, B_ERROR },	// DT        B
	{ 0x4500, "Select or reselect failure", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4600, "Unsuccessful soft reset", err_act_fail, B_ERROR },	// DTLPWROM  BK
	{ 0x4700, "SCSI parity error", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4701, "Data phase CRC error detected", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4702, "SCSI parity error detected during ST data phase", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4703, "Information unit iuCRC error detected", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4704, "Asynchronous information protection error detected", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4705, "Protocol service CRC error", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4706, "PHY test function in progress", err_act_fail, B_ERROR },	// DT     MAEBKVF
	{ 0x477F, "Some commands cleared by iSCSI protocol event", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x4800, "Initiator detected error message received", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4900, "Invalid message error", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4A00, "Command phase error", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4B00, "Data phase error", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4B01, "Invalid target port transfer tag received", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x4B02, "Too much write data", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x4B03, "ACK/NAK timeout", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x4B04, "NAK received", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x4B05, "Data offset error", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x4B06, "Initiator response timeout", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x4B07, "Connection lost", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B08, "Data-in buffer overflow - data buffer size", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B09, "Data-in buffer overflow - data buffer descriptor area", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B0A, "Data-in buffer error", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B0B, "Data-out buffer overflow - data buffer size", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B0C, "Data-out buffer overflow - data buffer descriptor area", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B0D, "Data-out buffer error", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B0E, "PCIe fabric error", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B0F, "PCIe completion timeout", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B10, "PCIe completer abort", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B11, "PCIe poisoned TLP received", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B12, "PCIe eCRC check failed", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B13, "PCIe unsupported request", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B14, "PCIe ACS violation", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4B15, "PCIe TLP prefix blocked", err_act_fail, B_ERROR },	// DT PWROMAEBK F
	{ 0x4C00, "Logical unit failed self-configuration", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4DFF, "Tagged overlapped commands ASQ", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x4E00, "Overlapped commands attempted", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x5000, "Write append error", err_act_fail, B_ERROR },	// T
	{ 0x5001, "Write append position error", err_act_fail, B_ERROR },	// T
	{ 0x5002, "Position error related to timing", err_act_fail, B_ERROR },	// T
	{ 0x5100, "Erase failure", err_act_fail, B_ERROR },	// T   RO
	{ 0x5101, "Erase failure - incomplete erase operation detected", err_act_fail, B_ERROR },	// R
	{ 0x5200, "Cartridge fault", err_act_fail, B_ERROR },	// T
	{ 0x5300, "Media load or eject failed", err_act_fail, B_ERROR },	// DTL WROM  BK
	{ 0x5301, "Unload tape failure", err_act_fail, B_ERROR },	// T
	{ 0x5302, "Medium removal prevented", err_act_fail, B_ERROR },	// DT  WROM  BK
	{ 0x5303, "Medium removal prevented by data transfer element", err_act_fail, B_ERROR },	// M
	{ 0x5304, "Medium thread or unthread failure", err_act_fail, B_ERROR },	// T
	{ 0x5305, "Volume identifier invalid", err_act_fail, B_ERROR },	// M
	{ 0x5306, "Volume identifier missing", err_act_fail, B_ERROR },	// M
	{ 0x5307, "Duplicate volume identifier", err_act_fail, B_ERROR },	// M
	{ 0x5308, "Element status unknown", err_act_fail, B_ERROR },	// M
	{ 0x5400, "SCSI to host system interface failure", err_act_fail, B_ERROR },	// P
	{ 0x5500, "System resource failure", err_act_fail, B_ERROR },	// P
	{ 0x5501, "System buffer full", err_act_fail, B_DEVICE_FULL },	// D     O   BK
	{ 0x5502, "Insufficient reservation resources", err_act_fail, B_ERROR },	// DTLPWROMAE K
	{ 0x5503, "Insufficient resources", err_act_fail, B_ERROR },	// DTLPWROMAE K
	{ 0x5504, "Insufficient registration resources", err_act_fail, B_ERROR },	// DTLPWROMAE K
	{ 0x5505, "Insufficient access control resources", err_act_fail, B_ERROR },	// DT PWROMAEBK
	{ 0x5506, "Auxiliary memory out of space", err_act_fail, B_ERROR },	// DT  WROM  B
	{ 0x5507, "Quota error", err_act_fail, B_ERROR },	// F
	{ 0x5508, "Maximum number of supplemental decryption keys exceeded", err_act_fail, B_ERROR },	// T
	{ 0x5509, "Medium auxiliary memory not accessible", err_act_fail, B_ERROR },	// M
	{ 0x550A, "Data currently unavailable", err_act_fail, B_ERROR },	// M
	{ 0x550B, "Insufficient power for operation", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x550C, "Insufficient resources to create ROD", err_act_fail, B_ERROR },	// DT P      B
	{ 0x550D, "Insufficient resources to create ROD token", err_act_fail, B_ERROR },	// DT P      B
	{ 0x5700, "Unable to recover table-of-contents", err_act_fail, B_ERROR },	// R
	{ 0x5800, "Generation does not exist", err_act_fail, B_ERROR },	// O
	{ 0x5900, "Updated block read", err_act_fail, B_ERROR },	// O
	{ 0x5A00, "Operator request or state change input", err_act_fail, B_ERROR },	// DTLPWRO   BK
	{ 0x5A01, "Operator medium removal request", err_act_retry, B_DEV_MEDIA_CHANGE_REQUESTED },	// DT  WROM  BK
	{ 0x5A02, "Operator selected write protect", err_act_fail, B_ERROR },	// DT  WRO A BK
	{ 0x5A03, "Operator selected write permit", err_act_fail, B_ERROR },	// DT  WRO A BK
	{ 0x5B00, "Log exception", err_act_fail, B_ERROR },	// DTLPWROM   K
	{ 0x5B01, "Threshold condition met", err_act_fail, B_ERROR },	// DTLPWROM   K
	{ 0x5B02, "Log counter at maximum", err_act_fail, B_ERROR },	// DTLPWROM   K
	{ 0x5B03, "Log list codes exhausted", err_act_fail, B_ERROR },	// DTLPWROM   K
	{ 0x5C00, "RPL status change", err_act_fail, B_ERROR },	// D     O
	{ 0x5C01, "Spindles synchronized", err_act_ok, B_OK },	// D     O
	{ 0x5C02, "Spindles not synchronized", err_act_fail, B_ERROR },	// D     O
	{ 0x5D00, "Failure prediction threshold exceeded", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x5D01, "Media failure prediction threshold exceeded", err_act_fail, B_ERROR },	// R    B
	{ 0x5D02, "Logical unit failure prediction threshold exceeded", err_act_fail, B_ERROR },	// R
	{ 0x5D03, "Spare area exhaustion prediction threshold exceeded", err_act_fail, B_ERROR },	// R
	{ 0x5D10, "Hardware impending failure general hard drive failure", err_act_fail, B_ERROR },	// D         B
	{ 0x5D11, "Hardware impending failure drive error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D12, "Hardware impending failure data error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D13, "Hardware impending failure seek error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D14, "Hardware impending failure too many block reassigns", err_act_fail, B_ERROR },	// D         B
	{ 0x5D15, "Hardware impending failure access times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D16, "Hardware impending failure start unit times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D17, "Hardware impending failure channel parametrics", err_act_fail, B_ERROR },	// D         B
	{ 0x5D18, "Hardware impending failure controller detected", err_act_fail, B_ERROR },	// D         B
	{ 0x5D19, "Hardware impending failure throughput performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D1A, "Hardware impending failure seek time performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D1B, "Hardware impending failure spin-up retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D1C, "Hardware impending failure drive calibration retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D20, "Controller impending failure general hard drive failure", err_act_fail, B_ERROR },	// D         B
	{ 0x5D21, "Controller impending failure drive error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D22, "Controller impending failure data error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D23, "Controller impending failure seek error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D24, "Controller impending failure too many block reassigns", err_act_fail, B_ERROR },	// D         B
	{ 0x5D25, "Controller impending failure access times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D26, "Controller impending failure start unit times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D27, "Controller impending failure channel parametrics", err_act_fail, B_ERROR },	// D         B
	{ 0x5D28, "Controller impending failure controller detected", err_act_fail, B_ERROR },	// D         B
	{ 0x5D29, "Controller impending failure throughput performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D2A, "Controller impending failure seek time performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D2B, "Controller impending failure spin-up retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D2C, "Controller impending failure drive calibration retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D30, "Data channel impending failure general hard drive failure", err_act_fail, B_ERROR },	// D         B
	{ 0x5D31, "Data channel impending failure drive error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D32, "Data channel impending failure data error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D33, "Data channel impending failure seek error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D34, "Data channel impending failure too many block reassigns", err_act_fail, B_ERROR },	// D         B
	{ 0x5D35, "Data channel impending failure access times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D36, "Data channel impending failure start unit times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D37, "Data channel impending failure channel parametrics", err_act_fail, B_ERROR },	// D         B
	{ 0x5D38, "Data channel impending failure controller detected", err_act_fail, B_ERROR },	// D         B
	{ 0x5D39, "Data channel impending failure throughput performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D3A, "Data channel impending failure seek time performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D3B, "Data channel impending failure spin-up retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D3C, "Data channel impending failure drive calibration retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D40, "Servo impending failure general hard drive failure", err_act_fail, B_ERROR },	// D         B
	{ 0x5D41, "Servo impending failure drive error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D42, "Servo impending failure data error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D43, "Servo impending failure seek error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D44, "Servo impending failure too many block reassigns", err_act_fail, B_ERROR },	// D         B
	{ 0x5D45, "Servo impending failure access times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D46, "Servo impending failure start unit times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D47, "Servo impending failure channel parametrics", err_act_fail, B_ERROR },	// D         B
	{ 0x5D48, "Servo impending failure controller detected", err_act_fail, B_ERROR },	// D         B
	{ 0x5D49, "Servo impending failure throughput performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D4A, "Servo impending failure seek time performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D4B, "Servo impending failure spin-up retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D4C, "Servo impending failure drive calibration retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D50, "Spindle impending failure general hard drive failure", err_act_fail, B_ERROR },	// D         B
	{ 0x5D51, "Spindle impending failure drive error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D52, "Spindle impending failure data error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D53, "Spindle impending failure seek error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D54, "Spindle impending failure too many block reassigns", err_act_fail, B_ERROR },	// D         B
	{ 0x5D55, "Spindle impending failure access times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D56, "Spindle impending failure start unit times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D57, "Spindle impending failure channel parametrics", err_act_fail, B_ERROR },	// D         B
	{ 0x5D58, "Spindle impending failure controller detected", err_act_fail, B_ERROR },	// D         B
	{ 0x5D59, "Spindle impending failure throughput performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D5A, "Spindle impending failure seek time performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D5B, "Spindle impending failure spin-up retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D5C, "Spindle impending failure drive calibration retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D60, "Firmware impending failure general hard drive failure", err_act_fail, B_ERROR },	// D         B
	{ 0x5D61, "Firmware impending failure drive error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D62, "Firmware impending failure data error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D63, "Firmware impending failure seek error rate too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D64, "Firmware impending failure too many block reassigns", err_act_fail, B_ERROR },	// D         B
	{ 0x5D65, "Firmware impending failure access times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D66, "Firmware impending failure start unit times too high", err_act_fail, B_ERROR },	// D         B
	{ 0x5D67, "Firmware impending failure channel parametrics", err_act_fail, B_ERROR },	// D         B
	{ 0x5D68, "Firmware impending failure controller detected", err_act_fail, B_ERROR },	// D         B
	{ 0x5D69, "Firmware impending failure throughput performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D6A, "Firmware impending failure seek time performance", err_act_fail, B_ERROR },	// D         B
	{ 0x5D6B, "Firmware impending failure spin-up retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5D6C, "Firmware impending failure drive calibration retry count", err_act_fail, B_ERROR },	// D         B
	{ 0x5DFF, "Failure prediction threshold exceeded (false)", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x5E00, "Low power condition on", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E01, "Idle condition activated by timer", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E02, "Standby condition activated by timer", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E03, "Idle condition activated by command", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E04, "Standby condition activated by command", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E05, "Idle-B condition activated by timer", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E06, "Idle-B condition activated by command", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E07, "Idle-C condition activated by timer", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E08, "Idle-C condition activated by command", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E09, "Standby-Y condition activated by timer", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E0A, "Standby-Y condition activated by command", err_act_fail, B_ERROR },	// DTLPWRO A  K
	{ 0x5E41, "Power state change to active", err_act_fail, B_ERROR },	// B
	{ 0x5E42, "Power state change to idle", err_act_fail, B_ERROR },	// B
	{ 0x5E43, "Power state change to standby", err_act_fail, B_ERROR },	// B
	{ 0x5E45, "Power state change to sleep", err_act_fail, B_ERROR },	// B
	{ 0x5E47, "Power state change to device control", err_act_fail, B_ERROR },	// BK
	{ 0x6000, "Lamp failure", err_act_fail, B_ERROR },	//
	{ 0x6100, "Video acquisition error", err_act_fail, B_ERROR },	//
	{ 0x6101, "Unable to acquire video", err_act_fail, B_ERROR },	//
	{ 0x6102, "Out of focus", err_act_fail, B_ERROR },	//
	{ 0x6200, "Scan head positioning error", err_act_fail, B_ERROR },	//
	{ 0x6300, "End of user area encountered on this track", err_act_fail, B_ERROR },	// R
	{ 0x6301, "Packet does not fit in available space", err_act_fail, B_DEVICE_FULL },	// R
	{ 0x6400, "Illegal mode for this track", err_act_fail, B_DEV_NOT_READY },	// R
	{ 0x6401, "Invalid packet size", err_act_fail, B_ERROR },	// R
	{ 0x6500, "Voltage fault", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x6600, "Automatic document feeder cover up", err_act_fail, B_ERROR },	//
	{ 0x6601, "Automatic document feeder lift up", err_act_fail, B_ERROR },	//
	{ 0x6602, "Document jam in automatic document feeder", err_act_fail, B_ERROR },	//
	{ 0x6603, "Document miss feed automatic in document feeder", err_act_fail, B_ERROR },	//
	{ 0x6700, "Configuration failure", err_act_fail, B_ERROR },	// A
	{ 0x6701, "Configuration of incapable logical units failed", err_act_fail, B_ERROR },	// A
	{ 0x6702, "Add logical unit failed", err_act_fail, B_ERROR },	// A
	{ 0x6703, "Modification of logical unit failed", err_act_fail, B_ERROR },	// A
	{ 0x6704, "Exchange of logical unit failed", err_act_fail, B_ERROR },	// A
	{ 0x6705, "Remove of logical unit failed", err_act_fail, B_ERROR },	// A
	{ 0x6706, "Attachment of logical unit failed", err_act_fail, B_ERROR },	// A
	{ 0x6707, "Creation of logical unit failed", err_act_fail, B_ERROR },	// A
	{ 0x6708, "Assign failure occurred", err_act_fail, B_ERROR },	// A
	{ 0x6709, "Multiply assigned logical unit", err_act_fail, B_ERROR },	// A
	{ 0x670A, "Set target port groups command failed", err_act_fail, B_ERROR },	// DTLPWROMAEBKVF
	{ 0x670B, "ATA device feature not enabled", err_act_fail, B_ERROR },	// DT        B
	{ 0x6800, "Logical unit not configured", err_act_fail, B_ERROR },	// A
	{ 0x6801, "Subsidiary logical unit not configured", err_act_fail, B_ERROR },	// D
	{ 0x6900, "Data loss on logical unit", err_act_fail, B_ERROR },	// A
	{ 0x6901, "Multiple logical unit failures", err_act_fail, B_ERROR },	// A
	{ 0x6902, "Parity/data mismatch", err_act_fail, B_ERROR },	// A
	{ 0x6A00, "Informational, refer to log", err_act_fail, B_ERROR },	// A
	{ 0x6B00, "State change has occurred", err_act_fail, B_ERROR },	// A
	{ 0x6B01, "Redundancy level got better", err_act_fail, B_ERROR },	// A
	{ 0x6B02, "Redundancy level got worse", err_act_fail, B_ERROR },	// A
	{ 0x6C00, "Rebuild failure occurred", err_act_fail, B_ERROR },	// A
	{ 0x6D00, "Recalculate failure occurred", err_act_fail, B_ERROR },	// A
	{ 0x6E00, "Command to logical unit failed", err_act_fail, B_ERROR },	// A
	{ 0x6F00, "Copy protection key exchange failure - authentication failure", err_act_fail, B_ERROR },	// R
	{ 0x6F01, "Copy protection key exchange failure - key not present", err_act_fail, B_ERROR },	// R
	{ 0x6F02, "Copy protection key exchange failure - key not established", err_act_fail, B_ERROR },	// R
	{ 0x6F03, "Read of scrambled sector without authentication", err_act_fail, B_ERROR },	// R
	{ 0x6F04, "Media region code is mismatched to logical unit region", err_act_fail, B_ERROR },	// R
	{ 0x6F05, "Drive region must be permanent/region reset count error", err_act_fail, B_ERROR },	// R
	{ 0x6F06, "Insufficient block count for binding NONCE recording", err_act_fail, B_ERROR },	// R
	{ 0x6F07, "Conflict in binding nonce recording", err_act_fail, B_ERROR },	// R
	{ 0x70FF, "Decompression exception short algorithm id ASQ", err_act_fail, B_ERROR },	// T
	{ 0x7100, "Decompression exception long algorithm id", err_act_fail, B_ERROR },	// T
	{ 0x7200, "Session fixation error", err_act_fail, B_ERROR },	// R
	{ 0x7201, "Session fixation error writing lead-in", err_act_fail, B_ERROR },	// R
	{ 0x7202, "Session fixation error writing lead-out", err_act_fail, B_ERROR },	// R
	{ 0x7203, "Session fixation error - incomplete track in session", err_act_fail, B_ERROR },	// R
	{ 0x7204, "Empty or partially written reserved track", err_act_fail, B_ERROR },	// R
	{ 0x7205, "No more track reservations allowed", err_act_fail, B_ERROR },	// R
	{ 0x7206, "RMZ extension is not allowed", err_act_fail, B_ERROR },	// R
	{ 0x7207, "No more test zone extensions are allowed", err_act_fail, B_ERROR },	// R
	{ 0x7300, "CD control error", err_act_fail, B_ERROR },	// R
	{ 0x7301, "Power calibration area almost full", err_act_fail, B_ERROR },	// R
	{ 0x7302, "Power calibration area is full", err_act_fail, B_DEVICE_FULL },	// R
	{ 0x7303, "Power calibration area error", err_act_fail, B_ERROR },	// R
	{ 0x7304, "Program memory area update failure", err_act_fail, B_ERROR },	// R
	{ 0x7305, "Program memory area is full", err_act_fail, B_ERROR },	// R
	{ 0x7306, "RMA/PMA is almost full", err_act_fail, B_ERROR },	// R
	{ 0x7310, "Current power calibration area almost full", err_act_fail, B_ERROR },	// R
	{ 0x7311, "Current power calibration area is full", err_act_fail, B_ERROR },	// R
	{ 0x7317, "RDZ is full", err_act_fail, B_ERROR },	// R
	{ 0x7400, "Security error", err_act_fail, B_ERROR },	// T
	{ 0x7401, "Unable to decrypt data", err_act_fail, B_ERROR },	// T
	{ 0x7402, "Unencrypted data encountered while decrypting", err_act_fail, B_ERROR },	// T
	{ 0x7403, "Incorrect data encryption key", err_act_fail, B_ERROR },	// T
	{ 0x7404, "Cryptographic integrity validation failed", err_act_fail, B_ERROR },	// T
	{ 0x7405, "Error decrypting data", err_act_fail, B_ERROR },	// T
	{ 0x7406, "Unknown signature verification key", err_act_fail, B_ERROR },	// T
	{ 0x7407, "Encryption parameters not useable", err_act_fail, B_ERROR },	// T
	{ 0x7408, "Digital signature validation failure", err_act_fail, B_ERROR },	// DT   R M E  VF
	{ 0x7409, "Encryption mode mismatch on read", err_act_fail, B_ERROR },	// T
	{ 0x740A, "Encrypted block not raw read enabled", err_act_fail, B_ERROR },	// T
	{ 0x740B, "Incorrect encryption parameters", err_act_fail, B_ERROR },	// T
	{ 0x740C, "Unable to decrypt parameter list", err_act_fail, B_ERROR },	// DT   R MAEBKV
	{ 0x740D, "Encryption algorithm disabled", err_act_fail, B_ERROR },	// T
	{ 0x7410, "SA creation parameter value invalid", err_act_fail, B_ERROR },	// DT   R MAEBKV
	{ 0x7411, "SA creation parameter value rejected", err_act_fail, B_ERROR },	// DT   R MAEBKV
	{ 0x7412, "Invalid SA usage", err_act_fail, B_ERROR },	// DT   R MAEBKV
	{ 0x7421, "Data encryption configuration prevented", err_act_fail, B_ERROR },	// T
	{ 0x7430, "SA creation parameter not supported", err_act_fail, B_ERROR },	// DT   R MAEBKV
	{ 0x7440, "Authentication failed", err_act_fail, B_ERROR },	// DT   R MAEBKV
	{ 0x7461, "External data encryption key manager access error", err_act_fail, B_ERROR },	// V
	{ 0x7462, "External data encryption key manager error", err_act_fail, B_ERROR },	// V
	{ 0x7463, "External data encryption key not found", err_act_fail, B_ERROR },	// V
	{ 0x7464, "External data encryption request not authorized", err_act_fail, B_ERROR },	// V
	{ 0x746E, "External data encryption control timeout", err_act_fail, B_ERROR },	// T
	{ 0x746F, "External data encryption control error", err_act_fail, B_ERROR },	// T
	{ 0x7471, "Logical unit access not authorized", err_act_fail, B_ERROR },	// DT   R M E  V
	{ 0x7479, "Security conflict in translated device", err_act_fail, B_ERROR },	// D
};

// Use these values for loop control during searching:
#define	SCSI_SENSE_KEY_TABLE_LEN	(sizeof(sSCSISenseKeyTable)/sizeof(scsi_sense_key_table))
#define	SCSI_SENSE_ASC_ASCQ_TABLE_LEN	(sizeof(sSCSISenseAscTable)/sizeof(scsi_sense_asc_table))


status_t
scsi_get_sense_key_info(uint8 key, const char **label, err_act *action, status_t *error)
{
	int i;
	for (i = 0; i < (int)SCSI_SENSE_KEY_TABLE_LEN; i++) {
		if (sSCSISenseKeyTable[i].key == key) {
			*label = sSCSISenseKeyTable[i].label;
			*action = sSCSISenseKeyTable[i].action;
			*error = sSCSISenseKeyTable[i].err;
			return B_OK;
		}
	}
	*label = NULL;
	return B_BAD_VALUE;
}


status_t
scsi_get_sense_asc_info(uint16 asc_asq, const char **label, err_act *action, status_t *error)
{
	int i;
	for (i = 0; i < (int)SCSI_SENSE_ASC_ASCQ_TABLE_LEN; i++) {
		if (sSCSISenseAscTable[i].asc_ascq == asc_asq) {
			*label = sSCSISenseAscTable[i].label;
			*action = sSCSISenseAscTable[i].action;
			*error = sSCSISenseAscTable[i].err;
			return B_OK;
		}
	}
	*label = NULL;
	return B_BAD_VALUE;
}


