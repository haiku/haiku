/*
	Author:
	Rudolf Cornelissen 4/2002-11/2005
*/

#define MODULE_BIT 0x00100000

#include "nv_std.h"

#define PRADR	0x88
#define SCADR	0x8a
#define WR		0x00
#define RD		0x01

enum
{	// TVoutput mode to set
	NOT_SUPPORTED = 0,
	NTSC640_TST,
	NTSC640,
	NTSC800,
	PAL800_TST,
	PAL640,
	PAL800,
	NTSC720,
	PAL720,
	NTSC640_OS,
	PAL800_OS
};

/* Dirk Thierbach's Macro setup for registers 0xda-0xfe.
 * (also see http://sourceforge.net/projects/nv-tv-out/) */
static uint8 BtNtscMacro0 [] = {
  0x0f,0xfc,0x20,0xd0,0x6f,0x0f,0x00,0x00,0x0c,0xf3,0x09,
  0xbd,0x67,0xb5,0x90,0xb2,0x7d,0x00,0x00};
static uint8 BtNtscMacro1 [] = {
  0x0f,0xfc,0x20,0xd0,0x6f,0x0f,0x00,0x00,0x0c,0xf3,0x09,
  0xbd,0x67,0xb5,0x90,0xb2,0x7d,0x63,0x00};
static uint8 BtNtscMacro2 [] = {
  0x0f,0xfc,0x20,0xd0,0x6f,0x0f,0x00,0x00,0x0c,0xf3,0x09,
  0xbd,0x6c,0x31,0x92,0x32,0xdd,0xe3,0x00};
static uint8 BtNtscMacro3 [] = {
  0x0f,0xfc,0x20,0xd0,0x6f,0x0f,0x00,0x00,0x0c,0xf3,0x09,
  0xbd,0x66,0xb5,0x90,0xb2,0x7d,0xe3,0x00};

static uint8 BtPalMacro0 [] = {
  0x05,0x57,0x20,0x40,0x6e,0x7e,0xf4,0x51,0x0f,0xf1,0x05,
  0xd3,0x78,0xa2,0x25,0x54,0xa5,0x00,0x00};
static uint8 BtPalMacro1 [] = {
  0x05,0x57,0x20,0x40,0x6e,0x7e,0xf4,0x51,0x0f,0xf1,0x05,
  0xd3,0x78,0xa2,0x25,0x54,0xa5,0x63,0x00};

static uint8 BT_set_macro (int std, int mode)
{
	uint8 stat;
	uint8 buffer[21];

	LOG(4,("Brooktree: Setting Macro:\n"));

	if ((std < 0) | (std > 1) | (mode < 0) | (mode > 3))
	{
		LOG(4,("Brooktree: Non existing mode or standard selected, aborting.\n"));
		return 0x80;
	}
	
	switch (std)
	{
	case 0:
		/* NTSC */
		switch (mode)
		{
		case 0:
			/* disabled */
			LOG(4,("Brooktree: NTSC, disabled\n"));
			memcpy(&buffer[2], &BtNtscMacro0, 19);
			break;
		case 1:
			/* enabled mode 1 */
			LOG(4,("Brooktree: NTSC, mode 1\n"));
			memcpy(&buffer[2], &BtNtscMacro1, 19);
			break;
		case 2:
			/* enabled mode 2 */
			LOG(4,("Brooktree: NTSC, mode 2\n"));
			memcpy(&buffer[2], &BtNtscMacro2, 19);
			break;
		case 3:
			/* enabled mode 3 */
			LOG(4,("Brooktree: NTSC, mode 3\n"));
			memcpy(&buffer[2], &BtNtscMacro3, 19);
			break;
		}
		break;
	case 1:
		/* PAL */
		switch (mode)
		{
		case 0:
			/* disabled */
			LOG(4,("Brooktree: PAL, disabled\n"));
			memcpy(&buffer[2], &BtPalMacro0, 19);
			break;
		case 1:
		case 2:
		case 3:
			/* enabled */
			LOG(4,("Brooktree: PAL, enabled\n"));
			memcpy(&buffer[2], &BtPalMacro1, 19);
			break;
		}
		break;
	}

	buffer[0] = si->ps.tv_encoder.adress + WR;
	/* select first register to write to */
	buffer[1] = 0xda;

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting Macro\n"));

	return stat;
}//end BT_set_macro.

/*
 see if a (possible) BT/CX chip resides at the given adress.
 Return zero if no errors occurred.
*/
static uint8 BT_check (uint8 bus, uint8 adress)
{
	uint8 buffer[3];

	buffer[0] = adress + WR;
	/* set ESTATUS at b'00'; and enable bt chip-outputs
	 * WARNING:
	 * If bit0 = 0 is issued below (EN_OUT = disabled), the BT will lock SDA
	 * after writing adress $A0 (setting EN_XCLK)!!!
	 * Until a reboot the corresponding I2C bus will be inacessable then!!! */
	buffer[1] = 0xc4;
	/* fixme: if testimage 'was' active txbuffer[3] should become 0x05...
	 * (currently this cannot be detected in a 'foolproof' way so don't touch...) */
	/* (ESTATUS b'0x' means: RX ID and VERSION info later..) */
	buffer[2] = 0x01;

	/* reset status */
	i2c_flag_error (-1);

	/* do check */
	i2c_bstart(bus);
	i2c_writebuffer(bus, buffer, sizeof(buffer));
	i2c_bstop(bus);
	return i2c_flag_error(0);
}

/* identify chiptype */
static uint8 BT_read_type (void)
{
	uint8 id, type, stat;
	uint8 buffer[3];

	/* Make sure a CX (Conexant) chip (if this turns out to be there) is set to
	 * BT-compatibility mode! (This command will do nothing on a BT chip...) */
	buffer[0] = si->ps.tv_encoder.adress + WR;
	/* select CX reg. for BT-compatible readback, video still off */
	buffer[1] = 0x6c;
	/* set it up */
	buffer[2] = 0x02;

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* abort on errors */
	stat = i2c_flag_error(0);
	if (stat) return stat;

	/* Do actual readtype command */
	i2c_bstart(si->ps.tv_encoder.bus);
	/* issue I2C read command */
	i2c_writebyte(si->ps.tv_encoder.bus, si->ps.tv_encoder.adress + RD);
	/* receive 1 byte;
	 * ACK level to TX after last byte to RX should be 1 (= NACK) (see I2C spec). */
	/* note:
	 * While the BT's don't care, CX chips will block the SDA line if
	 * an ACK gets sent! */
	id = i2c_readbyte(si->ps.tv_encoder.bus, true);
	i2c_bstop(si->ps.tv_encoder.bus);
	/* abort on errors */
	stat = i2c_flag_error(0);
	if (stat) return stat;

	/* check type to be supported one */
	type = (id & 0xe0) >> 5;
	if (type > 3)
	{
		LOG(4,("Brooktree: Found unsupported encoder type %d, aborting.\n", type));
		return 0x80;
	}

	/* inform driver about TV encoder found */
	si->ps.tvout = true;
	si->ps.tv_encoder.type = BT868 + type;
	si->ps.tv_encoder.version = id & 0x1f;

	return stat;
}

bool BT_probe()
{
	uint8 bus;
	bool btfound = false;
	bool *i2c_bus = &(si->ps.i2c_bus0);

	LOG(4,("Brooktree: Checking wired I2C bus(ses) for first possible TV encoder...\n"));
	for (bus = 0; bus < 3; bus++)
	{
		if (i2c_bus[bus] && !btfound)
		{
			/* try primary adress on bus */
			if (!BT_check(bus, PRADR))
			{
				btfound = true;
				si->ps.tv_encoder.adress = PRADR;
				si->ps.tv_encoder.bus = bus;
			}
			else
			{
				/* try secondary adress on bus */
				if (!BT_check(bus, SCADR))
				{
					btfound = true;
					si->ps.tv_encoder.adress = SCADR;
					si->ps.tv_encoder.bus = bus;
				}
			}
		}
	}

	/* identify exact TV encoder type */
	if (btfound)
	{
		/* if errors are found, retry */
		/* note:
		 * NACK: occurs on some ASUS V7700 GeForce cards!
		 * (apparantly the video-in chip or another chip resides at 'BT' adresses
		 * there..) */
		uint8 stat;
		uint8 cnt = 0;
		while ((stat = BT_read_type()) && (cnt < 3))
		{
			/* don't retry on unsupported chiptype */
			if (stat == 0x80)
			{
				btfound = 0;
				break;
			}
			cnt++;
		}
		if (stat & 0x7f)
		{
			LOG(4,("Brooktree: Too much errors occurred, aborting.\n"));
			btfound = 0;
		}
	}

	if (btfound)
		LOG(4,("Brooktree: Found TV encoder on bus %d, adress $%02x\n",
			si->ps.tv_encoder.bus, si->ps.tv_encoder.adress));
	else
		LOG(4,("Brooktree: No TV encoder Found\n"));

	return btfound;
}

