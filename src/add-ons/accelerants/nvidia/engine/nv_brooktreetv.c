/*
	Author:
	Rudolf Cornelissen 4/2002-9/2005
*/

#define MODULE_BIT 0x00100000

#include "nv_std.h"

#define PRADR	0x88
#define SCADR	0x8a
#define WR		0x00
#define RD		0x01

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
	 * Until a reboot the corresponding IIC bus will be inacessable then!!! */
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
	/* issue IIC read command */
	i2c_writebyte(si->ps.tv_encoder.bus, si->ps.tv_encoder.adress + RD);
	/* receive 1 byte;
	 * ACK level to TX after last byte to RX should be 1 (= NACK) (see IIC spec). */
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
	bool btfound = false;

	LOG(4,("Brooktree: Checking IIC bus(ses) for first possible TV encoder...\n"));
	if (si->ps.i2c_bus0)
	{
		/* try primary adress on bus 0 */
		if (!BT_check(0, PRADR))
		{
			btfound = true;
			si->ps.tv_encoder.adress = PRADR;
			si->ps.tv_encoder.bus = 0;
		}
		else
		{
			/* try secondary adress on bus 0 */
			if (!BT_check(0, SCADR))
			{
				btfound = true;
				si->ps.tv_encoder.adress = SCADR;
				si->ps.tv_encoder.bus = 0;
			}
		}
	}

	if (si->ps.i2c_bus1 && !btfound)
	{
		/* try primary adress on bus 1 */
		if (!BT_check(1, PRADR))
		{
			btfound = true;
			si->ps.tv_encoder.adress = PRADR;
			si->ps.tv_encoder.bus = 1;
		}
		else
		{
			/* try secondary adress on bus 1 */
			if (!BT_check(1, SCADR))
			{
				btfound = true;
				si->ps.tv_encoder.adress = SCADR;
				si->ps.tv_encoder.bus = 1;
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
			LOG(4,("Brooktree: too much errors occurred, aborting.\n"));
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

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue IIC write command
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

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue IIC write command
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

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue IIC write command
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

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue IIC write command
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

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue IIC write command
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

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue IIC write command
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

	buffer[7] = 0x28;	//scope: tuned. lsb h_blank_o: h_blank_o = horizontal viewport location on TV
						//(guideline for initial setting: (h_blank_o / h_clk_0) * 63.55556uS = 9.5uS for NTSC)
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

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue IIC write command
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
		buffer[7] = 0xf4;
		buffer[8] = 0x17;
	}
	else
	{	//set BT value
		buffer[7] = 0xc0;//scope: tuned. lsb h_blank_o: h_blank_o = horizontal viewport location on TV
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
	else				//set BT value
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

	buffer[0] = si->ps.tv_encoder.adress + WR;	//issue IIC write command
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
	else				//set BT value
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

//fixme: resetup for brooktreetv, should cover all supported encoders later on..
int maventv_init(display_mode target)
{
	/* use a display_mode copy because we might tune it for TVout compatibility */
	display_mode tv_target = target;

	/* preset new TVout mode */
	if ((tv_target.flags & TV_BITS) == TV_PAL)
	{
		LOG(4, ("MAVENTV: PAL TVout\n"));
	}
	else
	{
		LOG(4, ("MAVENTV: NTSC TVout\n"));
	}

	/* tune new TVout mode */

	/* set flickerfilter */

	/* output: SVideo/Composite */

	/* calculate vertical sync point */

	/* program new TVout mode */

	/* setup CRTC timing */
	head2_set_timing(tv_target);

	/* start whole thing if needed */

	return 0;
}
