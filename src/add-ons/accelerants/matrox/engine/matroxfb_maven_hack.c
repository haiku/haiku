#define MODULE_BIT 0x00100000

#include "mga_std.h"
#include "matroxfb_maven_hack.h"

#define MODE_PAL	1
#define MODE_NTSC	2
#define MODE_TV(x)	(((x) == MODE_PAL) || ((x) == MODE_NTSC))
#define MODE_MONITOR	128

static int mode = MODE_PAL;

static const struct matrox_pll_features maven_pll = {
	50000,
	27000,
	4, 127,
	2, 31,
	3
};

static const struct matrox_pll_features2 maven1000_pll = {
	 50000000,
	300000000,
	 5, 128,
	 3,  32,
	 3
};

static const struct matrox_pll_ctl maven_PAL = {
	540000,
	    50
};

static const struct matrox_pll_ctl maven_NTSC = {
	450450,	/* 27027000/60 == 27000000/59.94005994 */
	    60
};


int matroxfb_PLL_calcclock(const struct matrox_pll_features* pll, unsigned int freq, unsigned int fmax,
		unsigned int* in, unsigned int* feed, unsigned int* post) {
	unsigned int bestdiff = ~0;
	unsigned int bestvco = 0;
	unsigned int fxtal = pll->ref_freq;
	unsigned int fwant;
	unsigned int p;

	LOG(4,("PLL_calcclock"));

	fwant = freq;

	for (p = 1; p <= pll->post_shift_max; p++) {
		if (fwant * 2 > fmax)
			break;
		fwant *= 2;
	}
	if (fwant < pll->vco_freq_min) fwant = pll->vco_freq_min;
	if (fwant > fmax) fwant = fmax;
	for (; p-- > 0; fwant >>= 1, bestdiff >>= 1) {
		unsigned int m;

		if (fwant < pll->vco_freq_min) break;
		for (m = pll->in_div_min; m <= pll->in_div_max; m++) {
			unsigned int diff, fvco;
			unsigned int n;

			n = (fwant * (m + 1) + (fxtal >> 1)) / fxtal - 1;
			if (n > pll->feed_div_max)
				break;
			if (n < pll->feed_div_min)
				n = pll->feed_div_min;
			fvco = (fxtal * (n + 1)) / (m + 1);
			if (fvco < fwant)
				diff = fwant - fvco;
			else
				diff = fvco - fwant;
			if (diff < bestdiff) {
				bestdiff = diff;
				*post = p;
				*in = m;
				*feed = n;
				bestvco = fvco;
			}
		}
	}
	LOG(4,("clk: %x %x %x %d %d %d\n", *in, *feed, *post, fxtal, bestvco, fwant));
	return bestvco;
}

int matroxfb_PLL_mavenclock(const struct matrox_pll_features2* pll,
		const struct matrox_pll_ctl* ctl,
		unsigned int htotal, unsigned int vtotal,
		unsigned int* in, unsigned int* feed, unsigned int* post,
		unsigned int* h2) {
	unsigned int besth2 = 0;
	unsigned int fxtal = ctl->ref_freq;
	unsigned int fmin = pll->vco_freq_min / ctl->den;
	unsigned int fwant;
	unsigned int p;
	unsigned int scrlen;
	unsigned int fmax;

	LOG(4,("PLL_calcclock"));

	scrlen = htotal * (vtotal - 1);
	fwant = htotal * vtotal;
	fmax = pll->vco_freq_max / ctl->den;

	LOG(2, ("FVMAB:want: %x, xtal: %x, h: %x, v: %x, fmax: %x\n",
		fwant, fxtal, htotal, vtotal, fmax));
	for (p = 1; p <= pll->post_shift_max; p++) {
		if (fwant * 2 > fmax)
			break;
		fwant *= 2;
	}
	if (fwant > fmax)
		return 0;
	for (; p-- > 0; fwant >>= 1) {
		unsigned int m;

		if (fwant < fmin) break;
		for (m = pll->in_div_min; m <= pll->in_div_max; m++) {
			unsigned int n;
			unsigned int dvd;
			unsigned int ln;

			n = (fwant * m) / fxtal;
			if (n < pll->feed_div_min)
				continue;
			if (n > pll->feed_div_max)
				break;

			ln = fxtal * n;
			dvd = m << p;

			if (ln % dvd)
				continue;
			ln = ln / dvd;

			if (ln < scrlen + 2)
				continue;
			ln = ln - scrlen;
			if (ln > htotal)
				continue;
			LOG(2,("FBMAV:Match: %x / %x / %x / %x\n", n, m, p, ln));
			if (ln > besth2) {
				LOG(2, ("FBMAV:Better...\n"));
				*h2 = besth2 = ln;
				*post = p;
				*in = m;
				*feed = n;
			}
		}
	}
	if (besth2 < 2)
		return 0;
	LOG(4,("FBMAV:clk: %x %x %x %d %d\n", *in, *feed, *post, fxtal, fwant));
	return fxtal * (*feed) / (*in) * ctl->den;
}

