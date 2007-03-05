/******************************************************************************
/
/	File:			Theater.cpp
/
/	Description:	ATI Rage Theater Video Decoder interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <Debug.h>
#include "Theater.h"
#include "Theater200.h"
#include "TheatreReg.h"
#include "lendian_bitfield.h"
#include <stdio.h>
#include <stdlib.h>
#include <OS.h>


const char* DEFAULT_MICROC_PATH = "/boot/home/config/settings/Media/RageTheater200/ativmc20.cod";
const char* DEFAULT_MICROC_TYPE = "BINARY";

CTheater200::CTheater200(CRadeon & radeon, int device)
		:CTheater(radeon, device),
		fMode(MODE_UNINITIALIZED),
		microcode_path(NULL),
		microcode_type(NULL)
		
{
	PRINT(("CTheater200::CTheater200()\n"));

	fMode = MODE_UNINITIALIZED;
	
	if( fPort.InitCheck() == B_OK ) {
		radeon_video_tuner tuner;
		radeon_video_decoder video;
		
		radeon.GetMMParameters(tuner, video, fClock,
			fTunerPort, fCompositePort, fSVideoPort);
		
		if (fClock != C_RADEON_VIDEO_CLOCK_29_49892_MHZ &&
			fClock != C_RADEON_VIDEO_CLOCK_27_00000_MHZ)
			PRINT(("CTheater200::CTheater200() - Unsupported crystal clock!\n"));

		// fDevice = fPort.FindVIPDevice( C_THEATER200_VIP_DEVICE_ID );
		
	}
		
	if( InitCheck() != B_OK )
		PRINT(("CTheater200::CTheater200() - Rage Theater not found!\n"));
		
	InitTheatre();
	
}

CTheater200::~CTheater200()
{
	PRINT(("CTheater200::~CTheater200()\n"));
	
	if( InitCheck() == B_OK )
		SetEnable(false, false);
	
}

status_t CTheater200::InitCheck() const
{
	status_t res;
	
	res = fPort.InitCheck();
	if( res != B_OK )
	{
		PRINT(("CTheater200::InitCheck() fPort Failed\n"));
		return res;
	}
	
	res = (fDevice >= C_VIP_PORT_DEVICE_0 && fDevice <= C_VIP_PORT_DEVICE_3) ? B_OK : B_ERROR;	
	if( res != B_OK )
	{
		PRINT(("CTheater200::InitCheck() Invalid VIP Channel\n"));
		return res;
	}

	if (fMode != MODE_INITIALIZED_FOR_TV_IN);
		return B_ERROR;

	PRINT(("CTheater200::InitCheck() Sucess\n"));
	return res;
}

void CTheater200::Reset()
{
	PRINT(("CTheater200::Reset()\n"));
	
	SetHue(0);
	SetBrightness(0);
	SetSaturation(0);
	SetContrast(0);
	SetSharpness(false);
}

status_t CTheater200::DSPLoadMicrocode(char* micro_path, char* micro_type, struct rt200_microc_data* microc_datap)
{
	FILE* file;
	struct rt200_microc_head* microc_headp = &microc_datap->microc_head;
	struct rt200_microc_seg* seg_list = NULL;
	struct rt200_microc_seg* curr_seg = NULL;
	struct rt200_microc_seg* prev_seg = NULL;
	uint32 i;

	if (micro_path == NULL)
		return -1;

	if (micro_type == NULL)
		return -1;

	file = fopen(micro_path, "r");
	if (file == NULL) {
		PRINT(("Cannot open microcode file\n"));
		return -1;
	}

	if (!strcmp(micro_type, "BINARY"))
	{
		if (fread(microc_headp, sizeof(struct rt200_microc_head), 1, file) != 1)
		{
			PRINT(("Cannot read header from file: %s\n", micro_path));
			goto fail_exit;
		}

		PRINT(("Microcode: num_seg: %x\n", microc_headp->num_seg));

		if (microc_headp->num_seg == 0)
			goto fail_exit;
		
		for (i = 0; i < microc_headp->num_seg; i++)
		{
			int ret;
			
			curr_seg = (struct rt200_microc_seg*) malloc(sizeof(struct rt200_microc_seg));
			if (curr_seg == NULL)
			{
				PRINT(("Cannot allocate memory\n"));
				goto fail_exit;
			}

			ret = fread(&curr_seg->num_bytes, 4, 1, file);
			ret += fread(&curr_seg->download_dst, 4, 1, file);
			ret += fread(&curr_seg->crc_val, 4, 1, file);
			if (ret != 3)
			{
				PRINT(("Cannot read segment from microcode file: %s\n", micro_path));
				goto fail_exit;
			}

			curr_seg->data = (unsigned char*) malloc(curr_seg->num_bytes);
			if (curr_seg->data == NULL)
			{
				PRINT(("cannot allocate memory\n"));
				goto fail_exit;
			}

			PRINT(("Microcode: segment number: %x\n", i));
			PRINT(("Microcode: curr_seg->num_bytes: %x\n", curr_seg->num_bytes));
			PRINT(("Microcode: curr_seg->download_dst: %x\n", curr_seg->download_dst));
			PRINT(("Microcode: curr_seg->crc_val: %x\n", curr_seg->crc_val));

			if (seg_list)
			{
				prev_seg->next = curr_seg;
				curr_seg->next = NULL;
				prev_seg = curr_seg;
			}
			else
				seg_list = prev_seg = curr_seg;

		}
	
		curr_seg = seg_list;
		while (curr_seg)
		{
			if ( fread(curr_seg->data, curr_seg->num_bytes, 1, file) != 1 )
			{
				PRINT(("Cannot read segment data\n"));
				goto fail_exit;
			}

			curr_seg = curr_seg->next;
		}
	}
	else if (!strcmp(micro_type, "ASCII"))
	{
		char tmp1[12], tmp2[12], tmp3[12], tmp4[12];
		unsigned int ltmp;

		if ((fgets(tmp1, 12, file) != NULL) &&
			(fgets(tmp2, 12, file) != NULL) &&
			(fgets(tmp3, 12, file) != NULL) &&
			 fgets(tmp4, 12, file) != NULL)
		{
			microc_headp->device_id = strtoul(tmp1, NULL, 16);
			microc_headp->vendor_id = strtoul(tmp2, NULL, 16);
			microc_headp->revision_id = strtoul(tmp3, NULL, 16);
			microc_headp->num_seg = strtoul(tmp4, NULL, 16);
		}
		else
		{
			PRINT(("Cannot read header from file: %s\n", micro_path));
			goto fail_exit;
		}

		PRINT(("Microcode: num_seg: %x\n", microc_headp->num_seg));

		if (microc_headp->num_seg == 0)
			goto fail_exit;

		for (i = 0; i < microc_headp->num_seg; i++)
		{
			curr_seg = (struct rt200_microc_seg*) malloc(sizeof(struct rt200_microc_seg));
			if (curr_seg == NULL)
			{
				PRINT(("Cannot allocate memory\n"));
				goto fail_exit;
			}

			if (fgets(tmp1, 12, file) != NULL &&
				fgets(tmp2, 12, file) != NULL &&
				fgets(tmp3, 12, file) != NULL)
			{
				curr_seg->num_bytes = strtoul(tmp1, NULL, 16); 
				curr_seg->download_dst = strtoul(tmp2, NULL, 16); 
				curr_seg->crc_val = strtoul(tmp3, NULL, 16); 
			}
			else
			{
				PRINT(("Cannot read segment from microcode file: %s\n", micro_path));
				goto fail_exit;
			}
								
			curr_seg->data = (unsigned char*) malloc(curr_seg->num_bytes);
			if (curr_seg->data == NULL)
			{
				PRINT(("cannot allocate memory\n"));
				goto fail_exit;
			}

			PRINT(("Microcode: segment number: %x\n", i));
			PRINT(("Microcode: curr_seg->num_bytes: %x\n", curr_seg->num_bytes));
			PRINT(("Microcode: curr_seg->download_dst: %x\n", curr_seg->download_dst));
			PRINT(("Microcode: curr_seg->crc_val: %x\n", curr_seg->crc_val));

			if (seg_list)
			{
				curr_seg->next = NULL;
				prev_seg->next = curr_seg;
				prev_seg = curr_seg;
			}
			else
				seg_list = prev_seg = curr_seg;
		}

		curr_seg = seg_list;
		while (curr_seg)
		{
			for ( i = 0; i < curr_seg->num_bytes; i+=4)
			{

				if ( fgets(tmp1, 12, file) == NULL )
				{
					PRINT(("Cannot read from file\n"));
					goto fail_exit;
				}
				ltmp = strtoul(tmp1, NULL, 16);

				*(unsigned int*)(curr_seg->data + i) = ltmp;
			}
								  
			curr_seg = curr_seg->next;
		}

	}
	else
	{
		PRINT(("File type %s unknown\n", micro_type));
	}

	microc_datap->microc_seg_list = seg_list;

	fclose(file);
	return 0;

fail_exit:
	curr_seg = seg_list;
	while(curr_seg)
	{
		free(curr_seg->data);
		prev_seg = curr_seg;
		curr_seg = curr_seg->next;
		free(prev_seg);
	}
	fclose(file);

	return -1;
}


void CTheater200::DSPCleanMicrocode(struct rt200_microc_data* microc_datap)
{
	struct rt200_microc_seg* seg_list = microc_datap->microc_seg_list;
	struct rt200_microc_seg* prev_seg;

	while(seg_list)
	{
		free(seg_list->data);
		prev_seg = seg_list;
		seg_list = seg_list->next;
		free(prev_seg);
	}
}


status_t CTheater200::DspInit()
{
	uint32 data;
	int i = 0;

	PRINT(("CTheater200::Dsp_Init()\n"));
	
	/* Map FIFOD to DSP Port I/O port */
	data = Register(VIP_HOSTINTF_PORT_CNTL);
	SetRegister(VIP_HOSTINTF_PORT_CNTL, data & (~VIP_HOSTINTF_PORT_CNTL__FIFO_RW_MODE));

	/* The default endianess is LE. It matches the ost one for x86 */
	data = Register(VIP_HOSTINTF_PORT_CNTL);
	SetRegister(VIP_HOSTINTF_PORT_CNTL, data & (~VIP_HOSTINTF_PORT_CNTL__FIFOD_ENDIAN_SWAP));

	/* Wait until Shuttle bus channel 14 is available */
	data = Register(VIP_TC_STATUS);
	while(((data & VIP_TC_STATUS__TC_CHAN_BUSY) & 0x00004000) && (i++ < 10000))
		data = Register(VIP_TC_STATUS);
		  
	PRINT(("Microcode: dsp_init: channel 14 available\n"));
	
	return B_OK;
}