static uint8 BT_init_PAL640()
{
	uint8 stat;

	uint8 buffer[35];

	LOG(4,("Brooktree: Setting PAL 640x480 desktop mode\n"));

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue I2C write command
	buffer[1] = 0x76;			//select first bt register to write to
	buffer[2] = 0x60;
	buffer[3] = 0x80;
	buffer[4] = 0x8a;
	buffer[5] = 0xa6;
	buffer[6] = 0x68;
	buffer[7] = 0xc1;
	buffer[8] = 0x2e;
	buffer[9] = 0xf2;
	buffer[10] = 0x27;
	buffer[11] = 0x00;
	buffer[12] = 0xb0;
	buffer[13] = 0x0a;
	buffer[14] = 0x0b;
	buffer[15] = 0x71;
	buffer[16] = 0x5a;
	buffer[17] = 0xe0;
	buffer[18] = 0x36;
	buffer[19] = 0x00;
	buffer[20] = 0x50;
	buffer[21] = 0x72;
	buffer[22] = 0x1c;
	buffer[23] = 0x8d;		//chip-pin CLKI is pixel clock (only non-default here!)
	buffer[24] = 0x24;
	buffer[25] = 0xf0;
	buffer[26] = 0x58;
	buffer[27] = 0x81;
	buffer[28] = 0x49;
	buffer[29] = 0x8c;
	buffer[30] = 0x0c;
	buffer[31] = 0x8c;
	buffer[32] = 0x79;
	buffer[33] = 0x26;
	buffer[34] = 0x00;

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting mode PAL640\n"));

	return stat;
}//end BT_init_PAL640.

static uint8 BT_init_PAL800()
{
	uint8 stat;
	
	uint8 buffer[35];

	LOG(4,("Brooktree: Setting PAL 800x600 desktop mode\n"));

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue I2C write command
	buffer[1] = 0x76;			//select first bt register to write to
	buffer[2] = 0x00;
	buffer[3] = 0x20;
	buffer[4] = 0xaa;
	buffer[5] = 0xca;
	buffer[6] = 0x9a;
	buffer[7] = 0x0d;
	buffer[8] = 0x29;
	buffer[9] = 0xfc;
	buffer[10] = 0x39;
	buffer[11] = 0x00;
	buffer[12] = 0xc0;
	buffer[13] = 0x8c;
	buffer[14] = 0x03;
	buffer[15] = 0xee;
	buffer[16] = 0x5f;
	buffer[17] = 0x58;
	buffer[18] = 0x3a;
	buffer[19] = 0x66;
	buffer[20] = 0x96;
	buffer[21] = 0x00;
	buffer[22] = 0x00;
	buffer[23] = 0x90;		//chip-pin CLKI is pixel clock (only non-default here!)
	buffer[24] = 0x24;
	buffer[25] = 0xf0;
	buffer[26] = 0x57;
	buffer[27] = 0x80;
	buffer[28] = 0x48;
	buffer[29] = 0x8c;
	buffer[30] = 0x18;
	buffer[31] = 0x28;
	buffer[32] = 0x87;
	buffer[33] = 0x1f;
	buffer[34] = 0x00;

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting mode PAL800\n"));

	return stat;
}//end BT_init_PAL800.

static uint8 BT_init_NTSC640()
{
	uint8 stat;
	
	uint8 buffer[35];

	LOG(4,("Brooktree: Setting NTSC 640x480 desktop mode\n"));

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue I2C write command
	buffer[1] = 0x76;			//select first bt register to write to
	buffer[2] = 0x00;
	buffer[3] = 0x80;
	buffer[4] = 0x84;
	buffer[5] = 0x96;
	buffer[6] = 0x60;
	buffer[7] = 0x7d;
	buffer[8] = 0x22;
	buffer[9] = 0xd4;
	buffer[10] = 0x27;
	buffer[11] = 0x00;
	buffer[12] = 0x10;
	buffer[13] = 0x7e;
	buffer[14] = 0x03;
	buffer[15] = 0x58;
	buffer[16] = 0x4b;
	buffer[17] = 0xe0;
	buffer[18] = 0x36;
	buffer[19] = 0x92;
	buffer[20] = 0x54;
	buffer[21] = 0x0e;
	buffer[22] = 0x88;
	buffer[23] = 0x8c;		//chip-pin CLKI is pixel clock (only non-default here!)
	buffer[24] = 0x0a;
	buffer[25] = 0xe5;
	buffer[26] = 0x76;
	buffer[27] = 0x79;
	buffer[28] = 0x44;
	buffer[29] = 0x85;
	buffer[30] = 0x00;
	buffer[31] = 0x00;
	buffer[32] = 0x80;
	buffer[33] = 0x20;
	buffer[34] = 0x00;

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting mode NTSC640\n"));

	return stat;
}//end BT_init_NTSC640.

static uint8 BT_init_NTSC800()
{
	uint8 stat;
	
	uint8 buffer[35];

	LOG(4,("Brooktree: Setting NTSC 800x600 desktop mode\n"));

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue I2C write command
	buffer[1] = 0x76;			//select first bt register to write to
	buffer[2] = 0xa0;
	buffer[3] = 0x20;
	buffer[4] = 0xb6;
	buffer[5] = 0xce;
	buffer[6] = 0x84;
	buffer[7] = 0x55;
	buffer[8] = 0x20;
	buffer[9] = 0xd8;
	buffer[10] = 0x39;
	buffer[11] = 0x00;
	buffer[12] = 0x70;
	buffer[13] = 0x42;
	buffer[14] = 0x03;
	buffer[15] = 0xdf;
	buffer[16] = 0x56;
	buffer[17] = 0x58;
	buffer[18] = 0x3a;
	buffer[19] = 0xcd;
	buffer[20] = 0x9c;
	buffer[21] = 0x14;
	buffer[22] = 0x3b;
	buffer[23] = 0x91;		//chip-pin CLKI is pixel clock (only non-default here!)
	buffer[24] = 0x0a;
	buffer[25] = 0xe5;
	buffer[26] = 0x74;
	buffer[27] = 0x77;
	buffer[28] = 0x43;
	buffer[29] = 0x85;
	buffer[30] = 0xba;
	buffer[31] = 0xe8;
	buffer[32] = 0xa2;
	buffer[33] = 0x17;
	buffer[34] = 0x00;

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting mode PAL800\n"));

	return stat;
}//end BT_init_NTSC800.

