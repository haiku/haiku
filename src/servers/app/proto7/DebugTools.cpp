#include <iostream.h>
#include <stdio.h>
#include "DebugTools.h"

void PrintStatusToStream(status_t value)
{
	// Function which simply translates a returned status code into a string
	// and dumps it to stdout
	cout << "Status: " << TranslateStatusToString(value).String() << endl << flush;
}

BString TranslateStatusToString(status_t value)
{
	BString outstr;
	switch(value)
	{
		case B_OK:
			outstr="B_OK ";
			break;
		case B_NAME_NOT_FOUND:
			outstr="B_NAME_NOT_FOUND ";
			break;
		case B_BAD_VALUE:
			outstr="B_BAD_VALUE ";
			break;
		case B_ERROR:
			outstr="B_ERROR ";
			break;
		case B_TIMED_OUT:
			outstr="B_TIMED_OUT ";
			break;
		case B_NO_MORE_PORTS:
			outstr="B_NO_MORE_PORTS ";
			break;
		case B_WOULD_BLOCK:
			outstr="B_WOULD_BLOCK ";
			break;
		case B_BAD_PORT_ID:
			outstr="B_BAD_PORT_ID ";
			break;
		case B_BAD_TEAM_ID:
			outstr="B_BAD_TEAM_ID ";
			break;
		default:
			outstr="undefined status value in debugtools::PrintStatusToStream() ";
			break;
	}
	return BString(outstr);
}

void PrintColorSpaceToStream(color_space value)
{
	// Dump a color space to cout
	BString outstr;
	switch(value)
	{
		case B_RGB32:
			outstr="B_RGB32 ";
			break;
		case B_RGBA32:
			outstr="B_RGBA32 ";
			break;
		case B_RGB32_BIG:
			outstr="B_RGB32_BIG ";
			break;
		case B_RGBA32_BIG:
			outstr=" ";
			break;
		case B_UVL32:
			outstr="B_UVL32 ";
			break;
		case B_UVLA32:
			outstr="B_UVLA32 ";
			break;
		case B_LAB32:
			outstr="B_LAB32 ";
			break;
		case B_LABA32:
			outstr="B_LABA32 ";
			break;
		case B_HSI32:
			outstr="B_HSI32 ";
			break;
		case B_HSIA32:
			outstr="B_HSIA32 ";
			break;
		case B_HSV32:
			outstr="B_HSV32 ";
			break;
		case B_HSVA32:
			outstr="B_HSVA32 ";
			break;
		case B_HLS32:
			outstr="B_HLS32 ";
			break;
		case B_HLSA32:
			outstr="B_HLSA32 ";
			break;
		case B_CMY32:
			outstr="B_CMY32";
			break;
		case B_CMYA32:
			outstr="B_CMYA32 ";
			break;
		case B_CMYK32:
			outstr="B_CMYK32 ";
			break;
		case B_RGB24_BIG:
			outstr="B_RGB24_BIG ";
			break;
		case B_RGB24:
			outstr="B_RGB24 ";
			break;
		case B_LAB24:
			outstr="B_LAB24 ";
			break;
		case B_UVL24:
			outstr="B_UVL24 ";
			break;
		case B_HSI24:
			outstr="B_HSI24 ";
			break;
		case B_HSV24:
			outstr="B_HSV24 ";
			break;
		case B_HLS24:
			outstr="B_HLS24 ";
			break;
		case B_CMY24:
			outstr="B_CMY24 ";
			break;
		case B_GRAY1:
			outstr="B_GRAY1 ";
			break;
		case B_CMAP8:
			outstr="B_CMAP8 ";
			break;
		case B_GRAY8:
			outstr="B_GRAY8 ";
			break;
		case B_YUV411:
			outstr="B_YUV411 ";
			break;
		case B_YUV420:
			outstr="B_YUV420 ";
			break;
		case B_YCbCr422:
			outstr="B_YCbCr422 ";
			break;
		case B_YCbCr411:
			outstr="B_YCbCr411 ";
			break;
		case B_YCbCr420:
			outstr="B_YCbCr420 ";
			break;
		case B_YUV422:
			outstr="B_YUV422 ";
			break;
		case B_YUV9:
			outstr="B_YUV9 ";
			break;
		case B_YUV12:
			outstr="B_YUV12 ";
			break;
		case B_RGB15:
			outstr="B_RGB15 ";
			break;
		case B_RGBA15:
			outstr="B_RGBA15 ";
			break;
		case B_RGB16:
			outstr="B_RGB16 ";
			break;
		case B_RGB16_BIG:
			outstr="B_RGB16_BIG ";
			break;
		case B_RGB15_BIG:
			outstr="B_RGB15_BIG ";
			break;
		case B_RGBA15_BIG:
			outstr="B_RGBA15_BIG ";
			break;
		case B_YCbCr444:
			outstr="B_YCbCr444 ";
			break;
		case B_YUV444:
			outstr="B_YUV444 ";
			break;
		case B_NO_COLOR_SPACE:
			outstr="B_NO_COLOR_SPACE ";
			break;
		default:
			outstr="Undefined color space ";
			break;
	}
	cout << "Color Space: " << outstr.String() << flush;
}

void TranslateMessageCodeToStream(int32 code)
{
	// Used to translate BMessage message codes back to a character
	// format
	
		cout << "'" 
		<< (char)((code & 0xFF000000) >>  24)
		<< (char)((code & 0x00FF0000) >>  16)
		<< (char)((code & 0x0000FF00) >>  8)
		<< (char)((code & 0x000000FF)) << "' ";
}

void PrintMessageCode(int32 code)
{
	// Used to translate BMessage message codes back to a character
	// format
	
		printf("Message code %c%c%c%c\n",
		(char)((code & 0xFF000000) >>  24),
		(char)((code & 0x00FF0000) >>  16),
		(char)((code & 0x0000FF00) >>  8),
		(char)((code & 0x000000FF)) );
}