status_t CTheater200::DspLoad( struct rt200_microc_data* microc_datap )
{

	struct rt200_microc_seg* seg_list = microc_datap->microc_seg_list;
	uint8	data8;
	uint32 data, fb_scratch0, fb_scratch1;
	uint32 i;
	uint32 tries = 0;
	uint32 result = 0;
	uint32 seg_id = 0;
		  
	PRINT(("Microcode: before everything: %x\n", data8));

	if (ReadFifo(0x000, &data8))
		PRINT(("Microcode: FIFO status0: %x\n", data8));
	else
	{
		PRINT(("Microcode: error reading FIFO status0\n"));
		return -1;
	}


	if (ReadFifo(0x100, &data8))
		PRINT(("Microcode: FIFO status1: %x\n", data8));
	else
	{
		PRINT(("Microcode: error reading FIFO status1\n"));
		return -1;
	}

	/*
	 * Download the Boot Code and CRC Checking Code (first segment)
	 */
	//debugger("DSPLoad");
	seg_id = 1;
	while(result != DSP_OK && tries++ < 10)
	{
	
		/* Put DSP in reset before download (0x02) */
		data = Register(VIP_TC_DOWNLOAD);
		SetRegister(VIP_TC_DOWNLOAD, (data & ~VIP_TC_DOWNLOAD__TC_RESET_MODE) | (0x02 << 17));
					 
		/* 
		 * Configure shuttle bus for tranfer between DSP I/O "Program Interface"
		 * and Program Memory at address 0 
		 */

		SetRegister(VIP_TC_SOURCE, 0x90000000);
		SetRegister(VIP_TC_DESTINATION, 0x00000000);
		SetRegister(VIP_TC_COMMAND, 0xe0000044 | ((seg_list->num_bytes - 1) << 7));

		/* Load first segment */
		PRINT(("Microcode: Loading first segment\n"));

		if (!WriteFifo(0x700, seg_list->num_bytes, seg_list->data))
		{
			PRINT(("Microcode: write to FIFOD failed\n"));
			return -1;
		}

		/* Wait until Shuttle bus channel 14 is available */
		i = data = 0;
		data = Register(VIP_TC_STATUS);
		while(((data & VIP_TC_STATUS__TC_CHAN_BUSY) & 0x00004000) && (i++ < 10000))
			data = Register(VIP_TC_STATUS);

		if (i >= 10000)
		{
			PRINT(("Microcode: channel 14 timeout\n"));
			return -1;
		}

		PRINT(("Microcode: dsp_load: checkpoint 1\n"));
		PRINT(("Microcode: TC_STATUS: %x\n", data));

		/* transfer the code from program memory to data memory */
		SetRegister(VIP_TC_SOURCE, 0x00000000);
		SetRegister(VIP_TC_DESTINATION, 0x10000000);
		SetRegister(VIP_TC_COMMAND, 0xe0000006 | ((seg_list->num_bytes - 1) << 7));

		/* Wait until Shuttle bus channel 14 is available */
		i = data = 0;
		data = Register(VIP_TC_STATUS);
		while(((data & VIP_TC_STATUS__TC_CHAN_BUSY) & 0x00004000) && (i++ < 10000))
			data = Register(VIP_TC_STATUS);
					 
		if (i >= 10000)
		{
			PRINT(("Microcode: channel 14 timeout\n"));
			return -1;
		}
		PRINT(("Microcode: dsp_load: checkpoint 2\n"));
		PRINT(("Microcode: TC_STATUS: %x\n", data));

		/* Take DSP out from reset (0x0) */
		data = Register(VIP_TC_DOWNLOAD);
		SetRegister(VIP_TC_DOWNLOAD, data & ~VIP_TC_DOWNLOAD__TC_RESET_MODE);

		data = Register(VIP_TC_STATUS);
		PRINT(("Microcode: dsp_load: checkpoint 3\n"));
		PRINT(("Microcode: TC_STATUS: %x\n", data));

		/* send dsp_download_check_CRC */
		fb_scratch0 = ((seg_list->num_bytes << 16) & 0xffff0000) | ((seg_id << 8) & 0xff00) | (0xff & 193);
		fb_scratch1 = (unsigned int)seg_list->crc_val;
					 
		result = DspSendCommand(fb_scratch1, fb_scratch0);

		PRINT(("Microcode: dsp_load: checkpoint 4\n"));
	}
	
	//debugger("DSPLoad");
	
	if (tries >= 10)
	{
		PRINT(("Microcode: Download of boot degment failed\n"));
		return -1;
	}

	PRINT(("Microcode: Download of boot code succeeded\n"));

	while((seg_list = seg_list->next) != NULL)
	{
		seg_id++;
		result = tries = 0;
		while(result != DSP_OK && tries++ < 10)
		{
			/* 
			 * Configure shuttle bus for tranfer between DSP I/O "Program Interface"
			 * and Data Memory at address 0 
			 */

			SetRegister(VIP_TC_SOURCE, 0x90000000);
			SetRegister(VIP_TC_DESTINATION, 0x10000000);
			SetRegister(VIP_TC_COMMAND, 0xe0000044 | ((seg_list->num_bytes - 1) << 7));

			if (!WriteFifo(0x700, seg_list->num_bytes, seg_list->data))
			{
				PRINT(("Microcode: write to FIFOD failed\n"));
				return -1;
			}
										
			i = data = 0;
			data = Register(VIP_TC_STATUS);
			while(((data & VIP_TC_STATUS__TC_CHAN_BUSY) & 0x00004000) && (i++ < 10000))
				data = Register(VIP_TC_STATUS);
										
			/* send dsp_download_check_CRC */
			fb_scratch0 = ((seg_list->num_bytes << 16) & 0xffff0000) | ((seg_id << 8) & 0xff00) | (0xff & 193);
			fb_scratch1 = (unsigned int)seg_list->crc_val;
										
			result = DspSendCommand(fb_scratch1, fb_scratch0);
		}

		if (i >=10)
		{
			PRINT(("Microcode: DSP failed to move seg: %x from data to code memory\n", seg_id));
			return -1;
		}

		PRINT(("Microcode: segment: %x loaded\n", seg_id));

		/*
		 * The segment is downloaded correctly to data memory. Now move it to code memory
		 * by using dsp_download_code_transfer command.
		 */

		fb_scratch0 = ((seg_list->num_bytes << 16) & 0xffff0000) | ((seg_id << 8) & 0xff00) | (0xff & 194);
		fb_scratch1 = (unsigned int)seg_list->download_dst;
								
		result = DspSendCommand(fb_scratch1, fb_scratch0);

		if (result != DSP_OK)
		{
			PRINT(("Microcode: DSP failed to move seg: %x from data to code memory\n", seg_id));
			return -1;
		}
	}

	PRINT(("Microcode: download complete\n"));

	/*
	 * The last step is sending dsp_download_check_CRC with "download complete"
	 */

	fb_scratch0 = ((165 << 8) & 0xff00) | (0xff & 193);
	fb_scratch1 = (unsigned int)0x11111;
								
	result = DspSendCommand(fb_scratch1, fb_scratch0);

	if (result == DSP_OK)
		PRINT(("Microcode: DSP microcode successfully loaded\n"));
	else
	{
		PRINT(("Microcode: DSP microcode UNsuccessfully loaded\n"));
		return -1;
	}

	return 0;
}