static uint8 BT_init_PAL720()
{
	uint8 stat;
	
	uint8 buffer[35];

	LOG(4,("Brooktree: Setting PAL 720x576 overscanning DVD mode\n"));

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue I2C write command
	buffer[1] = 0x76;			//select first bt register to write to
	buffer[2] = 0xf0;
	buffer[3] = 0xd0;
	buffer[4] = 0x82;
	buffer[5] = 0x9c;
	buffer[6] = 0x5a;
	buffer[7] = 0x31;
	buffer[8] = 0x16;
	buffer[9] = 0x22;
	buffer[10] = 0xa6;
	buffer[11] = 0x00;
	buffer[12] = 0x78;
	buffer[13] = 0x93;
	buffer[14] = 0x03;
	buffer[15] = 0x71;
	buffer[16] = 0x2a;
	buffer[17] = 0x40;
	buffer[18] = 0x0a;
	buffer[19] = 0x00;
	buffer[20] = 0x50;
	buffer[21] = 0x55;
	buffer[22] = 0x55;
	buffer[23] = 0x8c;		//chip-pin CLKI is pixel clock (only non-default here!)
	buffer[24] = 0x24;
	buffer[25] = 0xf0;
	buffer[26] = 0x59;
	buffer[27] = 0x82;
	buffer[28] = 0x49;
	buffer[29] = 0x8c;
	buffer[30] = 0x8e;
	buffer[31] = 0xb0;
	buffer[32] = 0xe6;
	buffer[33] = 0x28;
	buffer[34] = 0x00;

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting mode PAL720\n"));

	return stat;
}//end BT_init_PAL720.

static uint8 BT_init_NTSC720()
{
	uint8 stat;
	
	uint8 buffer[35];

	LOG(4,("Brooktree: Setting NTSC 720x480 overscanning DVD mode\n"));

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue I2C write command
	buffer[1] = 0x76;		//select first bt register to write to.
	buffer[2] = 0xf0;		//lsb h_clk_o: overscan comp = 0, so h_clk_o = 2 * h_clk_i (VSR=2 = scaling=1)
	buffer[3] = 0xd0;		//lsb h_active: h_active = 720 pixels wide port
	buffer[4] = 0x83;	//scope: OK hsync_width: (hsync_width / h_clk_o) * 63.55556uS = 4.70uS for NTSC
	buffer[5] = 0x98;	//scope: OK	hburst_begin: (hburst_begin / h_clk_o) * 63.55556uS = 5.3uS for NTSC
	buffer[6] = 0x5e;	//scope: OK hburst_end: ((hburst_end + 128) / h_clk_o) * 63.55556uS = 7.94uS for NTSC

	//How to find the correct values for h_blank_o and v_blank_o:
	// 1. Calculate h_blank_o according to initial setting guideline mentioned below;
	// 2. Set v_blank_o in the neighbourhood of $18, so that TV picture does not have ghosts on right side in it while
	//    horizontal position is about OK;
	// 3. Then tune h_blank_o for centered output on scope (look at front porch and back porch);
	// 4. Now shift the TV output using Hsync_offset for centered output while looking at TV (in method 'SetBT_Hphase' above);
	// 5. If no vertical shivering occurs when image is centered, you're done. Else:
	// 6. Modify the RIVA (BeScreen) h_sync_start setting somewhat to get stable centered picture possible on TV AND!:
	// 7. Make sure you update the Chrontel horizontal Phase setting also then!

	if (si->ps.tv_encoder.type >= CX25870)//set CX value
	{
		/* confirmed on NV11 using 4:3 TV and 16:9 TV */
		buffer[7] = 0x0c;	//scope: tuned. lsb h_blank_o: h_blank_o = horizontal viewport location on TV
							//(guideline for initial setting: (h_blank_o / h_clk_0) * 63.55556uS = 9.5uS for NTSC)
	}
	else //set BT value
	{
		/* confirmed on TNT1 using 4:3 TV and 16:9 TV */
		buffer[7] = 0x28;	//scope: tuned. lsb h_blank_o: h_blank_o = horizontal viewport location on TV
							//(guideline for initial setting: (h_blank_o / h_clk_0) * 63.55556uS = 9.5uS for NTSC)
	}
	buffer[8] = 0x18;	//try-out; scope: checked against other modes, looks OK.	v_blank_o: 1e active line ('pixel')

	buffer[9] = 0xf2;		//v_active_o: = (active output lines + 2) / field (on TV)
	buffer[10] = 0x26;		//lsn = msn h_clk_o;
							//b4-5 = msbits h_active;
							//b7 = b8 v_avtive_o.
	buffer[11] = 0x00;		//h_fract is always 0.
	buffer[12] = 0x78;		//lsb h_clk_i: h_clk_i is horizontal total = 888.
	buffer[13] = 0x90; 	//try-out; lsb h_blank_i: #clks between start sync and new line 1st pixel; copy to VGA delta-sync!
	buffer[14] = 0x03;		//b2-0 = msn h_clk_i;
						//try-out: b3 = msn h_blank_i;
							//b4 = vblankdly is always 0.
	buffer[15] = 0x0d;		//lsb v_lines_i: v_lines_i = 525
	buffer[16] = 0x1a; 	//try-out;	v_blank_i: #input lines between start sync and new line (pixel); copy to VGA delta-sync!
						//Make sure though that this value for the BT is *even*, or image will shiver a *lot* horizontally on TV.
	buffer[17] = 0xe0;		//lsb v_active_i: v_active_i = 480
	buffer[18] = 0x36;		//b1-0 = msn v_lines_i;
							//b3-2 = msn v_active_i;
							//b5-4 = ylpf = 3;
							//b7-6 = clpf = 0.		
	buffer[19] = 0x00;		//lsb v_scale: v_scale = off = $1000
	buffer[20] = 0x50;		//b5-0 = msn v_scale;
						//scope: tuned. b7-6 = msn h_blank_o.
							//(((PLL_INT + (PLL_FRACT/65536)) / 6) * 13500000) = PIXEL_CLK = (hor.tot. * v_lines_i * 60Hz)
	buffer[21] = 0x98;		//lsb PLL fract: PLL fract = 0x6e98
	buffer[22] = 0x6e;		//msb PLL fract
	buffer[23] = 0x8c;		//b5-0 = PLL int: PLL int = 0x0c;
							//b6 = by_pll: by_pll = 0;
							//b7 = EN_XCLK: chip-pin CLKI is pixel clock.
	buffer[24] = 0x0a;		//b0 = ni_out is always 0;
							//b1 = setup = 1 for NTSC;
							//b2 = 625line = 0 for NTSC;
							//b3 = vsync_dur = 1 for NTSC;
							//b4 = dic_screset is always 0;
							//b5 = pal_md = 0 for NTSC;
							//b6 = eclip is always 0;
							//b7 = reserved (en_scart) is always 0.
	buffer[25] = 0xe5;		//sync_amp $e5 for NTSC
	buffer[26] = 0x75;		//bst_amp $74-$76 for NTSC
	buffer[27] = 0x78;		//mcr: r-y $77-$79 for NTSC
	buffer[28] = 0x44;		//mcb: b-y $43-$44 for NTSC
	buffer[29] = 0x85;		//my: y $85 for NTSC
	buffer[30] = 0x3c;		//lsb msc: msc b31-0: NTSC formula: ((3579545 / pixelclk) * 2^32) = MSC
	buffer[31] = 0x91;		//msc = $20c2913c
	buffer[32] = 0xc2;
	buffer[33] = 0x20;		//msb msc.
	buffer[34] = 0x00;		//phase_off always $00

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting mode NTSC720\n"));

	return stat;
}//end BT_init_NTSC720.

