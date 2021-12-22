/*****************************************************************************/
// stxtinfo
// Written by Michael Wilber, Haiku Translation Kit Team
//
// Version:
//
// stxtinfo is a command line program for displaying text information about
// Be styled text (the format that StyledEdit uses).  StyledEdit stores the
// styled text information as an attribute of the file and it is this 
// information that this program is concerned with.  This format is outlined
// in TranslatorFormats.h.
//
// This program prints out information from the "styles" attribute that
// StyledEdit uses.  If that information is not available, it attempts
// to read the styles information from the contents of the file, assuming
// that it is the format that BTranslationUtils::PutStyledText() writes
// out.
//
// The intention of this program is to aid with the development and 
// debugging of the STXTTranslator.  It may also be useful for debugging /
// testing StyledEdit. 
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2003 Haiku Project
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense, 
// and/or sell copies of the Software, and to permit persons to whom the 
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included 
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL 
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING 
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
// DEALINGS IN THE SOFTWARE.
/*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ByteOrder.h>
#include <File.h>
#include <TranslatorFormats.h>
#include <Font.h>
	// for B_UNICODE_UTF8
#include <fs_attr.h>
	// for attr_info

#define max(x,y) ((x > y) ? x : y)
#define DATA_BUFFER_SIZE 64

struct StylesHeader {
	uint32 magic;	// 41 6c 69 21
	uint32 version;	// 0
	int32 count;
};

struct Style {
	int32	offset;
	char	family[64];
	char	style[64];
	float	size;
	float	shear;	// typically 90.0
	uint16	face;	// typically 0
	uint8	red;
	uint8	green;
	uint8	blue;
	uint8	alpha;	// 255 == opaque
	uint16	reserved; // 0
};

void
PrintStyle(Style &style, int32 i)
{
	style.offset = B_BENDIAN_TO_HOST_INT32(style.offset);
	style.size = B_BENDIAN_TO_HOST_FLOAT(style.size);
	style.shear = B_BENDIAN_TO_HOST_FLOAT(style.shear);
	style.face = B_BENDIAN_TO_HOST_INT16(style.face);
	style.reserved = B_BENDIAN_TO_HOST_INT16(style.reserved);			
			
	printf("\nStyle %d:\n", static_cast<int>(i + 1));
	printf("offset: %d\n", static_cast<int>(style.offset));
	printf("family: %s\n", style.family);
	printf("style: %s\n", style.style);
	printf("size: %f\n", style.size);
	printf("shear: %f (typically 90.0)\n", style.shear);
	printf("face: %u (typically 0)\n",
		static_cast<unsigned int>(style.face));
	printf("RGBA: (%u, %u, %u, %u)\n",
		static_cast<unsigned int>(style.red),
		static_cast<unsigned int>(style.blue),
		static_cast<unsigned int>(style.green),
		static_cast<unsigned int>(style.alpha));
		printf("reserved: %u (should be 0)\n",
		static_cast<unsigned int>(style.reserved));
}

bool
PrintStylesAttribute(BFile &file)
{
	const char *kAttrName = "styles";
	attr_info info;
	
	if (file.GetAttrInfo(kAttrName, &info) != B_OK)
		return false;
	if (info.type != B_RAW_TYPE) {
		printf("Error: styles attribute is of the wrong type\n");
		return false;
	}
	if (info.size < 160) {
		printf("Error: styles attribute is missing information\n");
		return false;
	}

	uint8 *pflatRunArray = new uint8[info.size];
	if (!pflatRunArray) {
		printf("Error: Not enough memory available to read styles attribute\n");
		return false;
	}
	
	ssize_t amtread = file.ReadAttr(kAttrName, B_RAW_TYPE, 0,
		pflatRunArray, info.size);
	if (amtread != info.size) {
		printf("Error: Unable to read styles attribute\n");
		return false;
	}
				
	// Check Styles
	StylesHeader stylesheader;
	memcpy(&stylesheader, pflatRunArray, sizeof(StylesHeader));
	if (swap_data(B_UINT32_TYPE, &stylesheader, sizeof(StylesHeader),
		B_SWAP_BENDIAN_TO_HOST) != B_OK) {
		printf("Error: Unable to swap byte order of styles header\n");
		return false;
	}
		
	// Print StylesHeader info
	printf("\"styles\" attribute data:\n\n");
	
	printf("magic number: 0x%.8lx ",
		static_cast<unsigned long>(stylesheader.magic));
	if (stylesheader.magic == 'Ali!')
		printf("(valid)\n");
	else
		printf("(INVALID, should be 0x%.8lx)\n",
			static_cast<unsigned long>('Ali!'));
		
	printf("version: 0x%.8lx ",
		static_cast<unsigned long>(stylesheader.version));
	if (stylesheader.version == 0)
		printf("(valid)\n");
	else
		printf("(INVALID, should be 0x%.8lx)\n", 0UL);
			
	printf("number of styles: %d\n",
		static_cast<int>(stylesheader.count));
		
	// Check and Print out each style
	Style *pstyle = reinterpret_cast<Style *>(pflatRunArray + sizeof(StylesHeader));
	for (int32 i = 0; i < stylesheader.count; i++)
		PrintStyle(*(pstyle + i), i);
				
	delete[] pflatRunArray;
	pflatRunArray = NULL;
	
	return true;
}

bool
PrintStxtInfo(BFile &file)
{
	const uint32 kstxtsize = sizeof(TranslatorStyledTextStreamHeader);
	const uint32 ktxtsize = sizeof(TranslatorStyledTextTextHeader);
	const uint32 kstylsize = sizeof(TranslatorStyledTextStyleHeader);
	const uint32 kStyleSize = sizeof(Style);
		
	uint8 buffer[max(max(max(kstxtsize, ktxtsize), kstylsize), kStyleSize)];
		
	// Check STXT Header
	status_t nread = 0;
	nread = file.Read(buffer, kstxtsize);
	if (nread != static_cast<status_t>(kstxtsize)) {
		printf("Error: Unable to read stream header\n");
		return false;
	}
	TranslatorStyledTextStreamHeader stxtheader;
	memcpy(&stxtheader, buffer, kstxtsize);
	if (swap_data(B_UINT32_TYPE, &stxtheader, kstxtsize,
		B_SWAP_BENDIAN_TO_HOST) != B_OK) {
		printf("Error: Unable to swap byte order of stream header\n");
		return false;
	}
	
	if (stxtheader.header.magic != B_STYLED_TEXT_FORMAT) {
		printf("Styled text magic number is incorrect, aborting.\n");
		return false;
	}
		
	// Print Stream Header (STXT)
	printf("Stream Header (STXT section):\n\n");
		
	printf("magic number: 0x%.8lx ", stxtheader.header.magic);
	if (stxtheader.header.magic == B_STYLED_TEXT_FORMAT)
		printf("(valid)\n");
	else
		printf("(INVALID, should be 0x%.8lx)\n",
			static_cast<unsigned long>(B_STYLED_TEXT_FORMAT));
				
	printf("header size: %u ",
		static_cast<unsigned int>(stxtheader.header.header_size));
	if (stxtheader.header.header_size == kstxtsize)
		printf("(valid)\n");
	else
		printf("(INVALID, should be %u)\n",
			static_cast<unsigned int>(kstxtsize));
				
	printf("data size: %u ",
		static_cast<unsigned int>(stxtheader.header.data_size));
	if (stxtheader.header.data_size == 0)
		printf("(valid)\n");
	else
		printf("(INVALID, should be 0)\n");
			
	printf("version: %d ",
		static_cast<int>(stxtheader.version));
	if (stxtheader.version == 100)
		printf("(valid)\n");
		else
		printf("(INVALID, should be 100)\n");

	
	// Check the TEXT header
	TranslatorStyledTextTextHeader txtheader;
	if (file.Read(buffer, ktxtsize) != static_cast<ssize_t>(ktxtsize)) {
		printf("Error: Unable to read text header\n");
		return false;
	}
	memcpy(&txtheader, buffer, ktxtsize);
	if (swap_data(B_UINT32_TYPE, &txtheader, ktxtsize,
		B_SWAP_BENDIAN_TO_HOST) != B_OK) {
		printf("Error: Unable to swap byte order of text header\n");
		return false;
	}
			
	// Print Text Header (TEXT)
	printf("\nText Header (TEXT section):\n\n");
		
	printf("magic number: 0x%.8lx ", txtheader.header.magic);
	if (txtheader.header.magic == 'TEXT')
		printf("(valid)\n");
	else
		printf("(INVALID, should be 0x%.8lx)\n",
			static_cast<unsigned long>('TEXT'));
				
	printf("header size: %u ",
		static_cast<unsigned int>(txtheader.header.header_size));
	if (stxtheader.header.header_size == ktxtsize)
		printf("(valid)\n");
	else
		printf("(INVALID, should be %u)\n",
			static_cast<unsigned int>(ktxtsize));
				
	printf("data size (bytes of text): %u\n",
		static_cast<unsigned int>(txtheader.header.data_size));
			
	printf("character set: %d ",
		static_cast<int>(txtheader.charset));
	if (txtheader.charset == B_UNICODE_UTF8)
		printf("(valid)\n");
	else
		printf("(INVALID, should be %d)\n", B_UNICODE_UTF8);
			
	// Skip the text data
	off_t seekresult, pos;
	pos = stxtheader.header.header_size +
		txtheader.header.header_size +
		txtheader.header.data_size;
	seekresult = file.Seek(txtheader.header.data_size, SEEK_CUR);
	if (seekresult < pos) {
		printf("Error: Unable to seek past text data. " \
			"Text data could be missing\n");
		return false;
	}
	if (seekresult > pos) {
		printf("Error: File position is beyond expected value\n");
		return false;
	}

	// Check the STYL header (not all STXT files have this)
	ssize_t read = 0;
	TranslatorStyledTextStyleHeader stylheader;
	read = file.Read(buffer, kstylsize);
	if (read != static_cast<ssize_t>(kstylsize) && read != 0) {
		printf("Error: Unable to read entire style header\n");
		return false;
	}
	
	// If there is no STYL header (and no errors)
	if (read != static_cast<ssize_t>(kstylsize)) {
		printf("\nFile contains no Style Header (STYL section)\n");
		return false;
	}
			
	memcpy(&stylheader, buffer, kstylsize);
	if (swap_data(B_UINT32_TYPE, &stylheader, kstylsize,
		B_SWAP_BENDIAN_TO_HOST) != B_OK) {
		printf("Error: Unable to swap byte order of style header\n");
		return false;
	}
		
	// Print Style Header (STYL)
	printf("\nStyle Header (STYL section):\n\n");
	
	printf("magic number: 0x%.8lx ", stylheader.header.magic);
	if (stylheader.header.magic == 'STYL')
		printf("(valid)\n");
	else
		printf("(INVALID, should be 0x%.8lx)\n",
			static_cast<unsigned long>('STYL'));
				
	printf("header size: %u ",
		static_cast<unsigned int>(stylheader.header.header_size));
	if (stylheader.header.header_size == kstylsize)
		printf("(valid)\n");
	else
		printf("(INVALID, should be %u)\n",
			static_cast<unsigned int>(kstylsize));
				
	printf("data size: %u\n",
		static_cast<unsigned int>(stylheader.header.data_size));
	printf("apply offset: %u (usually 0)\n",
		static_cast<unsigned int>(stylheader.apply_offset));
	printf("apply length: %u (usually the text data size)\n",
		static_cast<unsigned int>(stylheader.apply_length));
			
	// Check Styles
	StylesHeader stylesheader;
	read = file.Read(buffer, sizeof(StylesHeader));
	if (read != sizeof(StylesHeader)) {
		printf("Error: Unable to read Styles header\n");
		return false;
	}
	memcpy(&stylesheader, buffer, sizeof(StylesHeader));
	if (swap_data(B_UINT32_TYPE, &stylesheader, sizeof(StylesHeader),
		B_SWAP_BENDIAN_TO_HOST) != B_OK) {
		printf("Error: Unable to swap byte order of styles header\n");
		return false;
	}
		
	// Print StylesHeader info
	printf("\nStyles Header (Ali! section):\n\n");
	
	printf("magic number: 0x%.8lx ",
		static_cast<unsigned long>(stylesheader.magic));
	if (stylesheader.magic == 'Ali!')
		printf("(valid)\n");
	else
		printf("(INVALID, should be 0x%.8lx)\n",
			static_cast<unsigned long>('Ali!'));
		
	printf("version: 0x%.8lx ",
		static_cast<unsigned long>(stylesheader.version));
	if (stylesheader.version == 0)
		printf("(valid)\n");
	else
		printf("(INVALID, should be 0x%.8lx)\n", 0UL);
			
	printf("number of styles: %d\n",
		static_cast<int>(stylesheader.count));
		
	// Check and Print out each style
	for (int32 i = 0; i < stylesheader.count; i++) {
		Style style;
		read = file.Read(&style, sizeof(Style));
		if (read != sizeof(Style)) {
			printf("Error: Unable to read style %d\n",
				static_cast<int>(i + 1));
			return false;
		}			
			
		PrintStyle(style, i);
	}
	
	return true;
}

int
main(int argc, char **argv)
{
	printf("\n");
	
	if (argc == 2) {
		BFile file(argv[1], B_READ_ONLY);
		if (file.InitCheck() != B_OK)
			printf("Error opening %s\n", argv[1]);
		else {
			printf("Be styled text information for: %s\n\n", argv[1]);
			if (PrintStylesAttribute(file) == false) {
				printf("Unable to read styles attribute, attempting to read " \
					"style information from file...\n\n");
				PrintStxtInfo(file);
			}
		}
	}
	else {
		printf("stxtinfo - reports information about a Be styled text file\n");
		printf("\nUsage:\n");
		printf("stxtinfo filename.stxt\n");	
	}
	
	printf("\n");

	return 0;
}