unsigned int matroxfb_mavenclock(const struct matrox_pll_ctl* ctl,
		unsigned int htotal, unsigned int vtotal,
		unsigned int* in, unsigned int* feed, unsigned int* post,
		unsigned int* htotal2) {
	unsigned int fvco;
	unsigned int p;

	fvco = matroxfb_PLL_mavenclock(&maven1000_pll, ctl, htotal, vtotal, in, feed, &p, htotal2);
	if (!fvco)
		return -EINVAL;
	p = (1 << p) - 1;
	if (fvco <= 100000000)
		;
	else if (fvco <= 140000000)
		p |= 0x08;
	else if (fvco <= 180000000)
		p |= 0x10;
	else
		p |= 0x18;
	*post = p;
	return 0;
}

void DAC1064_calcclock(unsigned int freq, unsigned int fmax,
		unsigned int* in, unsigned int* feed, unsigned int* post) {
	unsigned int fvco;
	unsigned int p;

	fvco = matroxfb_PLL_calcclock(&maven_pll, freq, fmax, in, feed, &p);
	p = (1 << p) - 1;
	if (fvco <= 100000)
		;
	else if (fvco <= 140000)
		p |= 0x08;
	else if (fvco <= 180000)
		p |= 0x10;
	else
		p |= 0x18;
	*post = p;
	return;
}