static uint8 BT_init_PAL800_OS()
{
	uint8 stat;
	
	uint8 buffer[35];

	LOG(4,("Brooktree: Setting PAL 800x600 overscanning VCD mode\n"));

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue I2C write command
	buffer[1] = 0x76;		//select first bt register to write to.
	buffer[2] = 0x60;		//lsb h_clk_o: overscan comp = 0, so h_clk_o = 2 * h_clk_i (VSR=2 = scaling=1)
	buffer[3] = 0x20;		//lsb h_active: h_active = 800 pixels wide port
	buffer[4] = 0x8b;	//scope: OK hsync_width: (hsync_width / h_clk_o) * 64.0uS = 4.70uS for PAL
	buffer[5] = 0xa5;	//scope: OK	hburst_begin: (hburst_begin / h_clk_o) * 64.0uS = 5.6uS for PAL
	buffer[6] = 0x6b;	//scope: OK hburst_end: ((hburst_end + 128) / h_clk_o) * 64.0uS = 7.97uS for PAL

	//How to find the correct values for h_blank_o and v_blank_o:
	// 1. Calculate h_blank_o according to initial setting guideline mentioned below;
	// 2. Set v_blank_o in the neighbourhood of $18, so that TV picture does not have ghosts on right side in it while
	//    horizontal position is about OK;
	// 3. Then tune h_blank_o for centered output on scope (look at front porch and back porch);
	// 4. Now shift the TV output using Hsync_offset for centered output while looking at TV (in method 'SetBT_Hphase' above);
	// 5. If no vertical shivering occurs when image is centered, you're done. Else:
	// 6. Modify the RIVA (BeScreen) h_sync_start setting somewhat to get stable centered picture possible on TV AND!:
	// 7. Make sure you update the Chrontel horizontal Phase setting also then!

	if (si->ps.tv_encoder.type >= CX25870)//set CX value
	{
		/* confirmed on NV11 using 4:3 TV and 16:9 TV */
		buffer[7] = 0xf0;
		buffer[8] = 0x17;
	}
	else //set BT value
	{
		/* confirmed on TNT1 using 4:3 TV and 16:9 TV */
		buffer[7] = 0xd0;//scope: tuned. lsb h_blank_o: h_blank_o = horizontal viewport location on TV
						//(guideline for initial setting: (h_blank_o / h_clk_0) * 64.0uS = 10.0uS for PAL)
		buffer[8] = 0x18;//try-out; scope: checked against other modes, looks OK.	v_blank_o: 1e active line ('pixel')
	}

	buffer[9] = 0x2e;		//v_active_o: = (active output lines + 2) / field (on TV)
	buffer[10] = 0xb7;		//lsn = msn h_clk_o;
							//b4-5 = msbits h_active;
							//b7 = b8 v_avtive_o.
	buffer[11] = 0x00;		//h_fract is always 0.
	buffer[12] = 0xb0;		//lsb h_clk_i: h_clk_i is horizontal total = 944.

	if (si->ps.tv_encoder.type >= CX25870)//set CX value
		buffer[13] = 0x20;
	else //set BT value
		buffer[13] = 0x14;//try-out; lsb h_blank_i: #clks between start sync and new line 1st pixel; copy to VGA delta-sync!

	buffer[14] = 0x03;		//b2-0 = msn h_clk_i;
					 	//try-out: b3 = msn h_blank_i;
							//b4 = vblankdly is always 0.
	buffer[15] = 0x71;		//lsb v_lines_i: v_lines_i = 625

	if (si->ps.tv_encoder.type >= CX25870)//set CX value
		buffer[16] = 0x08;
	else				//set BT value
		buffer[16] = 0x2a;//try-out; v_blank_i: #input lines between start sync and new line (pixel); copy to VGA delta-sync!
					 	//Make sure though that this value for the BT is *even*, or image will shiver a *lot* horizontally on TV.

	buffer[17] = 0x58;		//lsb v_active_i: v_active_i = 600
	buffer[18] = 0x3a;		//b1-0 = msn v_lines_i;
							//b3-2 = msn v_active_i;
							//b5-4 = ylpf = 3;
							//b7-6 = clpf = 0.		
	buffer[19] = 0x00;		//lsb v_scale: v_scale = off = $1000
	buffer[20] = 0x10;		//b5-0 = msn v_scale;
						//scope: tuned. b7-6 = msn h_blank_o.
							//(((PLL_INT + (PLL_FRACT/65536)) / 6) * 13500000) = PIXEL_CLK = (hor.tot. * v_lines_i * 50Hz)
	buffer[21] = 0x72;		//lsb PLL fract: PLL fract = 0x1c72
	buffer[22] = 0x1c;		//msb PLL fract
	buffer[23] = 0x8d;		//b5-0 = PLL int: PLL int = 0x0d;
							//b6 = by_pll: by_pll = 0;
							//b7 = EN_XCLK: chip-pin CLKI is pixel clock.
	buffer[24] = 0x24;		//b0 = ni_out is always 0;
							//b1 = setup = 0 for PAL;
							//b2 = 625line = 1 for PAL;
							//b3 = vsync_dur = 0 for PAL;
							//b4 = dic_screset is always 0;
							//b5 = pal_md = 1 for PAL;
							//b6 = eclip is always 0;
							//b7 = reserved (en_scart) is always 0.
	buffer[25] = 0xf0;		//sync_amp $f0 for PAL
	buffer[26] = 0x57;		//bst_amp $57-$58 for PAL
	buffer[27] = 0x80;		//mcr: r-y $80-$81 for PAL
	buffer[28] = 0x48;		//mcb: b-y $48-$49 for PAL
	buffer[29] = 0x8c;		//my: y $8c for PAL
	buffer[30] = 0x31;		//lsb msc: msc b31-0: PAL formula: ((4433619 / pixelclk) * 2^32) = MSC
	buffer[31] = 0x8c;		//msc = $26798c31
	buffer[32] = 0x79;
	buffer[33] = 0x26;		//msb msc.
	buffer[34] = 0x00;		//phase_off always $00

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting mode PAL800 OS\n"));

	return stat;
}//end BT_init_PAL800_OS.

static uint8 BT_init_NTSC640_OS()
{
	uint8 stat;
	
	uint8 buffer[35];

	LOG(4,("Brooktree: Setting NTSC 640x480 overscanning VCD mode\n"));

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue I2C write command
	buffer[1] = 0x76;		//select first bt register to write to.
	buffer[2] = 0x20;		//lsb h_clk_o: overscan comp = 0, so h_clk_o = 2 * h_clk_i (VSR=2 = scaling=1)
	buffer[3] = 0x80;		//lsb h_active: h_active = 640 pixels wide port
	buffer[4] = 0x74;	//scope: OK hsync_width: (hsync_width / h_clk_o) * 63.55556uS = 4.70uS for NTSC
	buffer[5] = 0x83;	//scope: OK	hburst_begin: (hburst_begin / h_clk_o) * 63.55556uS = 5.3uS for NTSC
	buffer[6] = 0x44;	//scope: OK hburst_end: ((hburst_end + 128) / h_clk_o) * 63.55556uS = 7.94uS for NTSC

	//How to find the correct values for h_blank_o and v_blank_o:
	// 1. Calculate h_blank_o according to initial setting guideline mentioned below;
	// 2. Set v_blank_o in the neighbourhood of $18, so that TV picture does not have ghosts on right side in it while
	//    horizontal position is about OK;
	// 3. Then tune h_blank_o for centered output on scope (look at front porch and back porch);
	// 4. Now shift the TV output using Hsync_offset for centered output while looking at TV (in method 'SetBT_Hphase' above);
	// 5. If no vertical shivering occurs when image is centered, you're done. Else:
	// 6. Modify the RIVA (BeScreen) h_sync_start setting somewhat to get stable centered picture possible on TV AND!:
	// 7. Make sure you update the Chrontel horizontal Phase setting also then!

	buffer[7] = 0xf7;	//scope: tuned. lsb h_blank_o: h_blank_o = horizontal viewport location on TV:
						//(guideline for initial setting: (h_blank_o / h_clk_0) * 63.55556uS = 9.5uS for NTSC)

	if (si->ps.tv_encoder.type >= CX25870)//set CX value
		buffer[8] = 0x1d;
	else //set BT value
		buffer[8] = 0x1c;//try-out; scope: checked against other modes, looks OK.	v_blank_o: 1e active line ('pixel')

	buffer[9] = 0xf2;		//v_active_o: = (active output lines + 2) / field (on TV)
	buffer[10] = 0x26;		//lsn = msn h_clk_o;
							//b4-5 = msbits h_active;
							//b7 = b8 v_avtive_o.
	buffer[11] = 0x00;		//h_fract is always 0.
	buffer[12] = 0x10;		//lsb h_clk_i: h_clk_i is horizontal total = 784.
	buffer[13] = 0x14;	//try-out; lsb h_blank_i: #clks between start sync and new line 1st pixel; copy to VGA delta-sync!
	buffer[14] = 0x03;		//b2-0 = msn h_clk_i;
						//try-out: b3 = msn h_blank_i;
							//b4 = vblankdly is always 0.
	buffer[15] = 0x0d;		//lsb v_lines_i: v_lines_i = 525
	buffer[16] = 0x18;	//try-out;	v_blank_i: #input lines between start sync and new line (pixel); copy to VGA delta-sync!
					 	//Make sure though that this value for the BT is *even*, or image will shiver a *lot* horizontally on TV.
	buffer[17] = 0xe0;		//lsb v_active_i: v_active_i = 480
	buffer[18] = 0x36;		//b1-0 = msn v_lines_i;
							//b3-2 = msn v_active_i;
							//b5-4 = ylpf = 3;
							//b7-6 = clpf = 0.		
	buffer[19] = 0x00;		//lsb v_scale: v_scale = off = $1000
	buffer[20] = 0x10;		//b5-0 = msn v_scale;
						//scope: tuned. b7-6 = msn h_blank_o.
							//(((PLL_INT + (PLL_FRACT/65536)) / 6) * 13500000) = PIXEL_CLK = (hor.tot. * v_lines_i * 60Hz)
	buffer[21] = 0xdb;		//lsb PLL fract: PLL fract = 0xf9db
	buffer[22] = 0xf9;		//msb PLL fract
	buffer[23] = 0x8a;		//b5-0 = PLL int: PLL int = 0x0a;
							//b6 = by_pll: by_pll = 0;
							//b7 = EN_XCLK: chip-pin CLKI is pixel clock.
	buffer[24] = 0x0a;		//b0 = ni_out is always 0;
							//b1 = setup = 1 for NTSC;
							//b2 = 625line = 0 for NTSC;
							//b3 = vsync_dur = 1 for NTSC;
							//b4 = dic_screset is always 0;
							//b5 = pal_md = 0 for NTSC;
							//b6 = eclip is always 0;
							//b7 = reserved (en_scart) is always 0.
	buffer[25] = 0xe5;		//sync_amp $e5 for NTSC
	buffer[26] = 0x75;		//bst_amp $74-$76 for NTSC
	buffer[27] = 0x78;		//mcr: r-y $77-$79 for NTSC
	buffer[28] = 0x44;		//mcb: b-y $43-$44 for NTSC
	buffer[29] = 0x85;		//my: y $85 for NTSC
	buffer[30] = 0x37;		//lsb msc: msc b31-0: NTSC formula: ((3579545 / pixelclk) * 2^32) = MSC
	buffer[31] = 0x12;		//msc = $251b1237
	buffer[32] = 0x1b;
	buffer[33] = 0x25;		//msb msc.
	buffer[34] = 0x00;		//phase_off always $00

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting mode NTSC640 OS\n"));

	return stat;
}//end BT_init_NTSC640_OS.

