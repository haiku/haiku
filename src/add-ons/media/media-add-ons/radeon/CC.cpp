/******************************************************************************
/
/	File:			CC.cpp
/
/	Description:	Closed caption module.
/
/	Copyright 2001, Carlos Hasan
/
*******************************************************************************/

#include <stdio.h>
#include <Debug.h>
#include "CC.h"

CCaption::CCaption(CCapture & capture)
	:	fCapture(capture),
		fChannel(C_RADEON_CC1),
		fRow(0),
		fColumn(0),
		fColor(0),
		fLastControlCode(0)
{
	for (int row = 0; row < C_RADEON_CC_ROWS; row++) {
		for (int column = 0; column < C_RADEON_CC_COLUMNS; column++) {
			fText[row][column] = fDisplayText[row][column] = 0x0000;
		}
	}
}

CCaption::~CCaption()
{
}

status_t CCaption::InitCheck() const
{
	return fCapture.InitCheck();
}

void CCaption::SetMode(cc_mode mode)
{
	fChannel = mode;
	fLastControlCode = 0;
}

bool CCaption::DecodeBits(const unsigned char * buffer, int & data)
{
	// assume the buffer is long enough (at least VBI line width bytes)
	if (buffer == NULL)
		return false;

	// compute low/high levels
	int low = 0xff, high = 0x00;
	for (int offset = C_RADEON_CC_BLANK_START; offset < C_RADEON_CC_BLANK_START + 4 * C_RADEON_CC_BIT_DURATION; offset++) {
		if (low > buffer[offset])
			low = buffer[offset];
		if (high < buffer[offset])
			high = buffer[offset];
	}
	if (low + C_RADEON_CC_LEVEL_THRESHOLD >= high)
		return false;

	const int middle = (low + high) >> 1;
	
	// find position of the first pulse
	int start = C_RADEON_CC_BLANK_START + C_RADEON_CC_BIT_DURATION;
	while (start <= C_RADEON_CC_BLANK_START + 4 * C_RADEON_CC_BIT_DURATION) {
		if (buffer[start + 0] < middle && buffer[start + 1] < middle &&
			buffer[start + 3] > middle && buffer[start + 4] > middle)
			break;
		start++;
	}
	if (start >= C_RADEON_CC_BLANK_START + 4 * C_RADEON_CC_BIT_DURATION)
		return false;

	// compute position of the last pulse
	int end = start + 17 * C_RADEON_CC_BIT_DURATION;

	// start in middle of first pulse
	int bit = 0;
	bool one = true;
	for (int offset = start + C_RADEON_CC_BIT_DURATION / 2; offset < end; offset++) {
		int width = 1;
		
		// search the next pulse front
		while (offset < end) {
			if (one) {
				if (buffer[offset + 0] > middle && buffer[offset + 1] > middle &&
					buffer[offset + 3] < middle && buffer[offset + 4] < middle)
					break;
			}
			else {
				if (buffer[offset + 0] < middle && buffer[offset + 1] < middle &&
					buffer[offset + 3] > middle && buffer[offset + 4] > middle)
					break;
			}
			offset++;
			width++;
		}

		// compute pulse width in bits
		const int nbits = (width + (bit == 0 ? 0 : bit == 15 ? C_RADEON_CC_BIT_DURATION :
			C_RADEON_CC_BIT_DURATION / 2)) / C_RADEON_CC_BIT_DURATION;
		data >>= nbits;
		if (one)
			data |= (0xffff << (16 - nbits)) & 0xffff;
		
		if ((bit += nbits) >= C_RADEON_CC_BITS_PER_FIELD)
			break;
		
		one = !one;
	}
	
	if (bit != C_RADEON_CC_BITS_PER_FIELD)
		return false;

	return true;
}