void maven_init_TVdata
(
	struct mavenregs* data
) 
{
	static struct mavenregs palregs = { {
		0x2A, 0x09, 0x8A, 0xCB,	/* 00: chroma subcarrier */
		0x00,
		0x00,	/* ? not written */
		0x00,	/* modified by code (F9 written...) */
		0x00,	/* ? not written */
		0x7E,	/* 08 */
		0x44,	/* 09 */
		0x9C,	/* 0A */
		0x2E,	/* 0B */
		0x21,	/* 0C */
		0x00,	/* ? not written */
		0x3F, 0x03, /* 0E-0F */
		0x3F, 0x03, /* 10-11 */
		0x1A,	/* 12 */
		0x2A,	/* 13 */
		0x1C, 0x3D, 0x14, /* 14-16 */
		0x9C, 0x01, /* 17-18 */
		0x00,	/* 19 */
		0xFE,	/* 1A */
		0x7E,	/* 1B */
		0x60,	/* 1C */
		0x05,	/* 1D */
		0x89, 0x03, /* 1E-1F */
		0x72,	/* 20 */
		0x07,	/* 21 */
		0x72,	/* 22 */
		0x00,	/* 23 */
		0x00,	/* 24 */
		0x00,	/* 25 */
		0x08,	/* 26 */
		0x04,	/* 27 */
		0x00,	/* 28 */
		0x1A,	/* 29 */
		0x55, 0x01, /* 2A-2B */
		0x26,	/* 2C */
		0x07, 0x7E, /* 2D-2E */
		0x02, 0x54, /* 2F-30 */
		0xB0, 0x00, /* 31-32 */
		0x14,	/* 33 */
		0x49,	/* 34 */
		0x00,	/* 35 written multiple times */
		0x00,	/* 36 not written */
		0xA3,	/* 37 */
		0xC8,	/* 38 */
		0x22,	/* 39 */
		0x02,	/* 3A */
		0x22,	/* 3B */
		0x3F, 0x03, /* 3C-3D */
		0x00,	/* 3E written multiple times */
		0x00,	/* 3F not written */
	}, MODE_PAL, 625, 50 };
	static struct mavenregs ntscregs = { {
		0x21, 0xF0, 0x7C, 0x1F,	/* 00: chroma subcarrier */
		0x00,
		0x00,	/* ? not written */
		0x00,	/* modified by code (F9 written...) */
		0x00,	/* ? not written */
		0x7E,	/* 08 */
		0x43,	/* 09 */
		0x7E,	/* 0A */
		0x3D,	/* 0B */
		0x00,	/* 0C */
		0x00,	/* ? not written */
		0x41, 0x00, /* 0E-0F */
		0x3C, 0x00, /* 10-11 */
		0x17,	/* 12 */
		0x21,	/* 13 */
		0x1B, 0x1B, 0x24, /* 14-16 */
		0x83, 0x01, /* 17-18 */
		0x00,	/* 19 */
		0x0F,	/* 1A */
		0x0F,	/* 1B */
		0x60,	/* 1C */
		0x05,	/* 1D */
		0x89, 0x02, /* 1E-1F */
		0x5F,	/* 20 */
		0x04,	/* 21 */
		0x5F,	/* 22 */
		0x01,	/* 23 */
		0x02,	/* 24 */
		0x00,	/* 25 */
		0x0A,	/* 26 */
		0x05,	/* 27 */
		0x00,	/* 28 */
		0x10,	/* 29 */
		0xFF, 0x03, /* 2A-2B */
		0x24,	/* 2C */
		0x0F, 0x78, /* 2D-2E */
		0x00, 0x00, /* 2F-30 */
		0xB2, 0x04, /* 31-32 */
		0x14,	/* 33 */
		0x02,	/* 34 */
		0x00,	/* 35 written multiple times */
		0x00,	/* 36 not written */
		0xA3,	/* 37 */
		0xC8,	/* 38 */
		0x15,	/* 39 */
		0x05,	/* 3A */
		0x3B,	/* 3B */
		0x3C, 0x00, /* 3C-3D */
		0x00,	/* 3E written multiple times */
		0x00,	/* never written */
	}, MODE_NTSC, 525, 60 };

	if (mode & MODE_PAL)
		*data = palregs;
	else
		*data = ntscregs;

	data->regs[0x93] = 0xA2;

	/* gamma correction registers */
	data->regs[0x83] = 0x00;
	data->regs[0x84] = 0x00;
	data->regs[0x85] = 0x00;
	data->regs[0x86] = 0x1F;
	data->regs[0x87] = 0x10;
	data->regs[0x88] = 0x10;
	data->regs[0x89] = 0x10;
	data->regs[0x8A] = 0x64;	/* 100 */
	data->regs[0x8B] = 0xC8;	/* 200 */

	return;
}