static uint8 BT_testsignal(void)
{
	uint8 stat;

	uint8 buffer[3];

	LOG(4,("Brooktree: Enabling testsignal\n"));

	buffer[0] = si->ps.tv_encoder.adress + WR;
	/* select bt register for enabling colorbars and outputs */
	buffer[1] = 0xc4;
	/* issue the actual command */
	buffer[2] = 0x05;

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting up flickerfilter and outputs\n"));

	return stat;
}//end BT_testsignal.

static uint8 BT_setup_output(uint8 monstat, uint8 output, uint8 ffilter)
{
	uint8 stat;

	uint8 buffer[7];

	buffer[0] = si->ps.tv_encoder.adress + WR;
	/* select first TV config register to write */
	buffer[1] = 0xc6;
	/* input is 24bit mpx'd RGB, BLANK = out, sync = act. hi */
	buffer[2] = 0x98;
	/* disable all filters, exept flicker filter */
	buffer[3] = 0x98;
	if (!ffilter)
	{
		/* disable flicker filter */ 
		buffer[3] = 0xc0;
		LOG(4,("Brooktree: Disabling flickerfilter\n"));
	}
	else
		LOG(4,("Brooktree: Enabling flickerfilter\n"));

	/* (disable filters) */
	buffer[4] = 0xc0;
	/* (disable filters) */
	buffer[5] = 0xc0;
	switch (output)
	/* Description of ELSA Erazor III hardware layout:
	 * (This is the default (recommended) layout by NVIDIA)
	 * DAC A = CVBS
	 * DAC B = C (chrominance)
	 * DAC C = Y (luminance) */
		
	/* Description of Diamond VIPER550:
	 * DAC A = Not connected
	 * DAC B = C (chrominance)
	 * DAC C = Y (luminance)
	 * To be able to connect to CVBS TV's a special cable is supplied:
	 * This cable connects the Y (DAC C) output to the TV CVBS input. */
	{
	case 1:
		LOG(4,("Brooktree: Forcing both Y/C and CVBS signals where supported by hardware\n"));
		buffer[6] = 0x18;	// Y/C and CVBS out if all ports implemented
							// in hardware, else only Y/C or CVBS out.
		break;
	case 2:
		LOG(4,("Brooktree: Forcing CVBS signals on all outputs\n"));
		buffer[6] = 0x00;	// put CVBS on all outputs. Used for cards
		break;				// with only Y/C out and 'translation cable'.
	default:
		LOG(4,("Brooktree: Outputting signals according to autodetect status:\n"));
		switch (monstat)	// only 'autodetect' remains...
		{
		case 1: 
			LOG(4,("Brooktree: Only Y connected, outputting CVBS on all outputs\n"));
			buffer[6] = 0x00;	//only Y connected: must be CVBS!
			break;
		case 2:
			LOG(4,("Brooktree: Only C connected, outputting CVBS on all outputs\n"));
			buffer[6] = 0x00;	//only C connected: must be CVBS!
			break;				//(though cable is wired wrong...)
		case 5:
			LOG(4,("Brooktree: CVBS and only Y connected, outputting CVBS on all outputs\n"));
			buffer[6] = 0x00;	//CVBS and only Y connected: 2x CVBS!
			break;			   	//(officially not supported...)
		case 6:
			LOG(4,("Brooktree: CVBS and only C connected, outputting CVBS on all outputs\n"));
			buffer[6] = 0x00;	//CVBS and only C connected: 2x CVBS!
			break;			   	//(officially not supported...)
		default:
			LOG(4,("Brooktree: Outputting both Y/C and CVBS where supported by hardware\n"));
			buffer[6] = 0x18;	//nothing, or
							 	//Y/C only, or
							 	//CVBS only (but on CVBS output), or
							 	//Y/C and CVBS connected:
							 	//So activate recommended signals.
		}
	}

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting up flickerfilter and outputs\n"));

	return stat;
}//end BT_setup_output.