status_t CTheater200::DspSendCommand(uint32 fb_scratch1, uint32 fb_scratch0)
{
	uint32 data;
	int i;

	/*
	 * Clear the FB_INT0 bit in INT_CNTL
	 */
	data = Register(VIP_INT_CNTL);
	SetRegister(VIP_INT_CNTL, data | VIP_INT_CNTL__FB_INT0_CLR);

	/*
	 * Write FB_SCRATCHx registers. If FB_SCRATCH1==0 then we have a DWORD command.
	 */
	SetRegister(VIP_FB_SCRATCH0, fb_scratch0);
	if (fb_scratch1 != 0)
		SetRegister(VIP_FB_SCRATCH1, fb_scratch1);	

	/*
	 * Attention DSP. We are talking to you.
	 */
	data = Register(VIP_FB_INT);
	SetRegister(VIP_FB_INT, data | VIP_FB_INT__INT_7);

	/*
	 * Wait (by polling) for the DSP to process the command.
	 */
	i = 0;
	data = Register(VIP_INT_CNTL);
	while((!(data & VIP_INT_CNTL__FB_INT0)) && (i++ < 10))
	{
		snooze(1000);
		data = Register(VIP_INT_CNTL);
	}
	
	/*
	 * The return code is in FB_SCRATCH0
	 */
	fb_scratch0 = Register(VIP_FB_SCRATCH0);

	/*
	 * If we are here it means we got an answer. Clear the FB_INT0 bit.
	 */
	data = Register(VIP_INT_CNTL);
	SetRegister(VIP_INT_CNTL, data | VIP_INT_CNTL__FB_INT0_CLR);

	return fb_scratch0;
}