bool CCaption::Decode(const unsigned char * buffer0,
							const unsigned char * buffer1)
{
	enum caption_code {
		/* channel */
		C_CC1		= 0x0000,
		C_CC2		= 0x0800,
		
		/* address */
		C_ROW_1		= 0x0100,
		C_ROW_2		= 0x0120,
		C_ROW_3		= 0x0200,
		C_ROW_4		= 0x0220,
		C_ROW_5		= 0x0500,
		C_ROW_6		= 0x0520,
		C_ROW_7		= 0x0600,
		C_ROW_8		= 0x0620,
		C_ROW_9		= 0x0700,
		C_ROW_10	= 0x0720,
		C_ROW_11	= 0x0000,
		C_ROW_12	= 0x0300,
		C_ROW_13	= 0x0320,
		C_ROW_14	= 0x0400,
		C_ROW_15	= 0x0420,
				
		/* color */
		C_WHITE		= 0x0000,
		C_GREEN		= 0x0002,
		C_BLUE		= 0x0004,
		C_CYAN		= 0x0006,
		C_RED		= 0x0008,
		C_YELLOW	= 0x000a,
		C_MAGENTA	= 0x000c,
		C_ITALICS	= 0x000e,
		
		/* underline */
		C_NORMAL	= 0x0000,
		C_UNDERLINE	= 0x0001,
		
		/* control */
		C_RCL		= 0x0000,	// resume caption loading
		C_BS		= 0x0001,	// backspace
		C_AOF		= 0x0002,	// alarm off
		C_AON		= 0x0003,	// alarm on
		C_DER		= 0x0004,	// delete to end of row
		C_RU2		= 0x0005,	// roll-up captions (2 rows)
		C_RU3		= 0x0006,	// roll-up captions (3 rows)
		C_RU4		= 0x0007,	// roll-up captions (4 rows)
		C_FON		= 0x0008,	// flash on
		C_RDC		= 0x0009,	// resume direct captioning
		C_TR		= 0x000a,	// text restart
		C_RTD		= 0x000b,	// resume text display
		C_EDM		= 0x000c,	// erase displayed memory
		C_CR		= 0x000d,	// carriage return
		C_ENM		= 0x000e,	// erase non-displayed memory
		C_EOC		= 0x000f,	// end of caption
		
		/* special character */
		C_reg		= 0x0000,	// registered
		C_deg		= 0x0001,	// degree
		C_frac12	= 0x0002,	// fraction 1/2
		C_iquest	= 0x0003,	// invert question
		C_trademark	= 0x0004,	// trademark
		C_cent		= 0x0005,	// cent
		C_pound		= 0x0006,	// pound
		C_music		= 0x0007,	// music note
		C_agrave	= 0x0008,	// agrave
		C_tspace	= 0x0009,	// transparent space
		C_egrave	= 0x000a,	// egrave
		C_acirc		= 0x000b,	// a circ
		C_ecirc		= 0x000c,	// e circ
		C_icirc		= 0x000d,	// i circ
		C_ocirc		= 0x000e,	// o circ
		C_ucirc		= 0x000f,	// u circ
		
		/* standard character (ASCII) */
		C_aacute	= 0x002a,	// a acute accent
		C_eacute	= 0x005c,	// e acute accent
		C_iacute	= 0x005e,	// i acute accent
		C_oacute	= 0x005f,	// o acute accent
		C_uacute	= 0x0060,	// u acute accent
		C_ccedil	= 0x007b,	// c cedilla
		C_division	= 0x007c,	// division sign
		C_Ntilde	= 0x007d,	// N tilde
		C_ntilde	= 0x007e,	// n tilde
		C_block		= 0x007f	// solid block
	};
	
	int code, channel, character;
	
	/* decode next pair of bytes from the odd or even field buffer */
	if (!DecodeBits(fChannel == C_RADEON_CC1 || fChannel == C_RADEON_CC2 ? buffer0 : buffer1, code))
		return false;
	
	/* swap decoded bytes */
	code = ((code << 8) & 0xff00) | ((code >> 8) & 0x00ff);

	/* check parity */
	for (int bit = 0; bit < 7; bit++) {
		if ((code & (0x0001 << bit)) != 0)
			code ^= 0x0080;
		if ((code & (0x0100 << bit)) != 0)
			code ^= 0x8000;
	}
	if ((code & 0x8080) != 0x8080) {
		PRINT(("CCaption::Decode() - parity error (%04X)\n", code));
		return false;
	}
		
	/* check channel number */
	channel = (code & 0x0800) >> 12;
	if (channel == 0) {
		if (fChannel != C_RADEON_CC1 && fChannel != C_RADEON_CC3) {
			PRINT(("CCaption::Decode() - ignore channel (%02X)\n", code & 0x7f7f));
			return false;
		}
	}
	else {
		if (fChannel != C_RADEON_CC2 && fChannel != C_RADEON_CC4) {
			PRINT(("CCaption::Decode() - ignore channel (%02X)\n", code & 0x7f7f));
			return false;
		}
	}

	if ((code & 0x7000) == 0x0000) {
		/* one-byte standard character (0?XX) */
		character = code & 0x007f;
		
		if (character >= 0x20) {
		
			PRINT(("%c", character));
			
			fText[fRow][fColumn] = (fColor << 8) + character;
			if (fColumn < C_RADEON_CC_COLUMNS - 1)
				fColumn++;
		}
		else if (character != 0x00) {
			PRINT(("<%04X>", code & 0x7f7f));
		}
	}
	else if ((code & 0x7770) == 0x1120) {
		/* middle row code (112X) */
		fColor = code & 0x000f;
		
		fText[fRow][fColumn] = (fColor << 8) + ' ';
		if (fColumn < C_RADEON_CC_COLUMNS - 1)
			fColumn++;
		
		switch (fColor & 0x000e) {
		case C_WHITE:
			PRINT(("<white"));
			break;
		case C_GREEN:
			PRINT(("<green"));
			break;
		case C_BLUE:
			PRINT(("<blue"));
			break;
		case C_CYAN:
			PRINT(("<cyan"));
			break;
		case C_RED:
			PRINT(("<red"));
			break;
		case C_YELLOW:
			PRINT(("<yellow"));
			break;
		case C_MAGENTA:
			PRINT(("<magenta"));
			break;
		case C_ITALICS:
			PRINT(("<italics"));
			break;
		}
		if ((fColor & 0x0001) != 0)
			PRINT((",underline>"));
		else
			PRINT((">"));
	}
	else if ((code & 0x7770) == 0x1130) {
		/* two-byte special character (113X) */
		character = (code & 0x000f);

		fText[fRow][fColumn] = (fColor << 8) + character + 0x0080;
		if (fColumn < C_RADEON_CC_COLUMNS - 1)
			fColumn++;
		
		switch (character) {
		case C_reg:
			PRINT(("&reg;"));
			break;
		case C_deg:
			PRINT(("&deg;"));
			break;
		case C_frac12:
			PRINT(("&frac12;"));
			break;
		case C_iquest:
			PRINT(("&iquest;"));
			break;
		case C_trademark:
			PRINT(("&trademark;"));
			break;
		case C_cent:
			PRINT(("&cent;"));
			break;
		case C_pound:
			PRINT(("&pound;"));
			break;
		case C_music:
			PRINT(("&music;"));
			break;
		case C_agrave:
			PRINT(("&agrave;"));
			break;
		case C_tspace:
			PRINT(("&tspace;"));
			break;
		case C_egrave:
			PRINT(("&egrave;"));
			break;
		case C_acirc:
			PRINT(("&acirc;"));
			break;
		case C_ecirc:
			PRINT(("&ecirc;"));
			break;
		case C_icirc:
			PRINT(("&icirc;"));
			break;
		case C_ocirc:
			PRINT(("&ocirc;"));
			break;
		case C_ucirc:
			PRINT(("&ucirc;"));
			break;
		default:
			PRINT(("<special=%04X>", code & 0x7f7f));
			break;
		}
	}
	else if ((code & 0x7770) == 0x1420) {
	
		if (code == fLastControlCode)
			return false;
		fLastControlCode = code;
		
		/* miscellaneous control code (142X) */
		switch (code & 0x000f) {
		case C_RCL:
			// resume caption loading
			PRINT(("<rcl>"));
			break;
			
		case C_BS:
			// backspace
			PRINT(("<bs>"));
			if (fColumn > 0)
				fText[fRow][--fColumn] = 0x0000;
			break;
			
		case C_AOF:
			// alarm off
			PRINT(("<aof>"));
			break;
			
		case C_AON:
			// alarm on
			PRINT(("<aon>"));
			break;
			
		case C_DER:
			// delete to end of row
			PRINT(("<der>"));
			for (int column = fColumn; column < C_RADEON_CC_COLUMNS; column++)
				fText[fRow][column] = 0x0000;
			break;
			
		case C_RU2:
			// rollup captions (2 rows)
			PRINT(("<ru2>"));
			for (int row = 0; row < C_RADEON_CC_ROWS - 2; row++) {
				for (int column = 0; column < C_RADEON_CC_COLUMNS; column++)
					fText[row][column] = fText[row + 2][column];
			}
			for (int row = C_RADEON_CC_ROWS - 2; row < C_RADEON_CC_ROWS; row++) {
				for (int column = 0; column < C_RADEON_CC_COLUMNS; column++)
					fText[row][column] = 0x0000;
			}
			break;

		case C_RU3:
			// rollup captions (3 rows)
			PRINT(("<ru3>"));
			for (int row = 0; row < C_RADEON_CC_ROWS - 3; row++) {
				for (int column = 0; column < C_RADEON_CC_COLUMNS; column++)
					fText[row][column] = fText[row + 2][column];
			}
			for (int row = C_RADEON_CC_ROWS - 3; row < C_RADEON_CC_ROWS; row++) {
				for (int column = 0; column < C_RADEON_CC_COLUMNS; column++)
					fText[row][column] = 0x0000;
			}
			break;
			
		case C_RU4:
			// rollup captions (4 rows)
			PRINT(("<ru4>"));
			for (int row = 0; row < C_RADEON_CC_ROWS - 4; row++) {
				for (int column = 0; column < C_RADEON_CC_COLUMNS; column++)
					fText[row][column] = fText[row + 2][column];
			}
			for (int row = C_RADEON_CC_ROWS - 4; row < C_RADEON_CC_ROWS; row++) {
				for (int column = 0; column < C_RADEON_CC_COLUMNS; column++)
					fText[row][column] = 0x0000;
			}
			break;

		case C_FON:
			// flash on
			PRINT(("<fon>"));
			break;
			
		case C_RDC:
			// resume direct captioning
			PRINT(("<rdc>"));
			break;
			
		case C_TR:
			// text restart
			PRINT(("<tr>"));
			
			fRow = fColumn = 0;
			break;
		
		case C_RTD:
			// resume text display
			PRINT(("<rtd>"));

			fRow = fColumn = 0;
			break;
			
		case C_EDM:
			// erase displayed memory
			PRINT(("<edm>\n"));
			for (int row = 0; row < C_RADEON_CC_ROWS; row++) {
				for (int column = 0; column < C_RADEON_CC_COLUMNS; column++)
					fDisplayText[row][column] = 0x0000;
			}
			DisplayCaptions();
			break;
			
		case C_CR:
			// carriage return
			PRINT(("<cr>"));
			
			/* has no effect in caption loading mode */
			break;
		
		case C_ENM:
			// erase non-displayed memory
			PRINT(("<enm>"));
			for (int row = 0; row < C_RADEON_CC_ROWS; row++) {
				for (int column = 0; column < C_RADEON_CC_COLUMNS; column++)
					fText[row][column] = 0x0000;
			}
			break;
			
		case C_EOC:
			// end of caption
			PRINT(("<eoc>\n"));
			for (int row = 0; row < C_RADEON_CC_ROWS; row++) {
				for (int column = 0; column < C_RADEON_CC_COLUMNS; column++) {
					const int code = fText[row][column];
					fText[row][column] = fDisplayText[row][column];
					fDisplayText[row][column] = code;
				}
			}
			
			DisplayCaptions();
			break;		
		}
	}
	else if ((code & 0x7770) == 0x1720) {
		/* tab offset (172X) */
		const int offset = code & 0x000f;

		if (offset >= 1 && offset <= 3) {
			PRINT(("<tab%d>", offset));
			
			if ((fColumn += offset) >= C_RADEON_CC_COLUMNS - 1)
				fColumn = C_RADEON_CC_COLUMNS - 1;
		}
		else {
			PRINT(("<tab=%04X>", code & 0x7f7f));
		}
	}
	else if ((code & 0x7040) == 0x1040) {
		/* preamble address code (1Y4X, 1Y6X) */
	
		switch (code & 0x0720) {
		case C_ROW_1:
			PRINT(("\n<row1"));
			fRow = 0;
			break;
		case C_ROW_2:
			PRINT(("\n<row2"));
			fRow = 1;
			break;
		case C_ROW_3:
			PRINT(("\n<row3"));
			fRow = 2;
			break;
		case C_ROW_4:
			PRINT(("\n<row4"));
			fRow = 3;
			break;
		case C_ROW_5:
			PRINT(("\n<row5"));
			fRow = 4;
			break;
		case C_ROW_6:
			PRINT(("\n<row6"));
			fRow = 5;
			break;
		case C_ROW_7:
			PRINT(("\n<row7"));
			fRow = 6;
			break;
		case C_ROW_8:
			PRINT(("\n<row8"));
			fRow = 7;
			break;
		case C_ROW_9:
			PRINT(("\n<row9"));
			fRow = 8;
			break;
		case C_ROW_10:
			PRINT(("\n<row10"));
			fRow = 9;
			break;
		case C_ROW_11:
			PRINT(("\n<row11"));
			fRow = 10;
			break;
		case C_ROW_12:
			PRINT(("\n<row12"));
			fRow = 11;
			break;
		case C_ROW_13:
			PRINT(("\n<row13"));
			fRow = 12;
			break;
		case C_ROW_14:
			PRINT(("\n<row14"));
			fRow = 13;
			break;
		case C_ROW_15:
			PRINT(("\n<row15"));
			fRow = 14;
			break;
		default:
			PRINT(("\n<pac=%04X>", code & 0x7f7f));
			return false;
		}
		
		if ((code & 0x0010) == 0x0000) {
			/* change color */
			fColor = (code & 0x000f);
			fColumn = 0;
			
			switch (fColor & 0x000e) {
			case C_WHITE:
				PRINT((",white"));
				break;
			case C_GREEN:
				PRINT((",green"));
				break;
			case C_BLUE:
				PRINT((",blue"));
				break;
			case C_CYAN:
				PRINT((",cyan"));
				break;
			case C_RED:
				PRINT((",red"));
				break;
			case C_YELLOW:
				PRINT((",yellow"));
				break;
			case C_MAGENTA:
				PRINT((",magenta"));
				break;
			case C_ITALICS:
				PRINT((",italics"));
				break;
			}
			if ((fColor & 0x0001) != 0)
				PRINT((",underline>"));
			else
				PRINT((">"));
			
		}
		else {
			/* indent, white */
			fColor = C_WHITE | (code & 0x0001);
			fColumn = (code & 0x000e) << 1;
			PRINT((",col%d>", fColumn));
		}
	}
	else {
		/* two one-byte standard characters */
		character = (code >> 8) & 0x7f;
		if (character >= 0x20) {
		
			PRINT(("%c", character));
			
			fText[fRow][fColumn] = (fColor << 8) + character;
			if (fColumn < C_RADEON_CC_COLUMNS - 1)
				fColumn++;
		}
		else if (character != 0x00) {
			PRINT(("<%02X>", character));
		}
		
		character = (code >> 0) & 0x7f;
		if (character >= 0x20) {

			PRINT(("%c", character));

			fText[fRow][fColumn] = (fColor << 8) + character;
			if (fColumn < C_RADEON_CC_COLUMNS - 1)
				fColumn++;
		}
		else if (character != 0x00) {
			PRINT(("<%02X>", character));
		}
	}
	
	return true;
}