#define LR(x) i2c_maven_write((x), m->regs[(x)])
#define LRP(x) MAVWWP((x), (m->regs[(x)]|m->regs[(x+1)]<<8))
void maven_init_TV
(
	const struct mavenregs* m
) 
{
	int val;


	i2c_maven_write( 0x3E, 0x01);
	i2c_maven_read( 0x82);	/* fetch oscillator state? */
	i2c_maven_write( 0x8C, 0x00);
	i2c_maven_read( 0x94);	/* get 0x82 */
	i2c_maven_write( 0x94, 0xA2);
	/* xmiscctrl */

	MAVWWP(0x8E, 0x1EFF); 
	i2c_maven_write( 0xC6, 0x01);

	/* removed code... */

	i2c_maven_read( 0x06);
	i2c_maven_write( 0x06, 0xF9);	/* or read |= 0xF0 ? */

	/* removed code here... */

	/* real code begins here? */
	/* chroma subcarrier */
	LR(0x00); LR(0x01); LR(0x02); LR(0x03);

	LR(0x04);

	LR(0x2C);
	LR(0x08);
	LR(0x0A);
	LR(0x09);
	LR(0x29);
	LRP(0x31);
	LRP(0x17);
	LR(0x0B);
	LR(0x0C);
	if (m->mode & MODE_PAL) {
		i2c_maven_write( 0x35, 0x10); /* ... */
	} else {
		i2c_maven_write( 0x35, 0x0F); /* ... */
	}

	LRP(0x10);

	LRP(0x0E);
	LRP(0x1E);

	LR(0x20);	/* saturation #1 */
	LR(0x22);	/* saturation #2 */
	LR(0x25);	/* hue */
	LR(0x34);
	LR(0x33);
	LR(0x19);
	LR(0x12);
	LR(0x3B);
	LR(0x13);
	LR(0x39);
	LR(0x1D);
	LR(0x3A);
	LR(0x24);
	LR(0x14);
	LR(0x15);
	LR(0x16);
	LRP(0x2D);
	LRP(0x2F);
	LR(0x1A);
	LR(0x1B);
	LR(0x1C);
	LR(0x23);
	LR(0x26);
	LR(0x28);
	LR(0x27);
	LR(0x21);
	LRP(0x2A);
	if (m->mode & MODE_PAL)
		i2c_maven_write( 0x35, 0x1D);	/* ... */
	else
		i2c_maven_write( 0x35, 0x1C);

	LRP(0x3C);
	LR(0x37);
	LR(0x38);
	i2c_maven_write( 0xB3, 0x01);

	i2c_maven_read( 0xB0);	/* read 0x80 */
	i2c_maven_write( 0xB0, 0x08);	/* ugh... */
	i2c_maven_read( 0xB9);	/* read 0x7C */
	i2c_maven_write( 0xB9, 0x78);
	i2c_maven_read( 0xBF);	/* read 0x00 */
	i2c_maven_write( 0xBF, 0x02);
	i2c_maven_read( 0x94);	/* read 0x82 */
	i2c_maven_write( 0x94, 0xB3);

	LR(0x80); /* 04 1A 91 or 05 21 91 */
	LR(0x81);
	LR(0x82);

	i2c_maven_write( 0x8C, 0x20);
	i2c_maven_read( 0x8D);
	i2c_maven_write( 0x8D, 0x10);

	LR(0x90); /* 4D 50 52 or 4E 05 45 */
	LR(0x91);
	LR(0x92);

	LRP(0x9A); /* 0049 or 004F */
	LRP(0x9C); /* 0004 or 0004 */
	LRP(0x9E); /* 0458 or 045E */
	LRP(0xA0); /* 05DA or 051B */
	LRP(0xA2); /* 00CC or 00CF */
	LRP(0xA4); /* 007D or 007F */
	LRP(0xA6); /* 007C or 007E */
	LRP(0xA8); /* 03CB or 03CE */
	LRP(0x98); /* 0000 or 0000 */
	LRP(0xAE); /* 0044 or 003A */
	LRP(0x96); /* 05DA or 051B */
	LRP(0xAA); /* 04BC or 046A */
	LRP(0xAC); /* 004D or 004E */

	LR(0xBE);
	LR(0xC2);

	i2c_maven_read( 0x8D);
	i2c_maven_write( 0x8D, 0x00);

	LR(0x20);	/* saturation #1 */
	LR(0x22);	/* saturation #2 */
	LR(0x93);	/* whoops */
	LR(0x20);	/* oh, saturation #1 again */
	LR(0x22);	/* oh, saturation #2 again */
	LR(0x25);	/* hue */
	LRP(0x0E);
	LRP(0x1E);
	LRP(0x0E);	/* problems with memory? */
	LRP(0x1E);	/* yes, matrox must have problems in memory area... */

	/* load gamma correction stuff */
	LR(0x83);
	LR(0x84);
	LR(0x85);
	LR(0x86);
	LR(0x87);
	LR(0x88);
	LR(0x89);
	LR(0x8A);
	LR(0x8B);

	val = i2c_maven_read( 0x8D);
	val &= 0x10;			/* 0x10 or anything ored with it */
	i2c_maven_write( 0x8D, val);

	LR(0x33);
	LR(0x19);
	LR(0x12);
	LR(0x3B);
	LR(0x13);
	LR(0x39);
	LR(0x1D);
	LR(0x3A);
	LR(0x24);
	LR(0x14);
	LR(0x15);
	LR(0x16);
	LRP(0x2D);
	LRP(0x2F);
	LR(0x1A);
	LR(0x1B);
	LR(0x1C);
	LR(0x23);
	LR(0x26);
	LR(0x28);
	LR(0x27);
	LR(0x21);
	LRP(0x2A);
	if (m->mode & MODE_PAL)
		i2c_maven_write( 0x35, 0x1D);
	else
		i2c_maven_write( 0x35, 0x1C);
	LRP(0x3C);
	LR(0x37);
	LR(0x38);

	i2c_maven_read( 0xB0);
	LR(0xB0);	/* output mode */
	LR(0x90);
	LR(0xBE);
	LR(0xC2);

	LRP(0x9A);
	LRP(0xA2);
	LRP(0x9E);
	LRP(0xA6);
	LRP(0xAA);
	LRP(0xAC);
	i2c_maven_write( 0x3E, 0x00);
	i2c_maven_write( 0x95, 0x20);
}