void CTheater200::InitTheatre()
{
	uint32 data;
	uint32 M, N, P;

	/* this will give 108Mhz at 27Mhz reference */
	M = 28;
	N = 224;
	P = 1;

	ShutdownTheatre();
	snooze(100000);
	fMode = MODE_INITIALIZATION_IN_PROGRESS;

	data = M | (N << 11) | (P <<24);
	SetRegister(VIP_DSP_PLL_CNTL, data);

	Register(VIP_PLL_CNTL0, data);
	data |= 0x2000;
	SetRegister(VIP_PLL_CNTL0, data);

	/* RT_regw(VIP_I2C_SLVCNTL, 0x249); */
	Register(VIP_PLL_CNTL1, data);
	data |= 0x00030003;
	SetRegister(VIP_PLL_CNTL1, data);

	Register(VIP_PLL_CNTL0, data);
	data &= 0xfffffffc;
	SetRegister(VIP_PLL_CNTL0, data);
	snooze(15000);

	Register(VIP_CLOCK_SEL_CNTL, data);
	data |= 0x1b;
	SetRegister(VIP_CLOCK_SEL_CNTL, data);

	Register(VIP_MASTER_CNTL, data);
	data &= 0xffffff07;
	SetRegister(VIP_MASTER_CNTL, data);
	data &= 0xffffff03;
	SetRegister(VIP_MASTER_CNTL, data);
	snooze(1000);
	 
	if (microcode_path == NULL)
	{
		microcode_path = const_cast<char *>(DEFAULT_MICROC_PATH);
		PRINT(("Microcode: Use default microcode path: %s\n", DEFAULT_MICROC_PATH));
	}
	else
	{
		PRINT(("Microcode: Use microcode path: %s\n", microcode_path));
	}

	if (microcode_type == NULL)
	{
		microcode_type = const_cast<char *>(DEFAULT_MICROC_TYPE);
		PRINT(("Microcode: Use default microcode type: %s\n", DEFAULT_MICROC_TYPE));
	}
	else
	{
		PRINT(("Microcode: Use microcode type: %s\n", microcode_type));
	}
	
	if (DSPDownloadMicrocode() < 0)
	{
		ShutdownTheatre();
		return;
	}

	//DspSetLowPowerState(1);
	//DspSetVideoStreamFormat(1);

	fMode = MODE_INITIALIZED_FOR_TV_IN;
}