void CCaption::DisplayCaptions() const
{
	printf("\x1b[H\x1b[J");
	
	for (int row = 0; row < C_RADEON_CC_ROWS; row++) {
		for (int column = 0; column < C_RADEON_CC_COLUMNS; column++) {
			if (fDisplayText[row][column] == 0x0000) {
				printf("\x1b[0;47;37m ");
				continue;
			}
				
			const int code = (fDisplayText[row][column] >> 0) & 0xff;
			const int color = (fDisplayText[row][column] >> 8) & 0xff;

			switch (color & 0x000e) {
			case 0x0000: // WHITE
				printf("\x1b[0;%d;40;37m", (color & 1) << 2);
				break;
			case 0x0002: // GREEN
				printf("\x1b[0;%d;40;32m", (color & 1) << 2);
				break;
			case 0x0004: // BLUE
				printf("\x1b[0;%d;40;34m", (color & 1) << 2);
				break;
			case 0x0006: // CYAN
				printf("\x1b[0;%d;40;35m", (color & 1) << 2);
				break;
			case 0x0008: // RED
				printf("\x1b[0;%d;40;31m", (color & 1) << 2);
				break;
			case 0x000a: // YELLOW
				printf("\x1b[0;%d;40;33m", (color & 1) << 2);
				break;
			case 0x000c: // MAGENTA
				printf("\x1b[0;%d;40;36m", (color & 1) << 2);
				break;
			case 0x000e: // WHITE ITALICS
				if ((color & 1) == 0)
					printf("\x1b[1;40;37m");
				else
					printf("\x1b[1;4;40;37m");
				break;
			}
			
			if (code >= 0x20 && code <= 0x7f) {
				switch (code) {
				case 0x002a:
					// aacute
					printf("\xc3\xa1");
					break;
				case 0x005c:
					// eacute
					printf("\xc3\xa9");
					break;
				case 0x005e:
					// iacute
					printf("\xc3\xad");
					break;
				case 0x005f:
					// oacute
					printf("\xc3\xb3");
					break;
				case 0x0060:
					// uacute
					printf("\xc3\xba");
					break;
				case 0x007b:
					// ccedil
					printf("\xc3\xa7");
					break;
				case 0x007c:
					// division
					printf("\xc3\xb7");
					break;
				case 0x007d:
					// Ntilde
					printf("\xc3\x91");
					break;
				case 0x007e:
					// ntilde
					printf("\xc3\xb1");
					break;
				case 0x007f:
					// block
					printf("\xc1\xbf");
					break;
				default:
					// ASCII character
					printf("%c", code);
					break;
				}
			}
			else {
				switch (code) {
				case 0x0080:
					// reg
					printf("\xc2\xae");
					break;
				case 0x0081:
					// deg
					printf("\xc2\xb0");
					break;
				case 0x0082:
					// frac12
					printf("\xc2\xbd");
					break;
				case 0x0083:
					// iquest
					printf("\xc2\xbf");
					break;
				case 0x0084:
					// trademark
					printf("\xe2\x84");
					break;
				case 0x0085:
					// cent
					printf("\xc2\xa2");
					break;
				case 0x0086:
					// pound
					printf("\xc2\xa3");
					break;
				case 0x0087:
					// music
					printf("\xc2\xa7");
					break;
				case 0x0088:
					// agrave
					printf("\xc3\xa0");
					break;
				case 0x0089:
					// tspace
					printf("\x1b[0;47;37m ");
					break;
				case 0x008a:
					// egrave
					printf("\xc3\xa8");
					break;
				case 0x008b:
					// acirc
					printf("\xc3\xa2");
					break;
				case 0x008c:
					// ecirc
					printf("\xc3\xaa");
					break;
				case 0x008d:
					// icirc
					printf("\xc3\xae");
					break;
				case 0x008e:
					// ocirc
					printf("\xc3\xb4");
					break;
				case 0x008f:
					// ucirc
					printf("\xc3\xbb");
					break;
				default:
					// buggy code
					printf("<%02X>", code);
					break;
				}
			}
		}
		printf("\n");
	}
	printf("\x1b[0;30;47m");
}