int maven_find_exact_clocks(unsigned int ht, unsigned int vt,
		struct mavenregs* m) {
	unsigned int x;
	unsigned int err = ~0;

	/* 1:1 */
	m->regs[0x80] = 0x0F;
	m->regs[0x81] = 0x07;
	m->regs[0x82] = 0x81;

	for (x = 0; x < 8; x++) {
		unsigned int a, b, c, h2;
		unsigned int h = ht + 2 + x;

		if (!matroxfb_mavenclock((m->mode & MODE_PAL) ? &maven_PAL : &maven_NTSC, h, vt, &a, &b, &c, &h2)) {
			unsigned int diff = h - h2;

			if (diff < err) {
				err = diff;
				m->regs[0x80] = a - 1;
				m->regs[0x81] = b - 1;
				m->regs[0x82] = c | 0x80;
				m->hcorr = h2 - 2;
				m->htotal = h - 2;
			}
		}
	}
	return err != ~0U;
}

//called from main access points...
inline int maven_compute_timing
(
		struct my_timming* mt,
		struct mavenregs* m
) 
{
	unsigned int tmpi;
	unsigned int a, bv, c;

	m->mode = mode;
	if (MODE_TV(mode)) {
		unsigned int lmargin;
		unsigned int umargin;
		unsigned int vslen;
		unsigned int hcrt;
		unsigned int slen;

		maven_init_TVdata(m);

		if (maven_find_exact_clocks(mt->HTotal, mt->VTotal, m) == 0)
			return -EINVAL;

		lmargin = mt->HTotal - mt->HSyncEnd;
		slen = mt->HSyncEnd - mt->HSyncStart;
		hcrt = mt->HTotal - slen - mt->delay;
		umargin = mt->VTotal - mt->VSyncEnd;
		vslen = mt->VSyncEnd - mt->VSyncStart;

		if (m->hcorr < mt->HTotal)
			hcrt += m->hcorr;
		if (hcrt > mt->HTotal)
			hcrt -= mt->HTotal;
		if (hcrt + 2 > mt->HTotal)
			hcrt = 0;	/* or issue warning? */

		/* last (first? middle?) line in picture can have different length */
		/* hlen - 2 */
		m->regs[0x96] = m->hcorr;
		m->regs[0x97] = m->hcorr >> 8;
		/* ... */
		m->regs[0x98] = 0x00; m->regs[0x99] = 0x00;
		/* hblanking end */
		m->regs[0x9A] = lmargin;	/* 100% */
		m->regs[0x9B] = lmargin >> 8;	/* 100% */
		/* who knows */
		m->regs[0x9C] = 0x04;
		m->regs[0x9D] = 0x00;
		/* htotal - 2 */
		m->regs[0xA0] = m->htotal;
		m->regs[0xA1] = m->htotal >> 8;
		/* vblanking end */
		m->regs[0xA2] = mt->VTotal - mt->VSyncStart - 1;	/* stop vblanking */
		m->regs[0xA3] = (mt->VTotal - mt->VSyncStart - 1) >> 8;
		/* something end... [A6]+1..[A8] */
		m->regs[0xA4] = 0x01;
		m->regs[0xA5] = 0x00;
		/* something start... 0..[A4]-1 */
		m->regs[0xA6] = 0x00;
		m->regs[0xA7] = 0x00;
		/* vertical line count - 1 */
		m->regs[0xA8] = mt->VTotal - 1;
		m->regs[0xA9] = (mt->VTotal - 1) >> 8;
		/* horizontal vidrst pos */
		m->regs[0xAA] = hcrt;		/* 0 <= hcrt <= htotal - 2 */
		m->regs[0xAB] = hcrt >> 8;
		/* vertical vidrst pos */
		m->regs[0xAC] = mt->VTotal - 2;
		m->regs[0xAD] = (mt->VTotal - 2) >> 8;
		/* moves picture up/down and so on... */
		m->regs[0xAE] = 0x01; /* Fix this... 0..VTotal */
		m->regs[0xAF] = 0x00;
		{
			int hdec;
			int hlen;
			unsigned int ibmin = 4 + lmargin + mt->HDisplay;
			unsigned int ib;
			int i;

			/* Verify! */
			/* Where 94208 came from? */
			if (mt->HTotal)
				hdec = 94208 / (mt->HTotal);
			else
				hdec = 0x81;
			if (hdec > 0x81)
				hdec = 0x81;
			if (hdec < 0x41)
				hdec = 0x41;
			hdec--;
			hlen = 98304 - 128 - ((lmargin + mt->HDisplay - 8) * hdec);
			if (hlen < 0)
				hlen = 0;
			hlen = hlen >> 8;
			if (hlen > 0xFF)
				hlen = 0xFF;
			/* Now we have to compute input buffer length.
			   If you want any picture, it must be between
			     4 + lmargin + xres
			   and
			     94208 / hdec
			   If you want perfect picture even on the top
			   of screen, it must be also
			     0x3C0000 * i / hdec + Q - R / hdec
			   where
			        R      Qmin   Qmax
			     0x07000   0x5AE  0x5BF
			     0x08000   0x5CF  0x5FF
			     0x0C000   0x653  0x67F
			     0x10000   0x6F8  0x6FF
			 */
			i = 1;
			do {
				ib = ((0x3C0000 * i - 0x8000)/ hdec + 0x05E7) >> 8;
				i++;
			} while (ib < ibmin);
			if (ib >= m->htotal + 2) {
				ib = ibmin;
			}

			m->regs[0x90] = hdec;	/* < 0x40 || > 0x80 is bad... 0x80 is questionable */
			m->regs[0xC2] = hlen;
			/* 'valid' input line length */
			m->regs[0x9E] = ib;
			m->regs[0x9F] = ib >> 8;
		}
		{
			int vdec;
			int vlen;

			if (mt->VTotal) {
				double f1;
				uint32 a;
				uint32 b;

				// Unsure of how to do 64-bit integer maths on Be, so use FPU!
				a = m->vlines * (m->htotal + 2);
				b = (mt->VTotal - 1) * (m->htotal + 2) + m->hcorr + 2;
				f1 = (double)a * (double)32768; 
				f1 /= (double) b;
				vdec = (uint32) f1;
			} else
				vdec = 0x8000;
			if (vdec > 0x8000)
				vdec = 0x8000;
			vlen = (vslen + umargin + mt->VDisplay) * vdec;
			vlen = (vlen >> 16) - 146; /* FIXME: 146?! */
			if (vlen < 0)
				vlen = 0;
			if (vlen > 0xFF)
				vlen = 0xFF;
			vdec--;
			m->regs[0x91] = vdec;
			m->regs[0x92] = vdec >> 8;
			m->regs[0xBE] = vlen;
		}
		m->regs[0xB0] = 0x08;	/* output: SVideo/Composite */
		return 0;
	}

	DAC1064_calcclock(mt->pixclock, 450000, &a, &bv, &c);
	m->regs[0x80] = a;
	m->regs[0x81] = bv;
	m->regs[0x82] = c | 0x80;

	m->regs[0xB3] = 0x01;
	m->regs[0x94] = 0xB2;

	/* htotal... */
	m->regs[0x96] = mt->HTotal;
	m->regs[0x97] = mt->HTotal >> 8;
	/* ?? */
	m->regs[0x98] = 0x00;
	m->regs[0x99] = 0x00;
	/* hsync len */
	tmpi = mt->HSyncEnd - mt->HSyncStart;
	m->regs[0x9A] = tmpi;
	m->regs[0x9B] = tmpi >> 8;
	/* hblank end */
	tmpi = mt->HTotal - mt->HSyncStart;
	m->regs[0x9C] = tmpi;
	m->regs[0x9D] = tmpi >> 8;
	/* hblank start */
	tmpi += mt->HDisplay;
	m->regs[0x9E] = tmpi;
	m->regs[0x9F] = tmpi >> 8;
	/* htotal + 1 */
	tmpi = mt->HTotal + 1;
	m->regs[0xA0] = tmpi;
	m->regs[0xA1] = tmpi >> 8;
	/* vsync?! */
	tmpi = mt->VSyncEnd - mt->VSyncStart - 1;
	m->regs[0xA2] = tmpi;
	m->regs[0xA3] = tmpi >> 8;
	/* ignored? */
	tmpi = mt->VTotal - mt->VSyncStart;
	m->regs[0xA4] = tmpi;
	m->regs[0xA5] = tmpi >> 8;
	/* ignored? */
	tmpi = mt->VTotal - 1;
	m->regs[0xA6] = tmpi;
	m->regs[0xA7] = tmpi >> 8;
	/* vtotal - 1 */
	m->regs[0xA8] = tmpi;
	m->regs[0xA9] = tmpi >> 8;
	/* hor vidrst */
	tmpi = mt->HTotal - mt->delay;
	m->regs[0xAA] = tmpi;
	m->regs[0xAB] = tmpi >> 8;
	/* vert vidrst */
	tmpi = mt->VTotal - 2;
	m->regs[0xAC] = tmpi;
	m->regs[0xAD] = tmpi >> 8;
	/* ignored? */
	m->regs[0xAE] = 0x00;
	m->regs[0xAF] = 0x00;

	m->regs[0xB0] = 0x03;	/* output: monitor */
	m->regs[0xB1] = 0xA0;	/* ??? */
	m->regs[0x8C] = 0x20;	/* must be set... */
	m->regs[0x8D] = 0x00;	/* defaults to 0x10: test signal */
	m->regs[0xB9] = 0x1A;	/* defaults to 0x2C: too bright */
	m->regs[0xBF] = 0x22;	/* makes picture stable */

	return 0;
}