int CTheater200::DSPDownloadMicrocode()
{
	struct rt200_microc_data microc_data;
	microc_data.microc_seg_list = NULL;

	if (DSPLoadMicrocode(microcode_path, microcode_type, &microc_data) < 0)
	{
		PRINT(("Microcode: cannot load microcode\n"));
		goto err_exit;
	}
	else
	{
		PRINT(("Microcode: device_id: %x\n", microc_data.microc_head.device_id));
		PRINT(("Microcode: vendor_id: %x\n", microc_data.microc_head.vendor_id));
		PRINT(("Microcode: rev_id: %x\n", 	 microc_data.microc_head.revision_id));
		PRINT(("Microcode: num_seg: %x\n", 	 microc_data.microc_head.num_seg));
	}

	if (DspInit() < 0)
	{
		PRINT(("Microcode: dsp_init failed\n"));
		goto err_exit;
	}
	else
	{
		PRINT(("Microcode: dsp_init OK\n"));
	}

	if (DspLoad(&microc_data) < 0)
	{
		PRINT(("Microcode: dsp_download failed\n"));
		goto err_exit;
	}
	else
	{
		PRINT(("Microcode: dsp_download OK\n"));
	}

	DSPCleanMicrocode(&microc_data);
	return 0;

err_exit:

	DSPCleanMicrocode(&microc_data);
	return -1;
				
}

void CTheater200::ShutdownTheatre()
{
    fMode = MODE_UNINITIALIZED;
}

void CTheater200::ResetTheatreRegsForNoTVout()
{
	SetRegister(VIP_CLKOUT_CNTL, 0x0); 
	SetRegister(VIP_HCOUNT, 0x0); 
	SetRegister(VIP_VCOUNT, 0x0); 
	SetRegister(VIP_DFCOUNT, 0x0); 
#if 0
	SetRegister(VIP_CLOCK_SEL_CNTL, 0x2b7);  /* versus 0x237 <-> 0x2b7 */
	SetRegister(VIP_VIN_PLL_CNTL, 0x60a6039);
#endif
	SetRegister(VIP_FRAME_LOCK_CNTL, 0x0);
}

void CTheater200::ResetTheatreRegsForTVout()
{
	SetRegister(VIP_CLKOUT_CNTL, 0x29); 
#if 1
	SetRegister(VIP_HCOUNT, 0x1d1); 
	SetRegister(VIP_VCOUNT, 0x1e3); 
#else
	SetRegister(VIP_HCOUNT, 0x322); 
	SetRegister(VIP_VCOUNT, 0x151);
#endif
	SetRegister(VIP_DFCOUNT, 0x01); 
	SetRegister(VIP_CLOCK_SEL_CNTL, 0x2b7);		/* versus 0x237 <-> 0x2b7 */
	SetRegister(VIP_VIN_PLL_CNTL, 0x60a6039);
	SetRegister(VIP_FRAME_LOCK_CNTL, 0x0f);
}

int32 CTheater200::DspSetVideostreamformat(int32 format)
{
	int32 fb_scratch0 = 0;
	int32 result;
	
	fb_scratch0 = ((format << 8) & 0xff00) | (65 & 0xff);
	result = DspSendCommand(0, fb_scratch0);

	PRINT(("dsp_set_videostreamformat: %x\n", result));
		  
	return result;
}

int32 CTheater200::DspGetSignalLockStatus()
{
	int32 fb_scratch1 = 0;
	int32 fb_scratch0 = 0;
	int32 result;
	
	fb_scratch0 = 0 | (77 & 0xff);

	result = DspSendCommand(fb_scratch1, fb_scratch0);

	PRINT(("dsp_get_signallockstatus: %x, h_pll: %x, v_pll: %x\n", \
		result, (result >> 8) & 0xff, (result >> 16) & 0xff));
		  
	return result;
}

// disable/enable capturing
void CTheater200::SetEnable(bool enable, bool vbi)
{

	PRINT(("CTheater200::SetEnable(%d, %d)\n", enable, vbi));

	if (enable) {
		WaitVSYNC();

		SetADC(fStandard, fSource);

		SetScaler(fStandard, fHActive, fVActive, fDeinterlace);
		
		// Enable ADC block
		SetRegister(VIP_ADC_CNTL, ADC_PDWN, ADC_PDWN_UP);

		WaitVSYNC();

		// restore luminance and chroma settings
		SetLuminanceLevels(fStandard, fBrightness, fContrast);
		SetChromaLevels(fStandard, fSaturation, fHue);
	}
}

