/******************************************************************************
/
/	File:			Radeon.cpp
/
/	Description:	ATI Radeon Graphics Chip interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <unistd.h>
#include <dirent.h>
#include <Debug.h>
#include <string.h>
//#include "Driver.h"
#include "Radeon.h"
#include "OS.h"

static const char * const C_RADEON_REGISTER_AREA_NAME = "RadeonRegisters";
static const char * const C_RADEON_MEMORY_AREA_NAME = "RadeonMemory";
static const char * const C_RADEON_ROM_AREA_NAME = "RadeonROM";

// CRadeonRect
CRadeonRect::CRadeonRect()
	:	fLeft(0),
		fTop(0),
		fRight(0),
		fBottom(0)
{
}

CRadeonRect::CRadeonRect(int left, int top, int right, int bottom)
	:	fLeft(left),
		fTop(top),
		fRight(right),
		fBottom(bottom)
{
}

int CRadeonRect::Left() const
{
	return fLeft;
}

int CRadeonRect::Top() const
{
	return fTop;
}

int CRadeonRect::Right() const
{
	return fRight;
}

int CRadeonRect::Bottom() const
{
	return fBottom;
}

int CRadeonRect::Width() const
{
	return fRight - fLeft + 1;
}

int CRadeonRect::Height() const
{
	return fBottom - fTop + 1;
}

void CRadeonRect::SetLeft(int value)
{
	fLeft = value;
}
	
void CRadeonRect::SetTop(int value)
{
	fTop = value;
}

void CRadeonRect::SetRight(int value)
{
	fRight = value;
}

void CRadeonRect::SetBottom(int value)
{
	fBottom = value;
}

void CRadeonRect::SetTo(int left, int top, int right, int bottom)
{
	fLeft = left;
	fTop = top;
	fRight = right;
	fBottom = bottom;
}

void CRadeonRect::MoveTo(int left, int top)
{
	fRight += left - fLeft;
	fBottom += top - fTop;
	fLeft += left - fLeft;
	fTop += top - fTop;
}

void CRadeonRect::ResizeTo(int width, int height)
{
	fRight = fLeft + width - 1;
	fBottom = fTop + height - 1;
}


// CRadeon
CRadeon::CRadeon( const char *dev_name )
	:	fHandle(0),
		fRegister(NULL),
		fROM(NULL),
		fVirtualCard(NULL),
		fSharedInfo(NULL),
		fRegisterArea(0),
		fROMArea(0),
		fVirtualCardArea(0),
		fSharedInfoArea(0),
		caps_video_in(0)
{
	PRINT(("CRadeon::CRadeon()\n"));
	
	if ((fHandle = open(dev_name, O_RDWR | O_CLOEXEC)) < 0) {
		PRINT(("CRadeon::CRadeon() - Can't open kernel driver\n"));
		return;
	}

	radeon_get_private_data gpd;
	
	if (GetDeviceInformation(gpd) < B_OK) {
		PRINT(("CRadeon::CRadeon() - Can't get device information\n"));
		return;
	}

	CloneArea( 
		"Radeon virtual card", gpd.virtual_card_area, 
		&fVirtualCardArea, (void **)&fVirtualCard );
	CloneArea( 
		"Radeon shared info", gpd.shared_info_area, 
		&fSharedInfoArea, (void **)&fSharedInfo );
		
	if( fSharedInfo != NULL )  {
		CloneArea(
			"Radeon regs", fSharedInfo->regs_area, 
			&fRegisterArea, (void **)&fRegister );
		CloneArea(
			"Radeon ROM", fSharedInfo->ROM_area, 
			&fROMArea, (void **)&fROM );
	}
	
	if (fVirtualCard == NULL || fSharedInfo == NULL || 
		fROM == NULL || fRegister == NULL) 
	{
		PRINT(("CRadeon::CRadeon() - Can't map memory apertures\n"));
		return;
	} 
	
	PRINT(("CRadeon::CRadeon() - ATI Radeon found\n"));
}

CRadeon::~CRadeon()
{
	PRINT(("CRadeon::~CRadeon()\n"));

	if( fVirtualCard != NULL )
		delete_area( fVirtualCardArea );
		
	if( fSharedInfo != NULL )
		delete_area( fSharedInfoArea );
		
	if (fRegister != NULL)
		delete_area( fRegisterArea );
		
	if (fROM != NULL)
		delete_area( fROMArea );

	if (fHandle >= 0)		
		close(fHandle);
}

status_t CRadeon::InitCheck() const
{
	return 
		(fHandle >= 0 && 
		fRegister != NULL && fROM != NULL &&
		fVirtualCard != NULL && fSharedInfo != NULL) ? B_OK : B_ERROR;
}

uint32 CRadeon::VirtualMemoryBase() const
{
	return fSharedInfo->memory[mt_local].virtual_addr_start;
}

int CRadeon::Register(radeon_register index) const
{
	return fRegister[index >> 2];
}

void CRadeon::SetRegister(radeon_register index, int value)
{
	fRegister[index >> 2] = value;
}

int CRadeon::Register(radeon_register index, int mask) const
{
	return fRegister[index >> 2] & mask;
}

void CRadeon::SetRegister(radeon_register index, int mask, int value)
{
#ifdef DEBUG
	if ((value & ~mask) != 0)
		PRINT(("CRadeon::SetRegister(0x%04x, 0x%08x, 0x%08x)\n", index, mask, value));
#endif

	fRegister[index >> 2] = (fRegister[index >> 2] & ~mask) | (value & mask);
}

int CRadeon::VIPRegister(int device, int address)
{
	radeon_vip_read vr;
	status_t res;
	
	vr.magic = RADEON_PRIVATE_DATA_MAGIC;
	vr.channel = device;
	vr.address = address;
	vr.lock = true;
	
	res = ioctl( fHandle, RADEON_VIPREAD, &vr, sizeof( vr ));
	
	if( res == B_OK )
		return vr.data;
	else
		return -1;
}
	
void CRadeon::SetVIPRegister(int device, int address, int value)
{
	radeon_vip_write vw;
	
	vw.magic = RADEON_PRIVATE_DATA_MAGIC;
	vw.channel = device;
	vw.address = address;
	vw.data = value;
	vw.lock = true;
	
	ioctl( fHandle, RADEON_VIPWRITE, &vw, sizeof( vw ));
}


int CRadeon::VIPReadFifo(int device, uint32 address, uint32 count, uint8 *buffer)
{
	radeon_vip_fifo_read vr;
	status_t res;
	
	vr.magic = RADEON_PRIVATE_DATA_MAGIC;
	vr.channel = device;
	vr.address = address;
	vr.count = count;
	vr.data = buffer;
	vr.lock = true;
	
	res = ioctl( fHandle, RADEON_VIPFIFOREAD, &vr, sizeof( vr ));
	if( res == B_OK )
		return TRUE;
	else
		return FALSE;
	
}
	
int CRadeon::VIPWriteFifo(int device, uint32 address, uint32 count, uint8 *buffer)
{
	radeon_vip_fifo_write vw;
	status_t res;
	
	vw.magic = RADEON_PRIVATE_DATA_MAGIC;
	vw.channel = device;
	vw.address = address;
	vw.count = count;
	vw.data = buffer;
	vw.lock = true;
	
	res = ioctl( fHandle, RADEON_VIPFIFOWRITE, &vw, sizeof( vw ));
	
	if( res == B_OK )
		return TRUE;
	else
		return FALSE;
}


int CRadeon::FindVIPDevice( uint32 device_id )
{
	radeon_find_vip_device fvd;
	status_t res;
	
	fvd.magic = RADEON_PRIVATE_DATA_MAGIC;
	fvd.device_id = device_id;
	
	res = ioctl( fHandle, RADEON_FINDVIPDEVICE, &fvd, sizeof( fvd ));
	
	if( res == B_OK )
		return fvd.channel;
	else
		return -1;
}

shared_info* CRadeon::GetSharedInfo()
{
	return fSharedInfo;
}

void CRadeon::GetPLLParameters(int & refFreq, int & refDiv, int & minFreq, int & maxFreq, int & xclock)
{
	refFreq = fSharedInfo->pll.ref_freq;
	refDiv = fSharedInfo->pll.ref_div;
	minFreq = fSharedInfo->pll.min_pll_freq;
	maxFreq = fSharedInfo->pll.max_pll_freq;
	xclock = fSharedInfo->pll.xclk;
}

void CRadeon::GetMMParameters(radeon_video_tuner & tuner,
							  radeon_video_decoder & video,
							  radeon_video_clock & clock,
							  int & tunerPort,
							  int & compositePort,
							  int & svideoPort)
{

	unsigned char * fMMTable = NULL;

	PRINT(("CRadeon::GetMMParameters()\n"));

	if (fSharedInfo->is_atombios) {
		uint16 hdr  = fROM[0x48] + (fROM[0x49] << 8);
		uint16 PCIR = hdr + fROM[hdr] + (fROM[hdr + 1] << 8);
		uint16 hdr2 = fROM[PCIR - 4] + (fROM[PCIR - 3] << 8);
		uint16 hmMedia = fROM[hdr2 + 8] + (fROM[hdr2 + 9] << 8);
		if(fROM[hmMedia    ] == 0x14
		&& fROM[hmMedia + 1] == 0x00
		&& fROM[hmMedia + 2] == 0x01
		&& fROM[hmMedia + 3] == 0x01
		&& fROM[hmMedia + 4] == '$'
		&& fROM[hmMedia + 5] == 'M'
		&& fROM[hmMedia + 6] == 'M'
		&& fROM[hmMedia + 7] == 'T') {
			fMMTable = &fROM[hmMedia + 8];
			PRINT(("ATOMBIOS MM Table Signiture found\n"));
		} else {
			PRINT(("ATOMBIOS MM Table Not Found\n"));
			return;
		}
	} else {
		unsigned char *fVideoBIOS = fROM + fROM[0x48] + (fROM[0x49] << 8);
		fMMTable = fROM + fVideoBIOS[0x38] + (fVideoBIOS[0x39] << 8) - 2;
		
		if (fMMTable[0] != 0x0c)
		{
			PRINT(("MM_TABLE invalid size\n"));
			return;
		}
		else
		{
			PRINT(("MM Table Found (non ATOM) \n"));
			PRINT(("Revision      %02x\n", fMMTable[0]));
			PRINT(("Size          %02x\n", fMMTable[2]));
			fMMTable += 2;
		}
	}

	
	// check table:
	PRINT(( "MM_TABLE:\n"));
	const char* names[] = {
			"Tuner Type    %02x\n",
			"Audio Chip    %02x\n",
			"Product ID    %02x\n",
			"Tuner misc    %02x\n",
			"I2C Config    %02x\n",
			"Vid Decoder   %02x\n",
			"..Host config %02x\n",
			"input 0       %02x\n",
			"input 1       %02x\n",
			"input 2       %02x\n",
			"input 3       %02x\n",
			"input 4       %02x\n",
			0
	};
	
	int i = 0;	
	while(names[i]) {
		PRINT((names[i], fMMTable[i]));
		i++;
	}		
		
	switch (fMMTable[0] & 0x1f) {
	case 0x00:
		tuner = C_RADEON_NO_TUNER;
		PRINT(("CRadeon::GetMMParameters() No Tuner\n"));
		break;
	case 0x01:
		tuner = C_RADEON_FI1236_MK1_NTSC;
		break;
	case 0x02:
		tuner = C_RADEON_FI1236_MK2_NTSC_JAPAN;
		break;
	case 0x03:
		tuner = C_RADEON_FI1216_MK2_PAL_BG;
		break;
	case 0x04:
		tuner = C_RADEON_FI1246_MK2_PAL_I;
		break;
	case 0x05:
		tuner = C_RADEON_FI1216_MF_MK2_PAL_BG_SECAM_L;
		break;
	case 0x06:
		tuner = C_RADEON_FI1236_MK2_NTSC;
		break;
	case 0x07:
		tuner = C_RADEON_FI1256_MK2_SECAM_DK;
		break;
	case 0x08:
		tuner = C_RADEON_FI1236_MK2_NTSC;
		break;
	case 0x09:
		tuner = C_RADEON_FI1216_MK2_PAL_BG;
		break;
	case 0x0a:
		tuner = C_RADEON_FI1246_MK2_PAL_I;
		break;
	case 0x0b:
		tuner = C_RADEON_FI1216_MK2_PAL_BG_SECAM_L;
		break;
	case 0x0c:
		tuner = C_RADEON_FI1236_MK2_NTSC;
		break;
	case 0x0d:
		tuner = C_RADEON_TEMIC_FN5AL_PAL_IBGDK_SECAM_DK;
		break;
	default:
		tuner = C_RADEON_NO_TUNER;
		PRINT(("CRadeon::GetMMParameters() No Tuner\n"));
		break;
	}
	
	switch (fMMTable[5] & 0x0f) {
	case 0x00:
		video = C_RADEON_NO_VIDEO;
		PRINT(("CRadeon::GetMMParameters() No Video\n"));
		break;
	case 0x01:
		video = C_RADEON_BT819;
		break;
	case 0x02:
		video = C_RADEON_BT829;
		break;
	case 0x03:
		video = C_RADEON_BT829A;
		break;
	case 0x04:
		video = C_RADEON_SA7111;
		break;
	case 0x05:
		video = C_RADEON_SA7112;
		break;
	case 0x06:
		video = C_RADEON_RAGE_THEATER;
		PRINT(("CRadeon::GetMMParameters() Rage Theater\n"));
		break;
	default:
		video = C_RADEON_NO_VIDEO;
		PRINT(("CRadeon::GetMMParameters() No Video\n"));
		break;
	}
	
	switch (fMMTable[5] & 0xf0) {
	case 0x00:
	case 0x10:
	case 0x20:
	case 0x30:
		clock = C_RADEON_NO_VIDEO_CLOCK;
		PRINT(("CRadeon::GetMMParameters() Video No Clock\n"));
		break;
	case 0x40:
		clock = C_RADEON_VIDEO_CLOCK_28_63636_MHZ;
		break;
	case 0x50:
		clock = C_RADEON_VIDEO_CLOCK_29_49892_MHZ;
		break;
	case 0x60:
		clock = C_RADEON_VIDEO_CLOCK_27_00000_MHZ;
		break;
	case 0x70:
		clock = C_RADEON_VIDEO_CLOCK_14_31818_MHZ;
		break;
	default:
		clock = C_RADEON_NO_VIDEO_CLOCK;
		PRINT(("CRadeon::GetMMParameters() Video No Clock\n"));
		break;
	}

	for (int port = 0; port < 5; port++) {	
		switch (fMMTable[7 + port] & 0x03) {
		case 0x00:
			// Unused or Invalid
			PRINT(("CRadeon::GetMMParameters() Invalid Port\n"));
			break;
		case 0x01:
			// Tuner Input
			PRINT(("CRadeon::GetMMParameters() Tuner Port\n"));
			tunerPort = 0;
			break;
		case 0x02:
			// Front/Rear Composite Input
			PRINT(("CRadeon::GetMMParameters() Composite Port\n"));
			compositePort = (fMMTable[7 + port] & 0x04 ? 2 : 1);
			break;
		case 0x03:
			// Front/Rear SVideo Input
			PRINT(("CRadeon::GetMMParameters() SVideo Port\n"));
			svideoPort = (fMMTable[7 + port] & 0x04 ? 6 : 5);
			break;
		}
	}
}

status_t CRadeon::AllocateGraphicsMemory( 
	memory_type_e memory_type, int32 size,
	int32 *offset, int32 *handle )
{
	radeon_alloc_mem am;
	status_t res;
	
	am.magic = RADEON_PRIVATE_DATA_MAGIC;
	am.size = size;
	am.memory_type = mt_local;
	am.global = false;
	
	res = ioctl( fHandle, RADEON_ALLOC_MEM, &am );
	
	if( res != B_OK )
		return res;
		
	*handle = am.handle;
	*offset = am.offset;
	return B_OK;
}
		
void CRadeon::FreeGraphicsMemory( 
	memory_type_e memory_type, int32 handle )
{
	radeon_free_mem fm;
	
	fm.magic = RADEON_PRIVATE_DATA_MAGIC;
	fm.memory_type = memory_type;
	fm.global = false;
	fm.handle = handle;

	ioctl( fHandle, RADEON_FREE_MEM, &fm );
}

status_t CRadeon::DMACopy( 
	uint32 src, void *target, size_t size, bool lock_mem, bool contiguous )
{
	radeon_dma_copy dc;
	
	dc.magic = RADEON_PRIVATE_DATA_MAGIC;
	dc.src = src;
	dc.target = target;
	dc.size = size;
	dc.lock_mem = lock_mem;
	dc.contiguous = contiguous;

	return ioctl( fHandle, RADEON_DMACOPY, &dc );
}

status_t CRadeon::GetDeviceInformation(radeon_get_private_data & info)
{
	info.magic = RADEON_PRIVATE_DATA_MAGIC;
			
	return ioctl( fHandle, RADEON_GET_PRIVATE_DATA, &info, sizeof( info ));
}

status_t CRadeon::CloneArea(const char * name, area_id src_area, 
	area_id *cloned_area, void ** map)
{
	int res = clone_area( name, map, B_ANY_ADDRESS,
		B_READ_AREA | B_WRITE_AREA, src_area );
		
	if( res < 0 ) {
		return res;
	} else {
		*cloned_area = res;
		return B_OK;
	}
}

status_t CRadeon::WaitInterrupt(int * mask, int * sequence, bigtime_t * time, bigtime_t timeout)
{
	radeon_wait_for_cap_irq wvc;
	status_t status;
	
	wvc.magic = RADEON_PRIVATE_DATA_MAGIC;
	wvc.timeout = timeout;
	
	status = ioctl( fHandle, RADEON_WAIT_FOR_CAP_IRQ, &wvc );
	
	if( status == B_OK ) {
		*mask = wvc.int_status;
		*sequence = wvc.counter;
		*time = wvc.timestamp;
	}
	
	return status;
}

#if 0
void CRadeon::PrintToStream()
{
	// ATI ROM Signature
	if (ROM(0) == 0x55 && ROM(1) == 0xAA) {
		for (int offset = 0; offset < 128 - 9; offset++) {
			if (ROM(offset + 0) == '7' &&
				ROM(offset + 1) == '6' &&
				ROM(offset + 2) == '1' &&
				ROM(offset + 3) == '2' &&
				ROM(offset + 4) == '9' &&
				ROM(offset + 5) == '5' &&
				ROM(offset + 6) == '5' &&
				ROM(offset + 7) == '2' &&
				ROM(offset + 8) == '0')
				break;
		}
	}

	// Video BIOS
	unsigned char *fVideoBIOS = fROM + fROM[0x48] + (fROM[0x49] << 8);
	
	PRINT((
		"----------------------------------------------------------------------\n"
        "ATI RADEON VIDEO BIOS\n"
        "\n"
        "BIOS Revision: %03d.%03d.%03d%03d.%s\n"
        "PCI Bus/Device/Function Code: 0x%04x\n"
        "BIOS Runtime Segment Address: 0x%04x\n"
        "I/O Base Address: 0x%04x\n"
        "Subsystem Vendor ID: 0x%04x\n"
        "Subsystem ID: 0x%04x\n"
        "Post Vendor ID: 0x%04x\n"
        "\n",

        // OEM Revision (ID1.ID2.REVISION.CONFIG_FILE)
        fVideoBIOS[2], fVideoBIOS[3],
        fVideoBIOS[4], fVideoBIOS[5],
        fROM + fVideoBIOS[0x10] + (fVideoBIOS[0x11] << 8),

        // PCI bus, device, function code
        fVideoBIOS[0x16] + (fVideoBIOS[0x17] << 8),

        // ROM BIOS segment
        fVideoBIOS[0x18] + (fVideoBIOS[0x19] << 8),

        // I/O base address
        fVideoBIOS[0x1a] + (fVideoBIOS[0x1b] << 8),

        // Subsystem Vendor ID, Subsystem ID, Post Vendor ID
        fVideoBIOS[0x1c] + (fVideoBIOS[0x1d] << 8),
        fVideoBIOS[0x1e] + (fVideoBIOS[0x1f] << 8),
        fVideoBIOS[0x20] + (fVideoBIOS[0x21] << 8)
    ));


	// PLL Information
	unsigned char *fPLL = fROM + fVideoBIOS[0x30] + (fVideoBIOS[0x31] << 8);
	
	PRINT((
		"----------------------------------------------------------------------\n"
        "ATI RADEON PLL INFORMATION TABLE\n"
        "\n"
        "External Clock: %g MHz\n"
        "Reference Frequency: %g MHz\n"
        "Reference Divisor: %d\n"
        "Min PLL Frequency: %g MHz\n"
        "Max PLL Frequency: %g MHz\n"
        "\n",
		(fPLL[0x08] + (fPLL[0x09] << 8)) / 1000.0,
		(fPLL[0x0e] + (fPLL[0x0f] << 8)) / 100.0,
		fPLL[0x10] + (fPLL[0x11] << 8),
		(fPLL[0x12] + (fPLL[0x13] << 8) + (fPLL[0x14] << 16) + (fPLL[0x15] << 24)) / 1000.0,
		(fPLL[0x12] + (fPLL[0x16] << 8) + (fPLL[0x17] << 16) + (fPLL[0x18] << 24)) / 1000.0));


	// TV Table
	unsigned char * fTVTable = fROM + fVideoBIOS[0x32] + (fVideoBIOS[0x33] << 8);

	PRINT((
		"----------------------------------------------------------------------\n"
        "ATI RADEON TV INFORMATION TABLE\n"
        "\n"
        "Table Signature: %c%c%c\n"
        "Table Version: %d\n"
        "Table Size: %d bytes\n"
        "\n"
        "TVOut Support: %s\n"
        "BIOS built-in TV standard: %s\n"
        "TVOut information: %s, %s MHz\n"
        "\n"
        "Run time supported TV standard:%s%s%s%s%s%s\n"
        "Initialization time supported TV standard:%s%s%s%s%s%s\n"
        "\n",

        // Table signature, version and size
        fTVTable[0x00],fTVTable[0x01], fTVTable[0x02],
        fTVTable[0x03],
        fTVTable[0x04] + ((int) fTVTable[0x05] << 8),

        // TVOut support
        fTVTable[0x06] == 'N' ? "TVOut chip not found" : "TVOut chip on board",

        // BIOS built-in initialization TV standard
        (fTVTable[0x07] & 0x0f) == 0x01 ? "NTSC" :
        (fTVTable[0x07] & 0x0f) == 0x02 ? "PAL" :
        (fTVTable[0x07] & 0x0f) == 0x03 ? "PAL-M" :
        (fTVTable[0x07] & 0x0f) == 0x04 ? "PAL-60" :
        (fTVTable[0x07] & 0x0f) == 0x05 ? "NTSC-J" :
        (fTVTable[0x07] & 0x0f) == 0x06 ? "SCART-PAL" : "Reserved",

        // TV Out information
        (fTVTable[0x09] & 0x03) == 0x00 ? "Invalid" :
        (fTVTable[0x09] & 0x03) == 0x01 ? "TV off, CRT on" :
        (fTVTable[0x09] & 0x03) == 0x02 ? "TV on, CRT off" : "TV on, CRT on",

        (fTVTable[0x09] & 0x0c) == 0x00 ? "29.498928713" :
        (fTVTable[0x09] & 0x0c) == 0x04 ? "28.63636" :
        (fTVTable[0x09] & 0x0c) == 0x08 ? "14.31818" : "27.0",

        // Runtime supported TV standard
        (fTVTable[0x0a] & 0x01) != 0 ? " NTSC" : "",
        (fTVTable[0x0a] & 0x02) != 0 ? " PAL" : "",
        (fTVTable[0x0a] & 0x04) != 0 ? " PAL-M" : "",
        (fTVTable[0x0a] & 0x08) != 0 ? " PAL-60" : "",
        (fTVTable[0x0a] & 0x10) != 0 ? " NTSC-J" : "",
        (fTVTable[0x0a] & 0x20) != 0 ? " SCART-PAL" : "",

        // Initialization time supported TV standard
        (fTVTable[0x0b] & 0x01) != 0 ? " NTSC" : "",
        (fTVTable[0x0b] & 0x02) != 0 ? " PAL" : "",
        (fTVTable[0x0b] & 0x04) != 0 ? " PAL-M" : "",
        (fTVTable[0x0b] & 0x08) != 0 ? " PAL-60" : "",
        (fTVTable[0x0b] & 0x10) != 0 ? " NTSC-J" : "",
        (fTVTable[0x0b] & 0x20) != 0 ? " SCART-PAL" : ""
    ));

    // Hardware Configuration Table
    unsigned char * fHWTable = fROM + fVideoBIOS[0x36] + (fVideoBIOS[0x37] << 8);

	PRINT((
		"----------------------------------------------------------------------\n"
        "ATI RADEON HARDWARE CONFIGURATION TABLE\n"
        "\n"
        "Table Signature: %c%c%c%c\n"
        "Table Revision: %d\n"
        "Table Size: %d\n"
        "\n"
        "I2C Type: %s\n"
        "TVOut Support: %s\n"
        "Video Out Crystal: %s\n"
        "ImpactTV Data Port: %s\n"
        "\n"
        "Video Port Capability:\n"
        "   AMC/DVS0 Video Port: %s\n"
        "   Zoom Video Port: %s\n"
        "   AMC/DVS1 Video Port: %s\n"
        "   VIP 16-bit Video Port: %s\n"
        "\n"
        "Host Port Configuration: %s\n"
        "\n",

        // Table Signature, Revision, Size
        fHWTable[0x00], fHWTable[0x01], fHWTable[0x02], fHWTable[0x03],
        fHWTable[0x04], fHWTable[0x05],

        // I2C type
        (fHWTable[0x06] & 0x0f) == 0x00 ? "Normal GP I/O (data=GP_IO2, clock=GP_IO1)" :
        (fHWTable[0x06] & 0x0f) == 0x01 ? "ImpacTV GP I/O" :
        (fHWTable[0x06] & 0x0f) == 0x02 ? "Dedicated I2C Pin" :
        (fHWTable[0x06] & 0x0f) == 0x03 ? "GP I/O (data=GP_IO12, clock=GP_IO13)" :
        (fHWTable[0x06] & 0x0f) == 0x04 ? "GP I/O (data=GPIO12, clock=GPIO10)" :
        (fHWTable[0x06] & 0x0f) == 0x05 ? "RAGE THEATER I2C Master" :
        (fHWTable[0x06] & 0x0f) == 0x06 ? "Rage128 MPP2 Pin" :
        (fHWTable[0x06] & 0x0f) == 0x0f ? "No I2C Configuration" : "Reserved",

        // TVOut support
        (fHWTable[0x07] & 0x0f) == 0x00 ? "No TVOut supported" :
        (fHWTable[0x07] & 0x0f) == 0x01 ? "ImpactTV1 supported" :
        (fHWTable[0x07] & 0x0f) == 0x02 ? "ImpactTV2 supported" :
        (fHWTable[0x07] & 0x0f) == 0x03 ? "Improved ImpactTV2 supported" :
        (fHWTable[0x07] & 0x0f) == 0x04 ? "RAGE THEATER supported" : "Reserved",

        // Video Out Crystal
        (fHWTable[0x07] & 0x70) == 0x00 ? "TVOut not installed" :
        (fHWTable[0x07] & 0x70) == 0x10 ? "28.63636 MHz" :
        (fHWTable[0x07] & 0x70) == 0x20 ? "29.49892713 MHz" :
        (fHWTable[0x07] & 0x70) == 0x30 ? "27.0 MHz" :
        (fHWTable[0x07] & 0x70) == 0x40 ? "14.31818 MHz" : "Reserved",

        // ImpactTV data port
        (fHWTable[0x07] & 0x80) == 0x00 ? "MPP1" : "MPP2",

        // Video Port Capability
        (fHWTable[0x08] & 0x01) == 0x00 ? "Not Supported" : "Supported",
        (fHWTable[0x08] & 0x02) == 0x00 ? "Not Supported" : "Supported",
        (fHWTable[0x08] & 0x04) == 0x00 ? "Not Supported" : "Supported",
        (fHWTable[0x08] & 0x08) == 0x00 ? "Not Supported" : "Supported",

        // Host Port Configuration
        (fHWTable[0x09] & 0x0f) == 0x00 ? "No Host Port" :
        (fHWTable[0x09] & 0x0f) == 0x01 ? "MPP Host Port" :
        (fHWTable[0x09] & 0x0f) == 0x02 ? "2 bit VIP Host Port" :
        (fHWTable[0x09] & 0x0f) == 0x03 ? "4 bit VIP Host Port" :
        (fHWTable[0x09] & 0x0f) == 0x04 ? "8 bit VIP Host Port" : "Reserved"
    ));

	// Multimedia Table
	unsigned char * fMMTable = fROM + fVideoBIOS[0x38] + (fVideoBIOS[0x39] << 8);

    PRINT((
    	"----------------------------------------------------------------------\n"
        "ATI RADEON MULTIMEDIA TABLE\n"
        "\n"
        "Table Revision: %d\n"
        "Table Size: %d bytes\n"
        "\n"
        "Tuner Chip: %s\n"
        "Tuner Input: %s\n"
        "Tuner Voltage Regulator: %s\n"
        "\n"
        "Audio Chip: %s\n"
        "FM Audio Decoder: %s\n"
        "Audio Scrambling: %s\n"
        "\n"
        "Product Type: %s, Revision %d\n"
        "Product ID: %s\n"
        "\n"
        "I2S Input Configuration: %s\n"
        "I2S Output Configuration: %s\n"
        "I2S Audio Chip: %s\n"
        "S/PDIF Output Configuration: %s\n"
        "\n"
        "Video Decoder: %s\n"
        "Video Standard/Crystal: %s\n"
        "Video Decoder Host Config: %s\n"
        "Hardware Teletext: %s\n"
        "\n"
        "Video Input:\n"
        "    0: %s, %s (ID %d)\n"
        "    1: %s, %s (ID %d)\n"
        "    2: %s, %s (ID %d)\n"
        "    3: %s, %s (ID %d)\n"
        "    4: %s, %s (ID %d)\n"
        "\n",

        /* Hardware Table */
        fMMTable[-2], fMMTable[-1],

        /* Tuner Type */
        (fMMTable[0] & 0x1f) == 0x00 ? "No Tuner" :
        (fMMTable[0] & 0x1f) == 0x01 ? "Philips FI1236 MK1 NTSC M/N North America" :
        (fMMTable[0] & 0x1f) == 0x02 ? "Philips FI1236 MK2 NTSC M/N Japan" :

        (fMMTable[0] & 0x1f) == 0x03 ? "Philips FI1216 MK2 PAL B/G" :
        (fMMTable[0] & 0x1f) == 0x04 ? "Philips FI1246 MK2 PAL I" :
        (fMMTable[0] & 0x1f) == 0x05 ? "Philips FI1216 MF MK2 PAL B/G, SECAM L/L'" :
        (fMMTable[0] & 0x1f) == 0x06 ? "Philips FI1236 MK2 NTSC M/N North America" :
        (fMMTable[0] & 0x1f) == 0x07 ? "Philips FI1256 MK2 SECAM D/K" :
        (fMMTable[0] & 0x1f) == 0x08 ? "Philips FM1236 MK2 NTSC M/N North America" :

        (fMMTable[0] & 0x1f) == 0x09 ? "Philips FI1216 MK2 PAL B/G - External Tuner POD" :
        (fMMTable[0] & 0x1f) == 0x0a ? "Philips FI1246 MK2 PAL I - External Tuner POD" :
        (fMMTable[0] & 0x1f) == 0x0b ? "Philips FI1216 MF MK2 PAL B/G, SECAM L/L' - External Tuner POD" :
        (fMMTable[0] & 0x1f) == 0x0c ? "Philips FI1236 MK2 NTSC M/N North America - External Tuner POD" :

        (fMMTable[0] & 0x1f) == 0x0d ? "Temic FN5AL RF3X7595 PAL I/B/G/DK & SECAM DK" :
        (fMMTable[0] & 0x1f) == 0x10 ? "Alps TSBH5 NTSC M/N North America" :
        (fMMTable[0] & 0x1f) == 0x11 ? "Alps TSC?? NTSC M/N North America" :
        (fMMTable[0] & 0x1f) == 0x12 ? "Alps TSCH5 NTSC M/N North America" :

        (fMMTable[0] & 0x1f) == 0x1f ? "Unknown Tuner Type" : "Reserved",

        /* Video Input for Tuner */
        (fMMTable[0] & 0xe0) == 0x00 ? "Video Input 0" :
        (fMMTable[0] & 0xe0) == 0x20 ? "Video Input 1" :
        (fMMTable[0] & 0xe0) == 0x40 ? "Video Input 2" :
        (fMMTable[0] & 0xe0) == 0x60 ? "Video Input 3" :
        (fMMTable[0] & 0xe0) == 0x80 ? "Video Input 4" : "Reserved",

        /* Tuner Voltage */
        (fMMTable[3] & 0x03) == 0x00 ? "No Tuner Power down feature" :
        (fMMTable[3] & 0x03) == 0x01 ? "Tuner Power down feature" : "Reserved",

        /* Audio Chip Type */
        (fMMTable[1] & 0x0f) == 0x00 ? "Philips TEA5582 NTSC Stereo, no dbx, no volume" :
        (fMMTable[1] & 0x0f) == 0x01 ? "Mono with audio mux" :
        (fMMTable[1] & 0x0f) == 0x02 ? "Philips TDA9850 NTSC N.A. Stereo, dbx, mux, no volume" :
        (fMMTable[1] & 0x0f) == 0x03 ? "Sony CXA2020S Japan NTSC Stereo, mux, no volume" :
        (fMMTable[1] & 0x0f) == 0x04 ? "ITT MSP3410D Europe Stereo, volume, internal mux" :
        (fMMTable[1] & 0x0f) == 0x05 ? "Crystal CS4236B" :
        (fMMTable[1] & 0x0f) == 0x06 ? "Philips TDA9851 NTSC Stereo, volume, no dbx, no mux" :
        (fMMTable[1] & 0x0f) == 0x07 ? "ITT MSP3415 (Europe)" :
        (fMMTable[1] & 0x0f) == 0x08 ? "ITT MSP3430 (N.A.)" :
        (fMMTable[1] & 0x0f) == 0x0f ? "No Audio Chip Installed" : "Reserved",

        /* FM Audio Decoder */
        (fMMTable[3] & 0x30) == 0x00 ? "No FM Audio Decoder" :
        (fMMTable[3] & 0x30) == 0x10 ? "FM Audio Decoder (Rohm BA1332F)" : "Reserved",

        /* Audio Scrambling */
        (fMMTable[3] & 0x80) == 0x00 ? "Not Supported" : "Supported",

        /* Product Type */
        (fMMTable[1] & 0x10) == 0x00 ? "OEM Product" : "ATI Product",

        /* OEM Revision */
        (fMMTable[1] & 0xe0) >> 5,

        /* Product ID */
        (fMMTable[1] & 0x10) == 0x00 ? "<OEM ID>" :
        (
            fMMTable[2] == 0x00 ? "ATI Prototype Board" :
            fMMTable[2] == 0x01 ? "ATI All in Wonder" :
            fMMTable[2] == 0x02 ? "ATI All in Wonder Pro, no MPEG/DVD decoder" :
            fMMTable[2] == 0x03 ? "ATI All in Wonder Pro, CD11 or similar MPEG/DVD decoder on MPP" :
            fMMTable[2] == 0x04 ? "ATI All in Wonder Plus" :
            fMMTable[2] == 0x05 ? "ATI Kitchener Board" :
            fMMTable[2] == 0x06 ? "ATI Toronto Board (analog audio)" :
            fMMTable[2] == 0x07 ? "ATI TV-Wonder" :
            fMMTable[2] == 0x08 ? "ATI Victoria Board (Rage XL plus RAGE THEATER)" : "Reserved"
        ),

        /* I2S Input/Output Configuration */
        (fMMTable[4] & 0x01) == 0x00 ? "Not Supported" : "Supported",
        (fMMTable[4] & 0x02) == 0x00 ? "Not Supported" : "Supported",

        /* I2S Audio Chip */
        (fMMTable[4] & 0x1c) == 0x00 ? "TDA1309_32Strap" :
        (fMMTable[4] & 0x1c) == 0x04 ? "TDA1309_64Strap" :
        (fMMTable[4] & 0x1c) == 0x08 ? "ITT MSP3430" :
        (fMMTable[4] & 0x1c) == 0x0c ? "ITT MSP3415" : "Reserved",

        /* S/PDIF Output Config */
        (fMMTable[4] & 0x20) == 0x00 ? "Not Supported" : "Supported",

        /* Video Decoder Type */
        (fMMTable[5] & 0x0f) == 0x00 ? "No Video Decoder" :
        (fMMTable[5] & 0x0f) == 0x01 ? "Bt819" :
        (fMMTable[5] & 0x0f) == 0x02 ? "Bt829" :
        (fMMTable[5] & 0x0f) == 0x03 ? "Bt829A" :
        (fMMTable[5] & 0x0f) == 0x04 ? "Philips SA7111" :
        (fMMTable[5] & 0x0f) == 0x05 ? "Philips SA7112 or SA7112A" :
        (fMMTable[5] & 0x0f) == 0x06 ? "RAGE THEATER" : "Reserved",

        /* Video-In Standard/Crystal */
        (fMMTable[5] & 0xf0) == 0x00 ? "NTSC and PAL Crystals installed" :
        (fMMTable[5] & 0xf0) == 0x10 ? "NTSC Crystal only" :
        (fMMTable[5] & 0xf0) == 0x20 ? "PAL Crystal only" :
        (fMMTable[5] & 0xf0) == 0x30 ? "NTSC, PAL, SECAM Ssingle crystal for Bt829 and Bt879" :
        (fMMTable[5] & 0xf0) == 0x40 ? "28.63636 MHz Crystal" :
        (fMMTable[5] & 0xf0) == 0x50 ? "29.49892713 MHz Crystal" :
        (fMMTable[5] & 0xf0) == 0x60 ? "27.0 MHz Crystal" :
        (fMMTable[5] & 0xf0) == 0x70 ? "14.31818 MHz Crystal" : "Reserved",

        /* Video Decoder Host Config */
        (fMMTable[6] & 0x07) == 0x00 ? "I2C Device" :
        (fMMTable[6] & 0x07) == 0x01 ? "MPP Device" :
        (fMMTable[6] & 0x07) == 0x02 ? "2 bits VIP Device" :
        (fMMTable[6] & 0x07) == 0x03 ? "4 bits VIP Device" :
        (fMMTable[6] & 0x07) == 0x04 ? "8 bits VIP Device" :
        (fMMTable[6] & 0x07) == 0x07 ? "PCI Device" : "Reserved",

        /* Hardware Teletext */
        (fMMTable[3] & 0x0c) == 0x00 ? "No Hardware Teletext" :
        (fMMTable[3] & 0x0c) == 0x04 ? "Philips SAA5281" : "Reserved",

        /* Video Input 0 */
        (fMMTable[7] & 0x03) == 0x00 ? "Unused/Invalid" :
        (fMMTable[7] & 0x03) == 0x01 ? "Tuner Input" :
        (fMMTable[7] & 0x03) == 0x02 ? "Composite Input" : "S-Video Input",
        (fMMTable[7] & 0x04) == 0x00 ? "Front Connector" : "Rear Connector",
        (fMMTable[7] & 0x38) >> 3,

        /* Video Input 1 */
        (fMMTable[8] & 0x03) == 0x00 ? "Unused/Invalid" :
        (fMMTable[8] & 0x03) == 0x01 ? "Tuner Input" :
        (fMMTable[8] & 0x03) == 0x02 ? "Composite Input" : "S-Video Input",
        (fMMTable[8] & 0x04) == 0x00 ? "Front Connector" : "Rear Connector",
        (fMMTable[8] & 0x38) >> 3,

        /* Video Input 2 */
        (fMMTable[9] & 0x03) == 0x00 ? "Unused/Invalid" :
        (fMMTable[9] & 0x03) == 0x01 ? "Tuner Input" :
        (fMMTable[9] & 0x03) == 0x02 ? "Composite Input" : "S-Video Input",
        (fMMTable[9] & 0x04) == 0x00 ? "Front Connector" : "Rear Connector",
        (fMMTable[9] & 0x38) >> 3,

        /* Video Input 3 */
        (fMMTable[10] & 0x03) == 0x00 ? "Unused/Invalid" :
        (fMMTable[10] & 0x03) == 0x01 ? "Tuner Input" :
        (fMMTable[10] & 0x03) == 0x02 ? "Composite Input" : "S-Video Input",
        (fMMTable[10] & 0x04) == 0x00 ? "Front Connector" : "Rear Connector",
        (fMMTable[10] & 0x38) >> 3,

        /* Video Input 4 */
        (fMMTable[11] & 0x03) == 0x00 ? "Unused/Invalid" :
        (fMMTable[11] & 0x03) == 0x01 ? "Tuner Input" :
        (fMMTable[11] & 0x03) == 0x02 ? "Composite Input" : "S-Video Input",
        (fMMTable[11] & 0x04) == 0x00 ? "Front Connector" : "Rear Connector",
        (fMMTable[11] & 0x38) >> 3
    ));
}
#endif
