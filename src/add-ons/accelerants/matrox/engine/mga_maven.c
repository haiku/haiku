/* program the MAVEN in monitor mode */
/* Thanx to Petr Vandrovec for info on the MAVEN */
/* Mark Watson 6/2000 */

#define MODULE_BIT 0x00001000

#include "mga_std.h"

status_t gx00_maven_dpms(uint8 display,uint8 h,uint8 v)
{
	if (display&h&v)
	{
		MAVW(MONEN,0xb2);
		MAVW(MONSET,0x20);            /*must be set to this in monitor mode*/
		MAVW(OUTMODE,3);              /*monitor mode*/
		MAVW(STABLE,0x22);            /*makes picture stable?*/
		MAVW(TEST,0x00);              /*turn off test signal*/
	}
	else
	{
		/*turn off screen using a few methods!*/
		MAVW(STABLE,0x6a);
//		MAVW(TEST,0x3);
		MAVW(OUTMODE,0x00);
	}

	return B_OK;
}

/*set a mode line - inputs are in pixels/scanlines*/
status_t gx00_maven_set_timing(
	uint32 hdisp_e,uint32 hsync_s,uint32 hsync_e,uint32 htotal,
	uint32 vdisp_e,uint32 vsync_s,uint32 vsync_e,uint32 vtotal,
	uint8 hsync_pos,uint8 vsync_pos
	)
{
	LOG(4,("MAVEN: setting timing\n"));

	/*check horizontal timing parameters are to nearest 8 pixels*/
	if ((hdisp_e&7)|(hsync_s&7)|(hsync_e&7)|(htotal&7))
	{
		LOG(8,("MAVEN:Horizontal timings are not multiples of 8 pixels\n"));
		return B_ERROR;
	}

	/*program the MAVEN*/
	MAVWW(LASTLINEL,htotal); 
	MAVWW(HSYNCLENL,(hsync_e-hsync_s));
	MAVWW(HSYNCSTRL,(htotal-hsync_s));
	MAVWW(HDISPLAYL,(htotal-hsync_s+hdisp_e));
	MAVWW(HTOTALL,(htotal+1));

	MAVWW(VSYNCLENL,(vsync_e-vsync_s-1));
	MAVWW(VSYNCSTRL,(vtotal-vsync_s));
	MAVWW(VDISPLAYL,(vtotal-1));
	MAVWW(VTOTALL,(vtotal-1));

	MAVWW(HVIDRSTL,(htotal-si->crtc_delay));
	MAVWW(VVIDRSTL,(vtotal-2));

	return B_OK;
}

void gx00_maven_delay(int number)
{
	MAVWW(HVIDRSTL,(si->dm.timing.h_total-number));
}

/*set the mode, brightness is a value from 0->2 (where 1 is equivalent to direct)*/
status_t gx00_maven_mode(int mode,float brightness)
{
	uint8 luma;

	/*set luma to a suitable value for brightness*/
	/*assuming 1A is a sensible value*/
	luma = (uint8)(0x1a * brightness);
	MAVW(LUMA,luma);
	LOG(4,("MAVEN: LUMA setting - %x\n",luma));

	return B_OK;
}

/*program the pixpll on the maven - frequency in kHz*/
status_t gx00_maven_set_pix_pll(float f_vco)
{
	uint8 m=0,n=0,p=0;

	float pix_setting;
	status_t result;

	LOG(4,("MAVEN:Setting PIX PLL %fMHz\n", f_vco));

	result = gx00_maven_pix_pll_find(f_vco,&pix_setting,&m,&n,&p);
	if (result != B_OK)
	{
		return result;
	}
	
	/*reprogram (select,wait for stability)*/
	MAVW(PIXPLLM,(m));				/*set m value*/
	MAVW(PIXPLLN,(n));				/*set n value*/
	MAVW(PIXPLLP,(p|0x80));                              /*set p value*/
	LOG(2,("MAVEN: Clocks found: %x %x %x\n",m,n,p));
	delay(1000);					/*wait 1000us for PIXPLL to lock (no way of knowing)*/
	LOG(2,("MAVEN: PIX PLL frequency locked\n"));

	return B_OK;
}

/*find nearest valid pix pll*/
status_t gx00_maven_pix_pll_find(float f_vco,float * result,uint8 * m_result,uint8 * n_result,uint8 * p_result)
{
	float f_ref=27.000;
	int n_min=4;
	int n_max=127;
	int m_min=2;
	int m_max=31;
	int m=0,n=0,p=0;
	float error;
	float error_best;
	int best[3]; 
	float f_rat;

	LOG(4,("MAVEN:Checking PIX PLL %fMHz\n", f_vco));

	/*f_rat.m/p=n where m=M+1,n=N+1,p=P+1,f_rat=f_vco/f_ref*/
	f_rat = f_vco/f_ref;
	error_best = 999999999;
	for (p=0x2 -1*(f_vco>80.0);p<0x10;p=p<<1)
	{
		for (m=m_min;m<m_max;m++)
		{
			/*calculate n for this m & p (and check for validity)*/
			n=(int)((f_rat*m*p)+0.5);
			if (n>n_max || n<n_min)
				continue;
		
			/*find error in frequency this gives*/
			error=fabs(((f_ref*n)/(m*p))-f_vco);
			if (error<error_best)
			{
				error_best = error;
				best[0]=m;
				best[1]=n;
				best[2]=p;
			}
		}
	}
	m=best[0];
	n=best[1];
	p=best[2];

	/*calculate value of s for fvco, not sure if these are correct*/
	for(;;)/*set loop filter bandwidth -> spot the Perl coder :-)*/
	{
		if(f_vco>180) {p|=0x18;break;};
		if(f_vco>140) {p|=0x10;break;};
		if(f_vco>100) {p|=0x08;break;};
		break;
	}

	/*set the result*/
	*result = (float) (f_ref*n)/(m*(p&0x7));
	*m_result = m-1;
	*n_result = n-1;
	*p_result = p-1;

	/*display the found value*/
	LOG(4,("MAVEN: pixpllcheck - requested %fMHz got %fMHz\n",(double)f_vco,*result));

	return B_OK;
}