inline int maven_program_timing
(
	const struct mavenregs* m
) 
{
	if (m->mode & MODE_MONITOR) {
		LR(0x80);
		LR(0x81);
		LR(0x82);

		LR(0xB3);
		LR(0x94);

		LRP(0x96);
		LRP(0x98);
		LRP(0x9A);
		LRP(0x9C);
		LRP(0x9E);
		LRP(0xA0);
		LRP(0xA2);
		LRP(0xA4);
		LRP(0xA6);
		LRP(0xA8);
		LRP(0xAA);
		LRP(0xAC);
		LRP(0xAE);

		LR(0xB0);	/* output: monitor */
		LR(0xB1);	/* ??? */
		LR(0x8C);	/* must be set... */
		LR(0x8D);	/* defaults to 0x10: test signal */
		LR(0xB9);	/* defaults to 0x2C: too bright */
		LR(0xBF);	/* makes picture stable */
	} else {
		maven_init_TV(m);
	}
	return 0;
}

inline int maven_resync() 
{
	i2c_maven_write( 0x95, 0x20);	/* start whole thing */
	return 0;
}

/******************************************************/

///////////////////////////////////////////////////////////////////////////////////////
//Main entry points... - sequence is compute, program, start
int maven_set_mode(int mod)
{
	switch (mode)
	{
	case MODE_NTSC:
	case MODE_PAL:
	case MODE_MONITOR:
		mode = mod;
		break;
	default:
		return -1;
	}
	return 0;
}

int maven_out_compute(struct my_timming* mt, struct mavenregs* maven) 
{
	return maven_compute_timing(mt, maven);
}

int maven_out_program(const struct mavenregs* maven) 
{
	return maven_program_timing(maven);
}

int maven_out_start() 
{
	return maven_resync();
}

/* ************************** */
//Used with Petr's permission (many thanks)
//Any bugs with this code contact me (MarkWatson@bcs.org.uk) not Petr
//-----------------------------------------------
//MODULE_AUTHOR("(c) 1999,2000 Petr Vandrovec <vandrove@vc.cvut.cz>");
//MODULE_DESCRIPTION("Matrox G200/G400 Matrox MGA-TVO driver");