void CTheater200::SetStandard(theater_standard standard, theater_source source)
{
	PRINT(("CTheater200::SetStandard(%s, %s)\n",
		"NTSC\0\0\0\0\0\0NTSC-J\0\0\0\0NTSC-443\0\0PAL-M\0\0\0\0\0"
		"PAL-N\0\0\0\0\0PAL-NC\0\0\0\0PAL-BDGHI\0PAL-60\0\0\0\0"
		"SECAM\0\0\0\0\0"+10*standard,
		"TUNER\0COMP\0\0SVIDEO"+6*source));
		
	fStandard = standard;
	fSource = source;
}

void CTheater200::SetSize(int hactive, int vactive)
{
	PRINT(("CTheater200::SetSize(%d, %d)\n", hactive, vactive));
	
	fHActive = hactive;
	fVActive = vactive;
}

void CTheater200::SetDeinterlace(bool deinterlace)
{
	PRINT(("CTheater200::SetDeinterlace(%d)\n", deinterlace));
	
	fDeinterlace = deinterlace;
}

/* one assumes as sharpness is not used it's not supported */
void CTheater200::SetSharpness(int sharpness)
{
	int32 fb_scratch0 = 0;
	int32 fb_scratch1 = 1;
	int32 result;
	
	PRINT(("CTheater200::SetSharpness(%d)\n", sharpness));
	
	fb_scratch0 = 0 | (73 & 0xff);
	result = DspSendCommand(fb_scratch1, fb_scratch0);
}

void CTheater200::SetBrightness(int brightness)
{
	PRINT(("CTheater200::SetBrightness(%d)\n", brightness));
	
	fBrightness = brightness;
	SetLuminanceLevels(fStandard, fBrightness, fContrast);
}

void CTheater200::SetContrast(int contrast)
{
	PRINT(("CTheater200::SetContrast(%d)\n", contrast));

	fContrast = contrast;
	SetLuminanceLevels(fStandard, fBrightness, fContrast);
}

void CTheater200::SetSaturation(int saturation)
{
	PRINT(("CTheater200::SetSaturation(%d)\n", saturation));

	fSaturation = saturation;
	SetChromaLevels(fStandard, fSaturation, fHue);
}

void CTheater200::SetHue(int hue)
{
	PRINT(("CTheater200::SetHue(%d)\n", hue));

	fHue = hue;
	SetChromaLevels(fStandard, fSaturation, fHue);
}

// setup analog-digital converter
void CTheater200::SetADC(theater_standard standard, theater_source source)
{
	uint32 fb_scratch0 = 0;
	uint32 result;
	uint32 data = 0;
	
	PRINT(("CTheater200::SetADC(%c, %c)\n", "NJ4MNCB6S"[standard], "TCS"[source]));
	
	// set HW_DEBUG before setting the standard
	SetRegister(VIP_HW_DEBUG, 0x0000f000);
	
	// select the video standard
	switch (standard) {
	case C_THEATER_NTSC:
	case C_THEATER_NTSC_JAPAN:
	case C_THEATER_NTSC_443:
	case C_THEATER_PAL_M:
		// SetRegister(VIP_STANDARD_SELECT, STANDARD_SEL, STANDARD_NTSC);
		// break;
	case C_THEATER_PAL_BDGHI:
	case C_THEATER_PAL_N:
	case C_THEATER_PAL_60:
	case C_THEATER_PAL_NC:
		// SetRegister(VIP_STANDARD_SELECT, STANDARD_SEL, STANDARD_PAL);
		// break;
	case C_THEATER_SECAM:
		// SetRegister(VIP_STANDARD_SELECT, STANDARD_SEL, STANDARD_SECAM);
		fb_scratch0 = ((standard << 8) & 0xff00) | (52 & 0xff);
		result = DspSendCommand(0, fb_scratch0);
		break;
	default:
		PRINT(("CTheater200::SetADC() - Bad standard\n"));
		return;
	}

	Register(VIP_GPIO_CNTL, data);
	PRINT(("VIP_GPIO_CNTL: %x\n", data));

	Register(VIP_GPIO_INOUT, data);
	PRINT(("VIP_GPIO_INOUT: %x\n", data));
	
	// select input connector and Y/C mode
	switch (source) {
	case C_THEATER_TUNER:
		// set video input connector
		fb_scratch0 = ((fTunerPort << 8) & 0xff00) | (55 & 0xff);
		DspSendCommand(0, fb_scratch0);
		
		/* this is to set the analog mux used for sond */
		Register(VIP_GPIO_CNTL, data);
		data &= ~0x10;
		SetRegister(VIP_GPIO_CNTL, data);
 
		Register(VIP_GPIO_INOUT, data);
		data &= ~0x10;
		SetRegister(VIP_GPIO_INOUT, data);
		break;
	case C_THEATER_COMPOSITE:
		// set video input connector
		fb_scratch0 = ((fCompositePort << 8) & 0xff00) | (55 & 0xff);
		DspSendCommand(0, fb_scratch0);
		
		/* this is to set the analog mux used for sond */
		Register(VIP_GPIO_CNTL, data);
		data |= 0x10;
		SetRegister(VIP_GPIO_CNTL, data);
 
		Register(VIP_GPIO_INOUT, data);
		data |= 0x10;
		SetRegister(VIP_GPIO_INOUT, data);
		break;
	case C_THEATER_SVIDEO:
		// set video input connector
		fb_scratch0 = ((fSVideoPort << 8) & 0xff00) | (55 & 0xff);
		DspSendCommand(0, fb_scratch0);
		
		/* this is to set the analog mux used for sond */
		Register(VIP_GPIO_CNTL, data);
		data |= 0x10;
		SetRegister(VIP_GPIO_CNTL, data);
 
		Register(VIP_GPIO_INOUT, data);
		data |= 0x10;
		SetRegister(VIP_GPIO_INOUT, data);
		break;
	default:
		PRINT(("CTheater200::SetADC() - Bad source\n"));
		return;
	}
	
	
	Register(VIP_GPIO_CNTL, data);
	PRINT(("VIP_GPIO_CNTL: %x\n", data));

	Register(VIP_GPIO_INOUT, data);
	PRINT(("VIP_GPIO_INOUT: %x\n", data));


	DspConfigureI2SPort(0, 0, 0);
	DspConfigureSpdifPort(0);

	/*dsp_audio_detection(t, 0);*/
	DspAudioMute(1, 1);
	DspSetAudioVolume(128, 128, 0);
	
}

