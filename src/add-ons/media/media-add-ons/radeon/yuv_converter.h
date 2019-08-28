/*
	Copyright (c) 2004, Thomas Kurschel
	

	Part of Radeon In add-on
		
	YUV converter
	
	The Rage Theatre always provides YUV422 data and starting with Radeon, ATI
	moved colour converter from 2D to 3D unit, so if you need another format you
	must convert it manually, unless you get 3D working (which is, starting with r300,
	hopeless anyway as there is no spec). 
	
	Colour temperature is according to BT.601; for YCbCr format see also GraphicsDefs.h
	
	This header is included from VideoIn.cpp, with various defines to convert to
	the wished format (RGB15, RGB16 or RGB32).
	
	Things to improve:
	- colour components should be interpolated for odd pixels
*/

	static const uint8 c_offs[8] =
		{ 128, 128, 128, 128, 128, 128, 128, 128 };
		
	static const int16 y_offs[4] =
		{ 16*128, 16*128, 16*128, 16*128 };

	static const uint16 masks[2][4] = {
		// high byte mask
		{ 0xff00, 0xff00, 0xff00, 0xff00 },
		// low byte mask
		{ 0x00ff, 0x00ff, 0x00ff, 0x00ff },
	};
	
	static const int16 scale[5][4] = {
		// Y pre-scale
		{ (int16)(1.1678 * 512), (int16)(1.1678 * 512), (int16)(1.1678 * 512), (int16)(1.1678 * 512) },
		// CbG CrG CbG CrG
		{ (int16)(-0.3929 * 256), (int16)(-0.8154 * 256), (int16)(-0.3929 * 256), (int16)(-0.8154 * 256) },
		// CbB CrR CbB CrR
		{ (int16)(2.0232 * 256), (int16)(1.6007 * 256), (int16)(2.0232 * 256), (int16)(1.6007 * 256) },
	};

	static const int8 masks_8bit[2][8] = {	
		// r/b 16 bit mask and r/g/b 15 bit mask
		{ 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8, 0xf8 },
		// g 16 bit mask
		{ 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc, 0xfc },
	};

	asm volatile(
	"2:\n"
		"pxor		%%mm7,%%mm7\n"
	
	"1:\n"
		"movq		(%0),%%mm0\n"		// mm0 = Cr2'Y3' Cb2'Y2' Cr0'Y1' Cb0'Y0'
		"movq		%%mm0,%%mm1\n"		// mm1 = Cr2'Y3' Cb2'Y2' Cr0'Y1' Cb0'Y0'
		
		// Y is in 16..235
		// we need to substract 16 and scale to full range
		// as standard MMX has a _signed_ integer multiply only, the highest bit must 
		// be zero before scaling, i.e. we use signed format 9.7
		"pand		8+%4,%%mm0\n"		// mm2 =     Y3'     Y2'     Y1'     Y0'
		"psllw		$7,%%mm0\n"
		"psubusw	%3,%%mm0\n"
		"pmulhw		%5,%%mm0\n"			// mm0 =      Y3      Y2      Y1      Y0

		// Cb and Cr is biased; compensate that		
		"psubb		%2,%%mm1\n"			// mm1 = Cr2 xxx Cb2 xxx Cr0 xxx Cb0 xxx
		"pand		%4,%%mm1\n"			// mm1 = Cr2     Cb2     Cr0     Cb0
		
		// transform Cb and Cr to green component
		"movq		%%mm1,%%mm2\n"
		"pmaddwd	8+%5,%%mm1\n"		// mm1 =  CbCrG2 xxxxxxx  CbCrG0 xxxxxxx
		"psrad		$16,%%mm1\n"		// mm1 =          CbCrG2          CbCrG0
		"packssdw	%%mm1,%%mm1\n"		// mm1 =  CbCrG2  CbCrG0  CbCrG2  CbCrG0
		"punpcklwd	%%mm1,%%mm1\n"		// mm1 =  CbCrG2  CbCrG2  CbCrG0  CbCrG0
		
		// transform Cb to blue and Cr to red component
		"pmulhw		16+%5,%%mm2\n"		// mm2 =    CrR2    CbB2    CrR0    CbB0
		
		// nasty shuffling to separate and duplicate components
		"movq		%%mm2,%%mm3\n"
		"punpcklwd	%%mm3,%%mm3\n"		// mm3 =    CrR0    CrR0    CbB0    CbB0
		"punpckhwd	%%mm2,%%mm2\n"		// mm2 =    CrR2    CrR2    CbB2    CbB2
		
		"movq		%%mm3,%%mm4\n"
		"punpckldq	%%mm2,%%mm3\n"		// mm3 =    CbB2    CbB2    CbB0    CbB0
		"punpckhdq	%%mm2,%%mm4\n"		// mm4 =    CrR2    CrR2    CrR0    CrR0
		
		// add Y to get final RGB			
		"paddsw		%%mm0,%%mm1\n"		// mm1 =      G3      G2      G1      G0
		"paddsw		%%mm0,%%mm3\n"		// mm3 =      B3      B2      B1      B0
		"paddsw		%%mm0,%%mm4\n"		// mm4 =      R3      R2      R1      R0

		// now, RBG can be converted to 8 bits each
		"packuswb	%%mm0,%%mm1\n"		// mm1 =  Y3  Y2  Y1  Y0  G3  G2  G1  G0
		"packuswb	%%mm4,%%mm3\n"		// mm3 =  R3  R2  R1  R0  B3  B2  B1  B0

#ifdef RGB32
		// convertion to RGB32
		"movq		%%mm3,%%mm2\n"
		"punpckhbw	%%mm1,%%mm3\n"		// mm3 =  Y3  R3  Y2  R2  Y1  R1  Y0  R0
		"punpcklbw	%%mm1,%%mm2\n"		// mm2 =  G3  B3  G2  B2  G1  B1  G0  B0
		
		"movq		%%mm2,%%mm1\n"
		"punpcklwd	%%mm3,%%mm2\n"
		"movq		%%mm2,0x00(%1)\n"	// dst =  Y1  R1  G1  B1  Y0  R0  G0  B0

		"punpckhwd	%%mm3,%%mm1\n"
		"movq		%%mm1,0x08(%1)\n"	// dst =  Y3  R3  G3  B3  Y2  R2  G2  B2
		
		"addl		$0x08,%0\n"			// source += 8
		"addl		$0x10,%1\n"			// destination += 16
		"subl		$0x10,%7\n"			// next pixels
#endif

#ifdef RGB16
		// convertion to RGB16
		// things would be much easier if Intel had added a RGB32->RGB16 instruction
		"pand		%6,%%mm3\n"			//  mm3 -  R3  R2  R1  R0  B3  B2  B1  B0 (masked)
		"pand		8+%6,%%mm1\n"		//  mm1 -  Y3  Y2  Y1  Y0  G3  G2  G1  G0 (masked)
		
		"punpcklbw	%%mm7,%%mm1\n"		//  mm1 -      G3      G2      G1      G0
		"movq		%%mm7,%%mm2\n"
		"punpckhbw 	%%mm3,%%mm2\n"		//  mm2 -  R3      R2      R1      R0
		"punpcklbw 	%%mm7,%%mm3\n"		//  mm3 -      B3      B2      B1      B0
		
		"psllw		$3,%%mm1\n"			//  mm1 -    G3      G2      G1      G0
		"psrlw		$3,%%mm3\n"			//  mm3 -      B3      B2      B1      B0
		
		"por		%%mm2,%%mm1\n"
		"por		%%mm3,%%mm1\n"
		"movq		%%mm1,(%1)\n"

		"addl		$0x08,%0\n"			// source += 8
		"addl		$0x08,%1\n"			// destination += 8
		"subl		$0x08,%7\n"			// next pixels
#endif

#ifdef RGB15
		// convertion to RGB15
		// same problem as before
		"pand		%6,%%mm3\n"			//  mm3 -  R3  R2  R1  R0  B3  B2  B1  B0 (masked)
		"pand		%6,%%mm1\n"			//  mm1 -  Y3  Y2  Y1  Y0  G3  G2  G1  G0 (masked)
		
		"punpcklbw	%%mm7,%%mm1\n"		//  mm1 -      G3      G2      G1      G0
		"movq		%%mm7,%%mm2\n"
		"punpckhbw 	%%mm3,%%mm2\n"		//  mm2 -  R3      R2      R1      R0
		"punpcklbw 	%%mm7,%%mm3\n"		//  mm3 -      B3      B2      B1      B0
		
		"psllw		$2,%%mm1\n"			//  mm1 -    G3      G2      G1      G0
		"psrlw		$1,%%mm2\n"			//  mm2 -  R3      R2      R1      R0
		"psrlw		$3,%%mm3\n"			//  mm3 -      B3      B2      B1      B0
		
		"por		%%mm2,%%mm1\n"
		"por		%%mm3,%%mm1\n"
		"movq		%%mm1,(%1)\n"

		"addl		$0x08,%0\n"			// source += 8
		"addl		$0x08,%1\n"			// destination += 8
		"subl		$0x08,%7\n"			// next pixels
#endif

		// next
		"jg			1b\n"
		
		"movl		%9,%7\n"
		"subl		%7,%8\n"
		
		"jg			2b\n"
		"emms\n"
		:
		: "a" (convert_buffer), "d" (bits), 
		  "g" (c_offs), "g" (y_offs), "g" (masks), "g" (scale), "g" (masks_8bit),
		  "c" (bytesPerRow), "S" (bitsLength), "D" (bytesPerRow));