static uint8 BT_setup_hphase(uint8 mode)
{
	uint8 stat, hoffset;

	uint8 buffer[7];

	LOG(4,("Brooktree: Tuning horizontal phase\n"));

	/* CX needs timing reset (advised on BT also), first 1mS delay needed! */
	snooze(1000);

	/* values below are all tested on TNT1, TNT2 and GeForce2MX */
	buffer[0] = si->ps.tv_encoder.adress + WR;
	/* select first TV output timing register to write */
	buffer[1] = 0x6c;
	/* turn on active video & generate timing reset on CX chips! */
	buffer[2] = 0x86;
	/* (set fail save values...) */
	buffer[3] = 0x00;		//set default horizontal sync offset
	buffer[4] = 0x02;		//set default horizontal sync width
	buffer[5] = 0x00;		//set default vertical sync offset

	/* do specific timing setup for all chips and modes: */
	switch (si->ps.card_type)
	{
	case NV05:
	case NV05M64:
	case NV15:
		/* confirmed TNT2, TNT2M64, GeForce2Ti.
		 * (8 pixels delayed hpos, so picture more to the right) */
		hoffset = 8;
		break;
	default:
		/* confirmed TNT1, GeForce256, GeForce2MX.
		 * (std hpos)
		 * NOTE: It might be that GeForce needs TNT2 offset:
		 * for now CX chips get seperate extra offset, until sure.
		 * (CX is only found AFAIK on GeForce cards, no BT tested
		 * on GeForce yet. CH was tested on GeForce and seemed to
		 * indicate TNT1 offset was needed.) */
		hoffset = 0;
		break;
	}

	switch (mode)
	{
	case NTSC640_TST:
	case NTSC640:
		if (si->ps.tv_encoder.type >= CX25870) hoffset +=8; //if CX shift picture right some more... 
		/* confirmed on TNT1 with BT869 using 4:3 TV and 16:9 TV */
		buffer[3] = (0x1e + hoffset);	//set horizontal sync offset
		break;
	case NTSC800:
		if (si->ps.tv_encoder.type >= CX25870) hoffset +=8; //if CX shift picture right some more... 
		buffer[3] = (0xe1 + hoffset);	//set horizontal sync offset
		buffer[4] = 0xc2;
		//Vsync offset reg. does not exist on CX: mode is checked and OK.
		buffer[5] = 0x40;				//set VSync offset (on BT's only)
		break;
	case PAL640:
		if (si->ps.tv_encoder.type >= CX25870) hoffset +=8; //if CX shift picture right some more... 
		buffer[3] = (0xa8 + hoffset);
		break;
	case PAL800_TST: 
	case PAL800:
		if (si->ps.tv_encoder.type >= CX25870) hoffset +=8; //if CX shift picture right some more... 
		buffer[3] = (0x2c + hoffset);
		break;
	case NTSC720:
		if (si->ps.tv_encoder.type >= CX25870)
			buffer[3] = (0xb2 + hoffset); //set horizontal sync offset CX
		else
			buffer[3] = (0xd0 + hoffset); //set horizontal sync offset BT
		buffer[4] = 0xff;				//hsync width = max:
		break;							//to prevent vertical image 'shivering'.
	case PAL720:
		buffer[3] = (0xd4 + hoffset);
		buffer[4] = 0xff;
		break;
	case NTSC640_OS:
		buffer[3] = (0xc8 + hoffset);
		buffer[4] = 0xff;
		break;
	case PAL800_OS:
		if (si->ps.tv_encoder.type >= CX25870)
			buffer[3] = (0x78 + hoffset); //set horizontal sync offset CX
		else
			buffer[3] = (0xc4 + hoffset); //set horizontal sync offset BT
		buffer[4] = 0xff;
		break;
	default: //nothing to be done here...
		break;
	}

	buffer[6] = 0x01;		//set default vertical sync width

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
		LOG(4,("Brooktree: I2C errors occurred while setting up h_phase\n"));

	return stat;
}//end BT_setup_hphase.

static uint8 BT_read_monstat(uint8* monstat)
{
	uint8 stat;
	uint8 buffer[3];

	/* make sure we have the recommended failsafe selected */
	*monstat = 0;

	LOG(4,("Brooktree: Autodetecting connected output devices\n"));

	/* set BT to return connection status in ESTATUS on next read CMD: */
	buffer[0] = si->ps.tv_encoder.adress + WR;
	/* set ESTATUS at b'01' (return conn.stat.) */
	buffer[1] = 0xc4;
	/* and leave chip outputs on. */
	buffer[2] = 0x41;

	/* reset status */
	i2c_flag_error (-1);

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
	{
		LOG(4,("Brooktree: I2C errors occurred while reading connection status (1)\n"));
		return stat;
	}

	/* do actual read connection status: */
	buffer[0] = si->ps.tv_encoder.adress + WR;
	/* select register with CHECK_STAT CMD */
	buffer[1] = 0xba;
	/* issue actual command. */
	buffer[2] = 0x40;

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
	{
		LOG(4,("Brooktree: I2C errors occurred while reading connection status (2)\n"));
		return stat;
	}

	/* CX: Wait 600uS for signals to stabilize (see datasheet) */
	/* warning, note:
	 * datasheet is in error! 60mS needed!! */
	snooze(60000);

	/* read back updated connection status: */
	buffer[0] = si->ps.tv_encoder.adress + RD;

	/* transmit 1 byte */
	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, 1);

	/* receive 1 byte */
	/* ACK level to TX after last byte to RX should be 1 (= NACK) (see I2C spec)
	 * While the BT's don't care, CX chips will block the SDA line if an ACK gets sent! */
	buffer[0] = 1;
	i2c_readbuffer(si->ps.tv_encoder.bus, buffer, 1);
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
	{
		LOG(4,("Brooktree: I2C errors occurred while reading connection status (3)\n"));
		return stat;
	}

	*monstat = ((buffer[0] & 0xe0) >> 5);
	LOG(4,("Brooktree: TV output monitor status = %d\n", *monstat));

	/* instruct BT to go back to normal operation: */
	buffer[0] = si->ps.tv_encoder.adress + WR;
	/* select register with CHECK_STAT CMD */
	buffer[1] = 0xba;
	/* issue actual command. */
	buffer[2] = 0x00;

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
	{
		LOG(4,("Brooktree: I2C errors occurred while reading connection status (4)\n"));
		return stat;
	}

	return stat;
}//end BT_read_monstat.

static uint8 BT_killclk_blackout(void)
{
	uint8 stat;

	uint8 buffer[4];

	LOG(4,("Brooktree: Killing clock and/or blacking out (blocking output signals)\n"));

	/* reset status */
	i2c_flag_error (-1);

	if (si->ps.tv_encoder.type <= BT869) //BT...
	{
		/* Only disable external pixelclock input on BT's.
		 * CX chips will lock the bus if you do this.
		 * (It looks like the external pixelclock is always OK as long as a valid 
		 * mode is programmed for the TVout chip. This means that disabling the use
		 * of this clock is not needed anyway.
		 * If you do disable this input, this pixelclock will rise to about 60Mhz BTW..) */

		/* disable use of external pixelclock source... */
		/* (should prevent BT for being 'overclocked' by RIVA in VGA-only mode...) */
		buffer[0] = si->ps.tv_encoder.adress + WR;
		/* select BT register for setting EN_XCLK */
		buffer[1] = 0xa0;
		/* clear it */
		buffer[2] = 0x00;

		i2c_bstart(si->ps.tv_encoder.bus);
		i2c_writebuffer(si->ps.tv_encoder.bus, buffer, 3);
		i2c_bstop(si->ps.tv_encoder.bus);
		/* log on errors */
		stat = i2c_flag_error(0);
		if (stat)
		{
			LOG(4,("Brooktree: I2C errors occurred while doing killclk_blackout (1-BT)\n"));
			return stat;
		}
	}
	else //CX...
	{
		/* Disable CX video out (or wild output will be seen on TV..) */
		buffer[0] = si->ps.tv_encoder.adress + WR;
		/* select register in CX */
		buffer[1] = 0x6c;
		/* disable active video out. */
		buffer[2] = 0x02;
	
		i2c_bstart(si->ps.tv_encoder.bus);
		i2c_writebuffer(si->ps.tv_encoder.bus, buffer, 3);
		i2c_bstop(si->ps.tv_encoder.bus);
		/* log on errors */
		stat = i2c_flag_error(0);
		if (stat)
		{
			LOG(4,("Brooktree: I2C errors occurred while doing killclk_blackout (1-CX)\n"));
			return stat;
		}
	}

	/* black-out TVout while outputs are enabled... */
	buffer[0] = si->ps.tv_encoder.adress + WR;
	/* select first TV config register to write */
	buffer[1] = 0xc4;
	/* disable testimage while outputs remain enabled */
	buffer[2] = 0x01;
	/* input is 24bit mpx'd RGB, BLANK = in, sync = act. hi */
	buffer[3] = 0x18;

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, sizeof(buffer));
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
	{
		LOG(4,("Brooktree: I2C errors occurred while doing killclk_blackout (2)\n"));
		return stat;
	}

	return stat;
}//end BT_killclk_blackout.