// wait until horizontal scaler is locked
void CTheater200::WaitHSYNC()
{
	for (int timeout = 0; timeout < 1000; timeout++) {
		if (Register(VIP_HS_PULSE_WIDTH, HS_GENLOCKED) != 0)
			return;
		snooze(20);
	}
	PRINT(("CTheater200::WaitHSYNC() - wait for HSync locking time out!\n"));
}



// wait until a visible line is viewed
void CTheater200::WaitVSYNC()
{
	for (int timeout = 0; timeout < 1000; timeout++) {
		int lineCount = CurrentLine();
		if (lineCount > 1 && lineCount < 20)
			return;
		snooze(20);
	}
	PRINT(("CTheater200::WaitVSYNC() - wait for VBI timed out!\n"));
}

// setup brightness and contrast
void CTheater200::SetLuminanceLevels(theater_standard standard, int brightness, int contrast)
{

	int32 fb_scratch1 = 0;
	int32 fb_scratch0 = 0;
	int32 result;

	/* set luminance processor constrast */
	fb_scratch0 = ((contrast << 8) & 0xff00) | (71 & 0xff);
	result = DspSendCommand(fb_scratch1, fb_scratch0);
	PRINT(("dsp_set_contrast: %x\n", result));

	/* set luminance processor brightness */
	fb_scratch0 = ((brightness << 8) & 0xff00) | (67 & 0xff);
	DspSendCommand(fb_scratch1, fb_scratch0);
	PRINT(("dsp_set_brightness: %x\n", result));
	
}

// set colour saturation and hue.
// hue makes sense for NTSC only and seems to act as saturation for PAL
void CTheater200::SetChromaLevels(theater_standard standard, int saturation, int hue)
{

	int32 fb_scratch1 = 0;
	int32 fb_scratch0 = 0;
	
	// Set Hue
	fb_scratch0 = ((hue << 8) & 0xff00) | (75 & 0xff);
	DspSendCommand(fb_scratch1, fb_scratch0);
	
	// Set Saturation
	fb_scratch0 = ((saturation << 8) & 0xff00) | (69 & 0xff);
	DspSendCommand(fb_scratch1, fb_scratch0);
	
	PRINT(("dsp_set_saturation: %x\n", saturation));
	PRINT(("dsp_set_tint: %x\n", hue));
}


// these values are used by scaler as well
static const uint16 h_active_start[] = {
	0x06b,	0x06B,	0x07E,	0x067,	0x09A,	0x07D,	0x09A,	0x084,	0x095 };
static const uint16 h_active_end[] = {
	0x363,	0x363,	0x42A,	0x363,	0x439,	0x439,	0x439,	0x363,	0x439 };
static const uint16 v_active_start[] = {
	0x025,	0x025,	0x025,	0x025,	0x02E,	0x02E,	0x02E,	0x025,	0x02E };
// PAL height is too small (572 instead of 576 lines), but changing 0x269 to 0x26d
// leads to trouble, and the last 2 lines seem to be used for VBI data 
// (read: garbage) anyway
static const uint16 v_active_end[] = {
	0x204,	0x204,	0x204,	0x204,	0x269,	0x269,	0x269,	0x204,	0x269 };
static const uint16 h_vbi_wind_start[] = {
	0x064,	0x064,	0x064,	0x064,	0x084,	0x084,	0x084,	0x064,	0x084 };
static const uint16 h_vbi_wind_end[] = {
	0x366,	0x366,	0x366,	0x366,	0x41F,	0x41F,	0x41F,	0x366,	0x41F };
static const uint16 v_vbi_wind_start[] = {
	0x00b,	0x00b,	0x00b,	0x00b,	0x008,	0x008,	0x008,	0x00b,	0x008 };
static const uint16 v_vbi_wind_end[] = {
	0x024,	0x024,	0x024,	0x024,	0x02d,	0x02d,	0x02d,	0x024,	0x02d };

	
void CTheater200::getActiveRange( theater_standard standard, CRadeonRect &rect )
{

	rect.SetTo( 
		h_active_start[standard], v_active_start[standard],
		h_active_end[standard], v_active_end[standard] );

}

