/******************************************************************************
/
/	File:			Radeon.h
/
/	Description:	ATI Radeon Graphics Chip interface.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#ifndef __RADEON_H__
#define __RADEON_H__

#include "radeon_interface.h"

#define BITS(a) int((0xffffffffU>>(31-(1?a)))&(0xffffffffU<<(0?a)))

enum radeon_video_tuner {
	C_RADEON_NO_TUNER							= 0,
	C_RADEON_FI1236_MK1_NTSC					= 1,
	C_RADEON_FI1236_MK2_NTSC_JAPAN				= 2,
	C_RADEON_FI1216_MK2_PAL_BG					= 3,
	C_RADEON_FI1246_MK2_PAL_I					= 4,
	C_RADEON_FI1216_MF_MK2_PAL_BG_SECAM_L		= 5,
	C_RADEON_FI1236_MK2_NTSC					= 6,
	C_RADEON_FI1256_MK2_SECAM_DK				= 7,
	C_RADEON_FI1216_MK2_PAL_BG_SECAM_L			= 8,
	C_RADEON_TEMIC_FN5AL_PAL_IBGDK_SECAM_DK		= 9
};

enum radeon_video_clock {
	C_RADEON_NO_VIDEO_CLOCK						= 0,
	C_RADEON_VIDEO_CLOCK_28_63636_MHZ			= 1,
	C_RADEON_VIDEO_CLOCK_29_49892_MHZ			= 2,
	C_RADEON_VIDEO_CLOCK_27_00000_MHZ			= 3,
	C_RADEON_VIDEO_CLOCK_14_31818_MHZ			= 4
};

enum radeon_video_decoder {
	C_RADEON_NO_VIDEO							= 0,
	C_RADEON_BT819								= 1,
	C_RADEON_BT829								= 2,
	C_RADEON_BT829A								= 3,
	C_RADEON_SA7111								= 4,
	C_RADEON_SA7112								= 5,
	C_RADEON_RAGE_THEATER						= 6
};

enum radeon_register {
	C_RADEON_VIDEOMUX_CNTL						= 0x0190,
		C_RADEON_VIPH_INT_SEL					= BITS(0:0),
		C_RADEON_ROM_CLK_DIVIDE					= BITS(20:16),
		C_RADEON_STR_ROMCLK						= BITS(21:21),
		C_RADEON_VIP_INTERNAL_DEBUG_SEL			= BITS(24:22),

	// I2C
    C_RADEON_I2C_CNTL_0                        	= 0x0090,
    C_RADEON_I2C_CNTL_0_PLUS1					= 0x0091,
        C_RADEON_I2C_DONE                      	= BITS(0:0),
        C_RADEON_I2C_NACK                      	= BITS(1:1),
        C_RADEON_I2C_HALT                      	= BITS(2:2),
        C_RADEON_I2C_SOFT_RST                  	= BITS(5:5),
        C_RADEON_I2C_DRIVE_EN                  	= BITS(6:6),
        C_RADEON_I2C_DRIVE_SEL                 	= BITS(7:7),
            C_RADEON_I2C_DRIVE_SEL_10_MCLKS    	= 0 << 7,
            C_RADEON_I2C_DRIVE_SEL_20_MCLKS    	= 1 << 7,
        C_RADEON_I2C_START                     	= BITS(8:8),
        C_RADEON_I2C_STOP                      	= BITS(9:9),
        C_RADEON_I2C_RECEIVE                   	= BITS(10:10),
        C_RADEON_I2C_ABORT                     	= BITS(11:11),
        C_RADEON_I2C_GO                        	= BITS(12:12),
		C_RADEON_I2C_PRESCALE					= BITS(31:16),
		
    C_RADEON_I2C_CNTL_1                        	= 0x0094,
        C_RADEON_I2C_DATA_COUNT                	= BITS(3:0),
        C_RADEON_I2C_ADDR_COUNT                	= BITS(10:8),
        C_RADEON_I2C_SEL                       	= BITS(16:16),
        C_RADEON_I2C_EN                        	= BITS(17:17),
		C_RADEON_I2C_TIME_LIMIT					= BITS(31:24),

    C_RADEON_I2C_DATA                          	= 0x0098,
        C_RADEON_I2C_DATA_MASK                 	= BITS(7:0),

	// Capture
	C_RADEON_CAP_INT_CNTL						= 0x0908,
		C_RADEON_CAP0_BUF0_INT_EN				= BITS(0:0),
		C_RADEON_CAP0_BUF0_EVEN_INT_EN			= BITS(1:1),
		C_RADEON_CAP0_BUF1_INT_EN				= BITS(2:2),
		C_RADEON_CAP0_BUF1_EVEN_INT_EN			= BITS(3:3),
		C_RADEON_CAP0_VBI0_INT_EN				= BITS(4:4),
		C_RADEON_CAP0_VBI1_INT_EN				= BITS(5:5),
		C_RADEON_CAP0_ONESHOT_INT_EN			= BITS(6:6),
		C_RADEON_CAP0_ANC0_INT_EN				= BITS(7:7),
		C_RADEON_CAP0_ANC1_INT_EN				= BITS(8:8),
		C_RADEON_CAP0_VBI2_INT_EN				= BITS(9:9),
		C_RADEON_CAP0_VBI3_INT_EN				= BITS(10:10),
		C_RADEON_CAP0_ANC2_INT_EN				= BITS(11:11),
		C_RADEON_CAP0_ANC3_INT_EN				= BITS(12:12),
	
	C_RADEON_CAP_INT_STATUS						= 0x090C,
		C_RADEON_CAP0_BUF0_INT					= BITS(0:0),
		C_RADEON_CAP0_BUF0_INT_AK				= BITS(0:0),
		C_RADEON_CAP0_BUF0_EVEN_INT				= BITS(1:1),
		C_RADEON_CAP0_BUF0_EVEN_INT_AK			= BITS(1:1),
		C_RADEON_CAP0_BUF1_INT					= BITS(2:2),
		C_RADEON_CAP0_BUF1_INT_AK				= BITS(2:2),
		C_RADEON_CAP0_BUF1_EVEN_INT				= BITS(3:3),
		C_RADEON_CAP0_BUF1_EVEN_INT_AK			= BITS(3:3),
		C_RADEON_CAP0_VBI0_INT					= BITS(4:4),
		C_RADEON_CAP0_VBI0_INT_AK				= BITS(4:4),
		C_RADEON_CAP0_VBI1_INT					= BITS(5:5),
		C_RADEON_CAP0_VBI1_INT_AK				= BITS(5:5),
		C_RADEON_CAP0_ONESHOT_INT				= BITS(6:6),
		C_RADEON_CAP0_ONESHOT_INT_AK			= BITS(6:6),
		C_RADEON_CAP0_ANC0_INT					= BITS(7:7),
		C_RADEON_CAP0_ANC0_INT_AK				= BITS(7:7),
		C_RADEON_CAP0_ANC1_INT					= BITS(8:8),
		C_RADEON_CAP0_ANC1_INT_AK				= BITS(8:8),
		C_RADEON_CAP0_VBI2_INT					= BITS(9:9),
		C_RADEON_CAP0_VBI2_INT_AK				= BITS(9:9),
		C_RADEON_CAP0_VBI3_INT					= BITS(10:10),
		C_RADEON_CAP0_VBI3_INT_AK				= BITS(10:10),
		C_RADEON_CAP0_ANC2_INT					= BITS(11:11),
		C_RADEON_CAP0_ANC2_INT_AK				= BITS(11:11),
		C_RADEON_CAP0_ANC3_INT					= BITS(12:12),
		C_RADEON_CAP0_ANC3_INT_AK				= BITS(12:12),
	
	C_RADEON_FCP_CNTL							= 0x0910,
		C_RADEON_FCP0_SRC_SEL					= BITS(2:0),
			C_RADEON_FCP0_SRC_PCICLK			= 0 << 0,
			C_RADEON_FCP0_SRC_PCLK				= 1 << 0,
			C_RADEON_FCP0_SRC_PCLKb				= 2 << 0,
			C_RADEON_FCP0_SRC_HREF				= 3 << 0,
			C_RADEON_FCP0_SRC_GND				= 4 << 0,
			C_RADEON_FCP0_SRC_HREFb				= 5 << 0,

	C_RADEON_CAP0_BUF0_OFFSET                   = 0x0920,
	C_RADEON_CAP0_BUF1_OFFSET                   = 0x0924,
	C_RADEON_CAP0_BUF0_EVEN_OFFSET              = 0x0928,
	C_RADEON_CAP0_BUF1_EVEN_OFFSET              = 0x092C,
	
	C_RADEON_CAP0_BUF_PITCH                     = 0x0930,
		C_RADEON_CAP0_BUF_PITCH_MASK			= BITS(11:0),
		
	C_RADEON_CAP0_V_WINDOW                      = 0x0934,
		C_RADEON_CAP0_V_START					= BITS(11:0),
		C_RADEON_CAP0_V_END						= BITS(27:16),

	C_RADEON_CAP0_H_WINDOW                      = 0x0938,
		C_RADEON_CAP0_H_START					= BITS(11:0),
		C_RADEON_CAP0_H_WIDTH					= BITS(27:16),
		
	C_RADEON_CAP0_VBI0_OFFSET                   = 0x093C, 
	C_RADEON_CAP0_VBI1_OFFSET                   = 0x0940,
	
	C_RADEON_CAP0_VBI_V_WINDOW                  = 0x0944,
		C_RADEON_CAP0_VBI_V_START				= BITS(11:0),
		C_RADEON_CAP0_VBI_V_END					= BITS(27:16),
		
	C_RADEON_CAP0_VBI_H_WINDOW                  = 0x0948,
		C_RADEON_CAP0_VBI_H_START				= BITS(11:0),
		C_RADEON_CAP0_VBI_H_WIDTH				= BITS(27:16),
	
	C_RADEON_CAP0_PORT_MODE_CNTL                = 0x094C,
		C_RADEON_CAP0_PORT_WIDTH				= BITS(1:1),
			C_RADEON_CAP0_PORT_WIDTH_8_BITS		= 0 << 1,
			C_RADEON_CAP0_PORT_WIDTH_16_BITS	= 1 << 1,
		C_RADEON_CAP0_PORT_BYTE_USED			= BITS(2:2),
			C_RADEON_CAP0_PORT_LOWER_BYTE_USED	= 0 << 2,
			C_RADEON_CAP0_PORT_UPPER_BYTE_USED	= 1 << 2,
		
	C_RADEON_CAP0_TRIG_CNTL                     = 0x0950,
		C_RADEON_CAP0_TRIGGER_R					= BITS(1:0),
			C_RADEON_CAP0_TRIGGER_R_COMPLETE	= 0 << 0,
			C_RADEON_CAP0_TRIGGER_R_PENDING		= 1 << 0,
			C_RADEON_CAP0_TRIGGER_R_IN_PROGRESS	= 2 << 0,
		C_RADEON_CAP0_TRIGGER_W					= BITS(0:0),
			C_RADEON_CAP0_TRIGGER_W_NO_ACTION	= 0 << 0,
			C_RADEON_CAP0_TRIGGER_W_CAPTURE		= 1 << 0,
		C_RADEON_CAP0_EN						= BITS(4:4),
		C_RADEON_CAP0_VSYNC_CNT_R				= BITS(15:8),
		C_RADEON_CAP0_VSYNC_CLR					= BITS(16:16),
		
	C_RADEON_CAP0_DEBUG                         = 0x0954,
		C_RADEON_CAP0_H_STATUS					= BITS(11:0),
		C_RADEON_CAP0_V_STATUS					= BITS(27:16),
		C_RADEON_CAP0_V_SYNC					= BITS(28:28),
		
	C_RADEON_CAP0_CONFIG                        = 0x0958,
		C_RADEON_CAP0_INPUT_MODE				= BITS(0:0),
			C_RADEON_CAP0_INPUT_MODE_ONESHOT	= 0 << 0,
			C_RADEON_CAP0_INPUT_MODE_CONTINUOUS	= 1 << 0,
		C_RADEON_CAP0_START_FIELD				= BITS(1:1),
			C_RADEON_CAP0_START_ODD_FIELD		= 0 << 1,
			C_RADEON_CAP0_START_EVEN_FIELD		= 1 << 1,
		C_RADEON_CAP0_START_BUF_R				= BITS(2:2),
		C_RADEON_CAP0_START_BUF_W				= BITS(3:3),
		C_RADEON_CAP0_BUF_TYPE					= BITS(5:4),
			C_RADEON_CAP0_BUF_TYPE_FIELD		= 0 << 4,
			C_RADEON_CAP0_BUF_TYPE_ALTERNATING	= 1 << 4,
			C_RADEON_CAP0_BUF_TYPE_FRAME		= 2 << 4,
		C_RADEON_CAP0_ONESHOT_MODE				= BITS(6:6),
			C_RADEON_CAP0_ONESHOT_MODE_FIELD	= 0 << 6,
			C_RADEON_CAP0_ONESHOT_MODE_FRAME	= 1 << 6,
		C_RADEON_CAP0_BUF_MODE					= BITS(8:7),
			C_RADEON_CAP0_BUF_MODE_SINGLE		= 0 << 7,
			C_RADEON_CAP0_BUF_MODE_DOUBLE		= 1 << 7,
			C_RADEON_CAP0_BUF_MODE_TRIPLE		= 2 << 7,
		C_RADEON_CAP0_MIRROR_EN					= BITS(9:9),
		C_RADEON_CAP0_ONESHOT_MIRROR_EN			= BITS(10:10),
		C_RADEON_CAP0_VIDEO_SIGNED_UV			= BITS(11:11),
		C_RADEON_CAP0_ANC_DECODE_EN				= BITS(12:12),
		C_RADEON_CAP0_VBI_EN					= BITS(13:13),
		C_RADEON_CAP0_SOFT_PULL_DOWN_EN			= BITS(14:14),
		C_RADEON_CAP0_VIP_EXTEND_FLAG_EN		= BITS(15:15),
		C_RADEON_CAP0_FAKE_FIELD_EN				= BITS(16:16),
		C_RADEON_CAP0_FIELD_START_LINE_DIFF		= BITS(18:17),
		C_RADEON_CAP0_HORZ_DOWN					= BITS(20:19),
			C_RADEON_CAP0_HORZ_DOWN_1X			= 0 << 19,
			C_RADEON_CAP0_HORZ_DOWN_2X			= 1 << 19,
			C_RADEON_CAP0_HORZ_DOWN_3X			= 2 << 19,
		C_RADEON_CAP0_VERT_DOWN					= BITS(22:21),
			C_RADEON_CAP0_VERT_DOWN_1X			= 0 << 21,
			C_RADEON_CAP0_VERT_DOWN_2X			= 1 << 21,
			C_RADEON_CAP0_VERT_DOWN_3X			= 2 << 21,
		C_RADEON_CAP0_STREAM_FORMAT				= BITS(25:23),
			C_RADEON_CAP0_STREAM_BROOKTREE		= 0 << 23,
			C_RADEON_CAP0_STREAM_CCIR656		= 1 << 23,
			C_RADEON_CAP0_STREAM_ZV				= 2 << 23,
			C_RADEON_CAP0_STREAM_VIP			= 3 << 23,
			C_RADEON_CAP0_STREAM_TRANSPORT		= 4 << 23,
		C_RADEON_CAP0_HDWNS_DEC					= BITS(26:26),
			C_RADEON_CAP0_DOWNSCALER			= 0 << 26,
			C_RADEON_CAP0_DECIMATOR				= 1 << 26,
		C_RADEON_CAP0_VIDEO_IN_FORMAT			= BITS(29:29),
			C_RADEON_CAP0_VIDEO_IN_YVYU422		= 0 << 29,
			C_RADEON_CAP0_VIDEO_IN_VYUY422		= 1 << 29,
		C_RADEON_CAP0_VBI_HORZ_DOWN				= BITS(31:30),
			C_RADEON_CAP0_VBI_HORZ_DOWN_1X		= 0 << 30,
			C_RADEON_CAP0_VBI_HORZ_DOWN_2X		= 1 << 30,
			C_RADEON_CAP0_VBI_HORZ_DOWN_4X		= 2 << 30,
		
	C_RADEON_CAP0_ANC0_OFFSET                   = 0x095C, 
	C_RADEON_CAP0_ANC1_OFFSET                   = 0x0960,
	
	C_RADEON_CAP0_ANC_H_WINDOW                  = 0x0964,
		C_RADEON_CAP0_ANC_WIDTH					= BITS(11:0),
	
	C_RADEON_CAP0_VIDEO_SYNC_TEST               = 0x0968,
		C_RADEON_CAP0_TEST_VID_SOF				= BITS(0:0),
		C_RADEON_CAP0_TEST_VID_EOF				= BITS(1:1),
		C_RADEON_CAP0_TEST_VID_EOL				= BITS(2:2),
		C_RADEON_CAP0_TEST_VID_FIELD			= BITS(3:3),
		C_RADEON_CAP0_TEST_SYNC_EN				= BITS(5:5),
	
	C_RADEON_CAP0_ONESHOT_BUF_OFFSET            = 0x096C,
	
	C_RADEON_CAP0_BUF_STATUS                    = 0x0970,
		C_RADEON_CAP0_PRE_VID_BUF				= BITS(1:0),
		C_RADEON_CAP0_CUR_VID_BUF				= BITS(3:2),
		C_RADEON_CAP0_PRE_FIELD					= BITS(4:4),
		C_RADEON_CAP0_CUR_FIELD					= BITS(5:5),
		C_RADEON_CAP0_PRE_VBI_BUF				= BITS(7:6),
		C_RADEON_CAP0_CUR_VBI_BUF				= BITS(9:8),
		C_RADEON_CAP0_VBI_BUF_STATUS			= BITS(10:10),
		C_RADEON_CAP0_PRE_ANC_BUF				= BITS(12:11),
		C_RADEON_CAP0_CUR_ANC_BUF				= BITS(14:13),
		C_RADEON_CAP0_ANC_BUF_STATUS			= BITS(16:16),
		C_RADEON_CAP0_ANC_PRE_BUF_CNT			= BITS(27:16),
		C_RADEON_CAP0_VIP_INC					= BITS(28:28),
		C_RADEON_CAP0_VIP_PRE_REPEAT_FIELD		= BITS(29:29),
		C_RADEON_CAP0_CAP_BUF_STATUS			= BITS(30:30),
		
	C_RADEON_CAP0_VBI2_OFFSET                   = 0x0980,
	C_RADEON_CAP0_VBI3_OFFSET                   = 0x0984,
	C_RADEON_CAP0_ANC2_OFFSET                   = 0x0988,
	C_RADEON_CAP0_ANC3_OFFSET                   = 0x098C,

	C_RADEON_VBI_BUFFER_CONTROL					= 0x0900,
		C_RADEON_CAP0_BUFFER_WATER_MARK			= BITS(4:0),
		C_RADEON_FULL_BUFFER_EN					= BITS(16:16),
		C_RADEON_CAP0_ANC_VBI_QUAD_BUF			= BITS(17:17),
		C_RADEON_VID_BUFFER_RESET				= BITS(20:20),
		C_RADEON_CAP_SWAP						= BITS(22:21),
		C_RADEON_CAP0_BUFFER_EMPTY_R			= BITS(24:24),
	
	// Test and Debug control
    C_RADEON_TEST_DEBUG_CNTL                    = 0x0120,
        C_RADEON_TEST_DEBUG_OUT_EN              = 0x00000001
};


class CRadeonRect {
public:
	CRadeonRect();

	CRadeonRect(int left, int top, int right, int bottom);

	int Left() const;
	
	int Top() const;
	
	int Right() const;
	
	int Bottom() const;

	int Width() const;
	
	int Height() const;
	
	void SetLeft(int value);
		
	void SetTop(int value);
	
	void SetRight(int value);
	
	void SetBottom(int value);
	
	void SetTo(int left, int top, int right, int bottom);

	void MoveTo(int left, int top);

	void ResizeTo(int width, int height);
			
private:
	int fLeft;	
	int fTop;
	int fRight;	
	int fBottom;	
};

class CRadeon {
public:
	CRadeon( const char *dev_name );
	
	~CRadeon();
	
	status_t InitCheck() const;

	uint32 VirtualMemoryBase() const;
		
public:
	int Register(radeon_register index) const;
	
	void SetRegister(radeon_register index, int value);

	int Register(radeon_register index, int mask) const;
	
	void SetRegister(radeon_register index, int mask, int value);
	
	int VIPRegister(int device, int address);
	
	void SetVIPRegister(int device, int address, int value);
	
	int VIPReadFifo(int device, uint32 address, uint32 count, uint8 *buffer);
	
	int VIPWriteFifo(int device, uint32 address, uint32 count, uint8 *buffer);
	
	int FindVIPDevice( uint32 device_id );
	
public:
	void GetPLLParameters(int & refFreq, int & refDiv, int & minFreq, int & maxFreq, int & xclock);

	void GetMMParameters(radeon_video_tuner & tuner,
						 radeon_video_decoder & video,
						 radeon_video_clock & clock,
						 int & tunerPort,
						 int & compositePort,
						 int & svideoPort);
	
public:
	status_t AllocateGraphicsMemory( 
		memory_type_e memory_type, int32 size,
		int32 *offset, int32 *handle );
		
	void FreeGraphicsMemory( 
		memory_type_e memory_type, int32 handle );
		
	status_t DMACopy( 
		uint32 src, void *target, size_t size, bool lock_mem, bool contiguous );

public:
	status_t GetDeviceInformation(radeon_get_private_data & info);
	
	status_t WaitInterrupt(int * mask, int * sequence, bigtime_t * time, bigtime_t timeout);
	
	status_t CloneArea(const char * name, area_id src_area, 
		area_id *cloned_area, void ** map);
	
	shared_info* GetSharedInfo();
private:
	int fHandle;
	unsigned int * fRegister;
	unsigned char * fROM;
	virtual_card *fVirtualCard;
	shared_info *fSharedInfo;
	
	area_id fRegisterArea;
	area_id fROMArea;
	area_id fVirtualCardArea;
	area_id fSharedInfoArea;
	
	uint32 caps_video_in;
};

template <typename T>
inline T Clamp(T value, T min, T max)
{
	return (value < min ? min : value > max ? max : value);
}

#endif