uint8 BT_dpms(bool display)
{
	uint8 stat;

	uint8 buffer[3];

	LOG(4,("Brooktree: setting DPMS: "));

	/* reset status */
	i2c_flag_error (-1);

	/* shutdown all analog electronics... */
	buffer[0] = si->ps.tv_encoder.adress + WR;
	/* select first TV config register to write */
	buffer[1] = 0xba;
	if (display)
	{
		/* enable all DACs */
		buffer[2] = 0x00;
		LOG(4,("display on\n"));
	}
	else
	{
		/* shutdown all DACs */
		buffer[2] = 0x10;
		LOG(4,("display off\n"));
	}

	i2c_bstart(si->ps.tv_encoder.bus);
	i2c_writebuffer(si->ps.tv_encoder.bus, buffer, 3);
	i2c_bstop(si->ps.tv_encoder.bus);
	/* log on errors */
	stat = i2c_flag_error(0);
	if (stat)
	{
		LOG(4,("Brooktree: I2C errors occurred while setting DPMS\n"));
		return stat;
	}

	return stat;
}//end BT_dpms.

uint8 BT_check_tvmode(display_mode target)
{
	uint8 status = NOT_SUPPORTED;
	uint32 mode = ((target.timing.h_display) | ((target.timing.v_display) << 16));

	switch (mode)
	{
	case (640 | (480 << 16)):
		if (((target.flags & TV_BITS) == TV_PAL) && (!(target.flags & TV_VIDEO)))
			status = PAL640;
		if ((target.flags & TV_BITS) == TV_NTSC)
		{
			if (!(target.flags & TV_VIDEO)) status = NTSC640;
			else status = NTSC640_OS;
		}
		break;
	case (768 | (576 << 16)):
		if (((target.flags & TV_BITS) == TV_PAL) && (target.flags & TV_VIDEO))
			status = PAL800_OS;
		break;
	case (800 | (600 << 16)):
		if (((target.flags & TV_BITS) == TV_PAL) && (!(target.flags & TV_VIDEO)))
			status = PAL800;
		if (((target.flags & TV_BITS) == TV_NTSC) && (!(target.flags & TV_VIDEO)))
			status = NTSC800;
		break;
	case (720 | (480 << 16)):
		if (((target.flags & TV_BITS) == TV_NTSC) && (target.flags & TV_VIDEO))
			status = NTSC720;
		break;
	case (720 | (576 << 16)):
		if (((target.flags & TV_BITS) == TV_PAL) && (target.flags & TV_VIDEO))
			status = PAL720;
		break;
	}

	return status;
}//end BT_check_tvmode.


/*
//BeTVOut's SwitchRIVAtoTV(vtot) timing formula: (note: vtot = (v_total - 2))
//-----------------------------------------------------------------------------------
//HORIZONTAL:
//-----------
h_sync_start = h_display; 

//fixme, note, checkout:
//feels like in fact TNT2-M64 nv_crtc.c registerprogramming should be adapted...
if (TNT2-M64)
{
	h_sync_end = h_display + 8;
	h_total = h_display + 56;
}
else //TNT1, TNT2, Geforce2... (so default)
{
	h_sync_end = h_display + 16;
	h_total = h_display + 48;
}

//fixme, note, checkout:
//BeTVOut uses two 'tweaks':
// - on TNT2-M64 only:
//   register h_blank_e is increased with 1 (so should be done in nv_crtc.c here)
// - 'all cards':
//   register h_blank_e b6 = 0 (only influences TNT2-M64 in modes NTSC800 and PAL800).
//-----------------------------------------------------------------------------------
//VERTICAL:
//---------
v_sync_start = v_display;
v_total = vtot + 2;
v_sync_end = v_total - 1; //(This takes care of the 'cursor trash' on TNT1's...)
//-----------------------------------------------------------------------------------
*/
static status_t BT_update_mode_for_gpu(display_mode *target, uint8 tvmode)
{
	//fixme if needed:
	//pixelclock is not actually pgm'd because PLL is pgm' earlier during setmode...
	switch (tvmode)
	{
	case NTSC640:
	case NTSC640_TST:
		target->timing.h_display = 640;
		target->timing.h_sync_start = 640;
		if (si->ps.card_type == NV05M64)
		{
			target->timing.h_sync_end = 648;
			target->timing.h_total = 696;
		}
		else
		{
			//fixme if possible:
			//see if tweaking h_sync_end can shift picture 8 pixels right to fix
			//ws tv's tuning fault (always going for max. compatibility :)
			target->timing.h_sync_end = 656;
			target->timing.h_total = 688;
		}
		target->timing.v_display = 480;
		target->timing.v_sync_start = 480;
		target->timing.v_sync_end = 555;	//This prevents 'cursor trash' on TNT1's
		target->timing.v_total = 556;		//Above 525 because mode scales down
		if (si->ps.card_type == NV05M64)
			target->timing.pixel_clock = ((696 * 556 * 60) / 1000);
		else
			target->timing.pixel_clock = ((688 * 556 * 60) / 1000);
		break;
	case NTSC800:
		target->timing.h_display = 800;
		target->timing.h_sync_start = 800;
		if (si->ps.card_type == NV05M64)
		{
			target->timing.h_sync_end = 808;
			target->timing.h_total = 856;
		}
		else
		{
			target->timing.h_sync_end = 816;
			target->timing.h_total = 848;
		}
		target->timing.v_display = 600;
		target->timing.v_sync_start = 600;
		target->timing.v_sync_end = 685;	//This prevents 'cursor trash' on TNT1's
		target->timing.v_total = 686;		//Above 525 because mode scales down
		if (si->ps.card_type == NV05M64)
			target->timing.pixel_clock = ((856 * 686 * 60) / 1000);
		else
			target->timing.pixel_clock = ((848 * 686 * 60) / 1000);
		break;
	case PAL640:
		target->timing.h_display = 640;
		target->timing.h_sync_start = 640;
		if (si->ps.card_type == NV05M64)
		{
			target->timing.h_sync_end = 648;
			target->timing.h_total = 696;
		}
		else
		{
			target->timing.h_sync_end = 656;
			target->timing.h_total = 688;
		}
		target->timing.v_display = 480;
		target->timing.v_sync_start = 480;
		target->timing.v_sync_end = 570;	//This prevents 'cursor trash' on TNT1's
		target->timing.v_total = 571;		//Below 625 because mode scales up
		if (si->ps.card_type == NV05M64)
			target->timing.pixel_clock = ((696 * 571 * 50) / 1000);
		else
			target->timing.pixel_clock = ((688 * 571 * 50) / 1000);
		break;
	case PAL800:
	case PAL800_TST:
		target->timing.h_display = 800;
		target->timing.h_sync_start = 800;
		if (si->ps.card_type == NV05M64)
		{
			target->timing.h_sync_end = 808;
			target->timing.h_total = 856;
		}
		else
		{
			target->timing.h_sync_end = 816;
			target->timing.h_total = 848;
		}
		target->timing.v_display = 600;
		target->timing.v_sync_start = 600;
		target->timing.v_sync_end = 695;	//This prevents 'cursor trash' on TNT1's
		target->timing.v_total = 696;		//Above 625 because mode scales down
		if (si->ps.card_type == NV05M64)
			target->timing.pixel_clock = ((856 * 696 * 50) / 1000);
		else
			target->timing.pixel_clock = ((848 * 696 * 50) / 1000);
		break;
	case NTSC640_OS:
		target->timing.h_display = 640;			//BT H_ACTIVE
		target->timing.h_sync_start = 744;		//set for CH/BT compatible TV output
		target->timing.h_sync_end = 744+20;		//delta is BT H_BLANKI
		target->timing.h_total = 784;			//BT H_CLKI
		target->timing.v_display = 480;			//BT  V_ACTIVEI
		target->timing.v_sync_start = 490;		//set for centered sync pulse
		target->timing.v_sync_end = 490+25;		//delta is BT V_BLANKI
		target->timing.v_total = 525;			//BT V_LINESI (== 525: 1:1 scaled mode)
		target->timing.pixel_clock = ((784 * 525 * 60) / 1000); //refresh
		break;
	case PAL800_OS:
		target->timing.h_display = 768;			//H_ACTIVE
		if (si->ps.tv_encoder.type <= BT869)
		{
			/* confirmed on TNT1 using 4:3 TV and 16:9 TV */
			target->timing.h_sync_start = 856;		//set for centered TV output
			target->timing.h_sync_end = 856+20;		//delta is BT H_BLANKI
		}
		else
		{
			/* confirmed on NV11 using 4:3 TV and 16:9 TV */
			target->timing.h_sync_start = 848;		//set for centered TV output
			target->timing.h_sync_end = 848+20;		//delta is BT H_BLANKI
		}
		target->timing.h_total = 944;			//BT H_CLKI
		target->timing.v_display = 576;			//V_ACTIVEI
		target->timing.v_sync_start = 579;		//set for centered sync pulse
		target->timing.v_sync_end = 579+42;		//delta is BT V_BLANKI
		target->timing.v_total = 625;			//BT V_LINESI (== 625: 1:1 scaled mode)
		target->timing.pixel_clock = ((944 * 625 * 50) / 1000); //refresh
		break;
	case NTSC720:
		/* (tested on TNT2 with BT869) */
		target->timing.h_display = 720;			//H_ACTIVE
		if (si->ps.tv_encoder.type <= BT869)
		{
			/* confirmed on TNT1 using 4:3 TV and 16:9 TV */
			target->timing.h_sync_start = 744;		//do not change!
			target->timing.h_sync_end = 744+144;	//delta is H_sync_pulse
		}
		else
		{
			/* confirmed on NV11 using 4:3 TV and 16:9 TV */
			target->timing.h_sync_start = 728;		//do not change!
			target->timing.h_sync_end = 728+160;	//delta is H_sync_pulse
		}
		target->timing.h_total = 888;			//BT H_TOTAL
		target->timing.v_display = 480;			//V_ACTIVEI
		target->timing.v_sync_start = 490;		//set for centered sync pulse
		target->timing.v_sync_end = 490+26;		//delta is V_sync_pulse
		target->timing.v_total = 525;			//CH V_TOTAL (== 525: 1:1 scaled mode)
		target->timing.pixel_clock = ((888 * 525 * 60) / 1000); //refresh
		break;
	case PAL720:
		target->timing.h_display = 720;			//BT H_ACTIVE
		target->timing.h_sync_start = 744;		//set for centered sync pulse
		target->timing.h_sync_end = 744+140;	//delta is BT H_BLANKI
		target->timing.h_total = 888;			//BT H_CLKI
		target->timing.v_display = 576;			//BT  V_ACTIVEI
		target->timing.v_sync_start = 579;		//set for centered sync pulse
		target->timing.v_sync_end = 579+42;		//delta is BT V_BLANKI
		target->timing.v_total = 625;			//BT V_LINESI (== 625: 1:1 scaled mode)
		target->timing.pixel_clock = ((888 * 625 * 50) / 1000); //refresh
		break;
	default:
		return B_ERROR;
	}

	return B_OK;
}//end BT_update_mode_for_gpu.