void CTheater200::getVBIRange( theater_standard standard, CRadeonRect &rect )
{

	rect.SetTo( 
		h_vbi_wind_start[standard], v_vbi_wind_start[standard],
		h_vbi_wind_end[standard], v_vbi_wind_end[standard] );

}

// setup capture scaler.
void CTheater200::SetScaler(theater_standard standard, int hactive, int vactive, bool deinterlace)
{

	int32 fb_scratch1 = 0;
	int32 fb_scratch0 = 0;
	int oddOffset, evenOffset;
	uint16 h_active_width, v_active_height;
	
//	ASSERT(vactive <= 511);

	// TK: Gatos uses different values here
	h_active_width = h_active_end[standard] - h_active_start[standard] + 1;
	v_active_height = v_active_end[standard] - v_active_start[standard] + 1;
	
	// for PAL, we have 572 lines only, but need 576 lines;
	// my attempts to find those missing lines all failed, so if the application requests
	// 576 lines, we had to upscale the video which is not supported by hardware;
	// solution: restrict to 572 lines - the scaler will fill out the missing lines with black
	if( vactive > v_active_height )
		vactive = v_active_height;
	
	if (deinterlace) {
		// progressive scan
		evenOffset = oddOffset = 512 - (int) ((512 * vactive) / v_active_height);
	}
	else {
		// interlaced
		evenOffset = (int) ((512 * vactive) / v_active_height);
		oddOffset = 2048 - evenOffset;
	}

	// Set Horizontal Size
	fb_scratch0 = ((h_active_width << 8) & 0x00ffff00) | (195 & 0xff);
	fb_scratch1 = ((h_active_end[standard] << 16) & 0xffff0000) | (h_active_start[standard] & 0xffff);
	DspSendCommand(fb_scratch1, fb_scratch0);

	// Set Vertical Size
	fb_scratch0 = ((v_active_height << 8) & 0x00ffff00) | (196 & 0xff);
	fb_scratch1 = ((v_active_end[standard] << 16) & 0xffff0000) | (v_active_start[standard] + 1 & 0xffff);
	DspSendCommand(fb_scratch1, fb_scratch0);
}

int32 CTheater200::DspAudioMute(int8 left, int8 right)
{
	int32 fb_scratch1 = 0;
	int32 fb_scratch0 = 0;
	int32 result;

	fb_scratch0 = ((right << 16) & 0xff0000) | ((left << 8) & 0xff00) | (21 & 0xff);
	result = DspSendCommand(fb_scratch1, fb_scratch0);

	PRINT(("dsp_audio_mute: %x\n", result));
		  
	return result;
}

int32 CTheater200::DspSetAudioVolume(int8 left, int8 right, int8 auto_mute)
{
	int32 fb_scratch1 = 0;
	int32 fb_scratch0 = 0;
	int32 result;
	
	fb_scratch0 = ((auto_mute << 24) & 0xff000000) 
		| ((right << 16) & 0xff0000) 
		| ((left << 8) & 0xff00) | (22 & 0xff);
	result = DspSendCommand(fb_scratch1, fb_scratch0);

	PRINT(("dsp_set_audio_volume: %x\n", result));
		  
	return result;
}

int32 CTheater200::DspConfigureI2SPort(int8 tx_mode, int8 rx_mode, int8 clk_mode)
{
	int32 fb_scratch1 = 0;
	int32 fb_scratch0 = 0;
	int32 result;

	fb_scratch0 = ((clk_mode << 24) & 0xff000000) | ((rx_mode << 16) & 0xff0000) 
					| ((tx_mode << 8) & 0xff00) | (40 & 0xff);

	result = DspSendCommand(fb_scratch1, fb_scratch0);

	PRINT(("dsp_configure_i2s_port: %x\n", result));
		  
	return result;
}

int32 CTheater200::DspConfigureSpdifPort(int8 state)
{
	int32 fb_scratch1 = 0;
	int32 fb_scratch0 = 0;
	int32 result;

	fb_scratch0 = ((state << 8) & 0xff00) | (41 & 0xff);

	result = DspSendCommand(fb_scratch1, fb_scratch0);

	PRINT(("dsp_configure_spdif_port: %x\n", result));
		  
	return result;
}

int CTheater200::ReadFifo( uint32 address, uint8 *buffer)
{
	return fPort.ReadFifo(fDevice, address, 1, buffer);
}
	
int CTheater200::WriteFifo( uint32 address, uint32 count, uint8 *buffer)
{
	return fPort.WriteFifo(fDevice, address, count, buffer);
}

int CTheater200::CurrentLine()
{
//	return Register(VIP_VS_LINE_COUNT) & VS_LINE_COUNT;
	int32 fb_scratch1 = 0;
	int32 fb_scratch0 = 0;
	int32 result;

	fb_scratch0 = 0 | (78 & 0xff);
	result = DspSendCommand(fb_scratch1, fb_scratch0);

	PRINT(("dsp_get_signallinenumber: %x, linenum: %x\n", \
		result, (result >> 8) & 0xffff));
		  
	return result;
	
}

void CTheater200::PrintToStream()
{
	PRINT(("<<< Rage Theater Registers >>>\n"));
	/*for (int index = 0x0400; index <= 0x06ff; index += 4) {
		int value = Register(index);
		PRINT(("REG_0x%04x = 0x%08x\n", index, value));
	}	*/
}