/* note:
 * tested on ELSA Erazor III 32Mb AGP (TNT2/BT869),
 * Diamond Viper V550 16Mb PCI (TNT1/BT869),
 * and ASUS V7100 GeForce2 MX200 AGP/32Mb (CH7007). */
static status_t BT_start_tvout(display_mode tv_target)
{
	/* TV_PRIMARY tells us that the head to be used with TVout is the head that's
	 * actually assigned as being the primary head at powerup:
	 * so non dualhead-mode-dependant, and not 'fixed' CRTC1! */
	if (tv_target.flags & TV_PRIMARY)
	{
		if ((tv_target.flags & DUALHEAD_BITS) != DUALHEAD_SWITCH)
			head1_start_tvout();
		else
			head2_start_tvout();
	}
	else
	{
		if ((tv_target.flags & DUALHEAD_BITS) != DUALHEAD_SWITCH)
			head2_start_tvout();
		else
			head1_start_tvout();
	}

	return B_OK;
}//end BT_start_tvout.

/* note:
 * tested on ELSA Erazor III 32Mb AGP (TNT2/BT869),
 * Diamond Viper V550 16Mb PCI (TNT1/BT869),
 * and ASUS V7100 GeForce2 MX200 AGP/32Mb (CH7007). */
status_t BT_stop_tvout(void)
{
	/* prevent BT from being overclocked by VGA-only modes & black-out TV-out */
	BT_killclk_blackout();

	/* TV_PRIMARY tells us that the head to be used with TVout is the head that's
	 * actually assigned as being the primary head at powerup:
	 * so non dualhead-mode-dependant, and not 'fixed' CRTC1! */
	if (si->dm.flags & TV_PRIMARY)
	{
		if ((si->dm.flags & DUALHEAD_BITS) != DUALHEAD_SWITCH)
			head1_stop_tvout();
		else
			head2_stop_tvout();
	}
	else
	{
		if ((si->dm.flags & DUALHEAD_BITS) != DUALHEAD_SWITCH)
			head2_stop_tvout();
		else
			head1_stop_tvout();
	}

	/* fixme if needed:
	 * a full encoder chip reset could be done here (so after decoupling crtc)... */
	/* (but: beware of the 'locked SDA' syndrome then!) */

	/* fixme if needed: we _could_ setup a TVout mode and apply the testsignal here... */
	if (0)
	{
		//set mode (selecting PAL/NTSC according to board wiring for example) etc, then:
		BT_testsignal();
	}

	return B_OK;
}//end BT_stop_tvout.

status_t BT_setmode(display_mode target)
{
	uint8 tvmode, monstat;
	/* enable flickerfilter in desktop modes, disable it in video modes. */
	uint8 ffilter = 0;

	/* use a display_mode copy because we might tune it for TVout compatibility */
	display_mode tv_target = target;

	/* preset new TVout mode */
	tvmode = BT_check_tvmode(tv_target);
	if (!tvmode) return B_ERROR;

	/* read current output devices connection status */
	BT_read_monstat(&monstat);

	/* (pre)set TV mode */
	/* note:
	 * Manual config is non-dependent of the state of the PAL hardware input pin;
	 * Also SDA lockups occur when setting EN_XCLK after autoconfig!
	 * Make sure PAL_MD=0 for NTSC and PAL_MD = 1 for PAL... */
	switch (tvmode)
	{
	case NTSC640:
	case NTSC640_TST:
		ffilter = 1;
		BT_init_NTSC640();
		break;
	case NTSC800:
		ffilter = 1;
		BT_init_NTSC800();
		break;
	case PAL640:
		ffilter = 1;
		BT_init_PAL640();
		break;
	case PAL800:
	case PAL800_TST:
		ffilter = 1;
		BT_init_PAL800();
		break;
	case NTSC640_OS:
		BT_init_NTSC640_OS();
		break;
	case PAL800_OS:
		BT_init_PAL800_OS();
		break;
	case NTSC720:
		BT_init_NTSC720();
		break;
	case PAL720:
		BT_init_PAL720();
		break;
	}

	/* modify BT Hphase signal to center TV image... */
	BT_setup_hphase(tvmode);

	/* disable Macro mode */
	switch (tvmode)
	{
	case NTSC640:
	case NTSC640_TST:
	case NTSC800:
	case NTSC640_OS:
	case NTSC720:
		/* NTSC */
		BT_set_macro (0, 0);
		break;
	default:
		/* PAL */
		BT_set_macro (1, 0);
		break;
	}

	/* setup output signal routing and flickerfilter */
	BT_setup_output(monstat, (uint8)(si->settings.tv_output), ffilter);

	/* update the GPU CRTC timing for the requested mode */
	BT_update_mode_for_gpu(&tv_target, tvmode);

	/* setup GPU CRTC timing */
	/* TV_PRIMARY tells us that the head to be used with TVout is the head that's
	 * actually assigned as being the primary head at powerup:
	 * so non dualhead-mode-dependant, and not 'fixed' CRTC1! */
	if (tv_target.flags & TV_PRIMARY)
	{
		if ((tv_target.flags & DUALHEAD_BITS) != DUALHEAD_SWITCH)
			head1_set_timing(tv_target);
		else
			head2_set_timing(tv_target);
	}
	else
	{
		if ((tv_target.flags & DUALHEAD_BITS) != DUALHEAD_SWITCH)
			head2_set_timing(tv_target);
		else
			head1_set_timing(tv_target);
	}

	/* now set GPU CRTC to slave mode */
	BT_start_tvout(tv_target);

	return B_OK;
}
