/*****************************************************************************/
// tiffinfo
// Written by Michael Wilber, OBOS Translation Kit Team
//
// Version:
//
// tiffinfo is a command line program for displaying text information about
// TIFF images. This information includes a listing of every field (tag) in
// the TIFF file, for every image in the file.  Also, for some fields,
// the numerical value for the field is converted to descriptive text.
//
// This application and all source files used in its construction, except 
// where noted, are licensed under the MIT License, and have been written 
// and are:
//
// Copyright (c) 2003 OpenBeOS Project
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
#include <ByteOrder.h>
#include <File.h>
#include <StorageDefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct IFDEntry {
	uint16 tag;
		// uniquely identifies the field
	uint16 fieldType;
		// number, string, float, etc.
	uint32 count;
		// length / number of values
		
	// The actual value or the file offset
	// where the actual value is located
	union {
		float 	floatval;
		uint32	longval;
		uint16	shortvals[2];
		uint8	bytevals[4];
	};
};

enum ENTRY_TYPE {
	TIFF_BYTE = 1,
	TIFF_ASCII,
	TIFF_SHORT,
	TIFF_LONG,
	TIFF_RATIONAL,
	TIFF_SBYTE,
	TIFF_UNDEFINED,
	TIFF_SSHORT,
	TIFF_SLONG,
	TIFF_SRATIONAL,
	TIFF_FLOAT,
	TIFF_DOUBLE
};

const char *
get_type_string(uint16 type)
{
	const char *kstrTypes[] = {
		"Byte",
		"ASCII",
		"Short",
		"Long",
		"Rational",
		"Signed Byte",
		"Undefined",
		"Signed Short",
		"Signed Long",
		"Signed Rational",
		"Float",
		"Double"
	};
	
	if (type >= 1 && type <= 12)
		return kstrTypes[type - 1];
	else
		return "?";
}

const char *
get_tag_string(uint16 tag)
{
	switch (tag) {
		case 254: return "New Subfile Type";
		case 255: return "Subfile Type";
		case 256: return "Image Width";
		case 257: return "Image Height";
		case 258: return "Bits Per Sample";
		case 259: return "Compression";
		case 262: return "Photometric Interpretation";
		case 263: return "Thresholding";
		case 264: return "CellWidth";
		case 265: return "CellLength";
		case 266: return "Fill Order";
		case 269: return "Document Name";
		case 270: return "Image Description";
		case 271: return "Make";
		case 272: return "Model";
		case 273: return "Strip Offsets";
		case 274: return "Orientation";
		case 277: return "Samples Per Pixel";
		case 278: return "Rows Per Strip";
		case 279: return "Strip Byte Counts";
		case 280: return "Min Sample Value";
		case 281: return "Max Sample Value";
		case 282: return "X Resolution";
		case 283: return "Y Resolution";
		case 284: return "Planar Configuration";
		case 285: return "Page Name";
		case 286: return "X Position";
		case 287: return "Y Position";
		case 288: return "Free Offsets";
		case 289: return "Free Byte Counts";
		case 290: return "Gray Response Unit";
		case 291: return "Gray Response Curve";
		case 292: return "T4 Options";
		case 293: return "T6 Options";
		case 296: return "Resolution Unit";
		case 297: return "Page Number";
		case 305: return "Software";
		case 306: return "DateTime";
		case 315: return "Artist";
		case 316: return "Host Computer";
		case 320: return "Color Map";
		case 322: return "Tile Width";
		case 323: return "Tile Height";
		case 324: return "Tile Offsets";
		case 325: return "Tile Byte Counts";
		case 338: return "Extra Samples";
		case 339: return "Sample Format";
		case 529: return "YCbCr Coefficients";
		case 530: return "YCbCr Subsampling";
		case 531: return "YCbCr Positioning";
		case 532: return "Reference Black White";
		case 32995: return "Matteing";
		case 32996: return "Data Type"; // obseleted by SampleFormat tag
		case 32997: return "Image Depth"; // tile / strip calculations
		case 32998: return "Tile Depth"; // tile / strip calculations
		case 33432: return "Copyright";		
		case 37439: return "StoNits?";
		
		default:
			return "?";
	}
}

void
print_ifd_value(IFDEntry &entry, BFile &file, swap_action swp)
{
	switch (entry.tag) {
		case 254: // NewSubfileType
			if (entry.count == 1 && entry.fieldType == TIFF_LONG) {
				if (entry.longval & 1)
					printf("Low Res (1) ");
				if (entry.longval & 2)
					printf("Page (2) ");
				if (entry.longval & 4)
					printf("Mask (4) ");
				
				printf("(0x%.8lx)", entry.longval);
				return;
			}
			break;
			
		case 256: // ImageWidth
		case 257: // ImageHeight
			if (entry.count == 1) {
				printf("%d",
					((entry.fieldType == TIFF_SHORT) ?
						entry.shortvals[0] : static_cast<unsigned int>(entry.longval)));
				return;
			}
			break;
			
		case 259:
			if (entry.count == 1 && entry.fieldType == TIFF_SHORT) {
				switch (entry.shortvals[0]) {
					case 1:
						printf("No Compression (1)");
						return;
					case 2:
						printf("CCITT Group 3 1-Dimensional Modified Huffman run-length encoding (2)");
						return;
					case 3:
						printf("Fax Group 3 (3)");
						return;
					case 4:
						printf("Fax Group 4 (4)");
						return;
					case 5:
						printf("LZW (5)");
						return;
					case 32773:
						printf("PackBits (32773)");
						return;
				}
			}
			break;
			
		case 262: // PhotometricInterpretation
			if (entry.count == 1 && entry.fieldType == TIFF_SHORT) {
				switch (entry.shortvals[0]) {
					case 0:
						printf("White is Zero (%d)", entry.shortvals[0]);
						return;
					case 1:
						printf("Black is Zero (%d)", entry.shortvals[0]);
						return;
					case 2:
						printf("RGB (%d)", entry.shortvals[0]);
						return;
					case 3:
						printf("Palette Color (%d)", entry.shortvals[0]);
						return;
					case 4:
						printf("Transparency Mask (%d)", entry.shortvals[0]);
						return;
				}
			}
			break;
			
		case 274: // Orientation
			if (entry.count == 1 && entry.fieldType == TIFF_SHORT) {
				switch (entry.shortvals[0]) {
					case 1:
						printf("top to bottom, left to right (1)");
						return;
					case 2:
						printf("top to bottom, right to left (2)");
						return;
					case 3:
						printf("bottom to top, right to left (3)");
						return;
					case 4:
						printf("bottom to top, left to right (4)");
						return;
					case 5:
						printf("left to right, top to bottom (5)");
						return;
					case 6:
						printf("right to left, top to bottom (6)");
						return;
					case 7:
						printf("right to left, bottom to top (7)");
						return;
					case 8:
						printf("left to right, bottom to top (8)");
						return;
				}
			}
			break;
			
		case 278: // RowsPerStrip
		{
			const uint32 ksinglestrip = 0xffffffffUL;
			printf("%u",
					static_cast<unsigned int>(entry.longval));
			if (entry.longval == ksinglestrip)
				printf(" (All rows in first strip)");
			return;
		}
			
		case 284: // PlanarConfiguration
			if (entry.count == 1 && entry.fieldType == TIFF_SHORT) {
				if (entry.shortvals[0] == 1) {
					printf("Chunky (%d)", entry.shortvals[0]);
					return;
				}
				else if (entry.shortvals[0] == 2) {
					printf("Planar (%d)", entry.shortvals[0]);
					return;
				}
			}
			break;
			
		case 296: // ResolutionUnit
			if (entry.count == 1 && entry.fieldType == TIFF_SHORT) {
				switch (entry.shortvals[0]) {
					case 1:
						printf("None (%d)", entry.shortvals[0]);
						return;
					case 2:
						printf("Inch (%d)", entry.shortvals[0]);
						return;
					case 3:
						printf("Cenimeter (%d)", entry.shortvals[0]);
						return;
				}
			}
			break;
			
		default:
			if (entry.fieldType == TIFF_ASCII) {
				char ascfield[256] = { 0 };
				
				if (entry.count <= 4)
					memcpy(ascfield, &entry.longval, entry.count);
				else if (entry.count > 4 && entry.count < 256) {
					ssize_t nread = file.ReadAt(entry.longval, ascfield, entry.count);
					if (nread != static_cast<ssize_t>(entry.count))
						ascfield[0] = '\0';
				}
				
				if (ascfield[0] != '\0') {
					printf("%s", ascfield);
					return;
				}
			} else if (entry.fieldType == TIFF_RATIONAL && entry.count == 1) {
				struct { uint32 numerator; uint32 denominator; } rational;
				
				ssize_t	nread = file.ReadAt(entry.longval, &rational, 8);
				if (nread == 8 &&
					swap_data(B_UINT32_TYPE, &rational, 8, swp) == B_OK) {
					
					printf("%u / %u (offset: 0x%.8lx)",
						static_cast<unsigned int>(rational.numerator),
						static_cast<unsigned int>(rational.denominator),
						entry.longval);
					return;
				}				
			} else if (entry.fieldType == TIFF_SRATIONAL && entry.count == 1) {
				struct { int32 numerator; int32 denominator; } srational;
				
				ssize_t	nread = file.ReadAt(entry.longval, &srational, 8);
				if (nread == 8 &&
					swap_data(B_INT32_TYPE, &srational, 8, swp) == B_OK) {
					
					printf("%d / %d (offset: 0x%.8lx)",
						static_cast<int>(srational.numerator),
						static_cast<int>(srational.denominator),
						entry.longval);
					return;
				}				
			} else if (entry.fieldType == TIFF_LONG && entry.count == 1) {
				printf("%u",
					static_cast<unsigned int>(entry.longval));
				return;
			} else if (entry.fieldType == TIFF_SLONG && entry.count == 1) {
				printf("%d",
					static_cast<int>(entry.longval));
				return;
			} else if (entry.fieldType == TIFF_SHORT && entry.count <= 2) {
				for (uint32 i = 0; i < entry.count; i++) {
					if (i > 0)
						printf(", ");
					printf("%u", entry.shortvals[i]);
				}
				return;
			} else if (entry.fieldType == TIFF_SSHORT && entry.count <= 2) {
				for (uint32 i = 0; i < entry.count; i++) {
					if (i > 0)
						printf(", ");
					printf("%d", entry.shortvals[i]);
				}
				return;
			} else if (entry.fieldType == TIFF_BYTE && entry.count <= 4) {
				for (uint32 i = 0; i < entry.count; i++) {
					if (i > 0)
						printf(", ");
					printf("%u", entry.bytevals[i]);
				}
				return;
			} else if (entry.fieldType == TIFF_SBYTE && entry.count <= 4) {
				for (uint32 i = 0; i < entry.count; i++) {
					if (i > 0)
						printf(", ");
					printf("%d", entry.bytevals[i]);
				}
				return;
			} else if (entry.fieldType == TIFF_UNDEFINED && entry.count <= 4) {
				for (uint32 i = 0; i < entry.count; i++) {
					if (i > 0)
						printf(", ");
					printf("0x%.2lx",
						static_cast<unsigned long>(entry.bytevals[i]));
				}
				return;
			}
			break;
	}
	printf("0x%.8lx", entry.longval);
}

int swap_value_field(IFDEntry &entry, swap_action swp)
{
	switch (entry.fieldType) {
		case TIFF_BYTE:
		case TIFF_ASCII:
		case TIFF_SBYTE:
		case TIFF_UNDEFINED:
			if (entry.count > 4) {
				if (swap_data(B_UINT32_TYPE, &entry.longval, 4, swp) != B_OK)
					return 0;
			}
			return 1;
			
		case TIFF_LONG:
		case TIFF_SLONG:
		case TIFF_RATIONAL:
		case TIFF_SRATIONAL:
		case TIFF_DOUBLE:
			if (swap_data(B_UINT32_TYPE, &entry.longval, 4, swp) != B_OK)
				return 0;
			return 1;
			
		case TIFF_FLOAT:
			if (swap_data(B_FLOAT_TYPE, &entry.floatval, 4, swp) != B_OK)
				return 0;
			return 1;
			
		case TIFF_SHORT:
		case TIFF_SSHORT:
			if (entry.count <= 2) {
				if (swap_data(B_UINT16_TYPE, &entry.shortvals,
					entry.count * 2, swp) != B_OK)
					return 0;
			} else {
				if (swap_data(B_UINT32_TYPE, &entry.longval, 4, swp) != B_OK)
					return 0;
			}
				
			return 1;
	}
	
	// no error, but unknown type
	return 2;
}

int
report_ifd_entries(BFile &file, uint16 entrycount, swap_action swp)
{
	IFDEntry entry;
	
	if (sizeof(IFDEntry) != 12) {
		printf("IFDEntry size must be 12\n");
		return 0;
	}
	
	off_t offset = file.Position();
	for (uint16 i = 0; i < entrycount; offset += 12, i++) {
		ssize_t nread = file.Read(&entry, 12);
		if (nread != 12) {
			printf("unable to read entire ifd entry\n");
			return 0;
		}	
		if (swap_data(B_UINT16_TYPE, &entry.tag, 4, swp) != B_OK ||
			swap_data(B_UINT32_TYPE, &entry.count, 4, swp) != B_OK) {
			printf("swap_data failed\n");
			return 0;
		}
		
		if (!swap_value_field(entry, swp)) {
			printf("swap_value_field failed\n");
			return 0;
		}
		
		printf("\nOffset: 0x%.8lx\n", static_cast<unsigned long>(offset));
		printf(  "   Tag: %s (%d)\n", get_tag_string(entry.tag), entry.tag);
		printf(  "  Type: %s (%d)\n", get_type_string(entry.fieldType),
			entry.fieldType);
		printf(  " Count: %d\n", static_cast<int>(entry.count));
		printf(  " Value: ");
		print_ifd_value(entry, file, swp);
		printf("\n");
	}
	
	return 1;
}

int
report_ifd(BFile &file, uint32 ifdoffset, swap_action swp)
{
	printf("\n<< BEGIN: IFD at 0x%.8lx >>\n\n", ifdoffset);
	
	if (file.Seek(ifdoffset, SEEK_SET) != ifdoffset) {
		printf("failed to seek to IFD offset: %d\n",
			static_cast<unsigned int>(ifdoffset));
		return 0;
	}
	
	uint16 entrycount = 0;
	ssize_t nread = file.Read(&entrycount, 2);
	if (nread != 2) {
		printf("unable to read entry count\n");
		return 0;
	}
	if (swap_data(B_UINT16_TYPE, &entrycount, sizeof(uint16), swp) != B_OK) {
		printf("failed to swap entrycount\n");
		return 0;
	}
	printf("Entry Count: %d\n", entrycount);
	
	// Print out entries
	int ret = report_ifd_entries(file, entrycount, swp);
	
	if (ret) {
		uint32 nextIFDOffset = 0;
		
		nread = file.Read(&nextIFDOffset, 4);
		if (nread != 4) {
			printf("unable to read next IFD\n");
			return 0;
		}
		if (swap_data(B_UINT32_TYPE, &nextIFDOffset, sizeof(uint32), swp) != B_OK) {
			printf("failed to swap next IFD\n");
			return 0;
		}
		
		printf("Next IFD Offset: 0x%.8lx\n", nextIFDOffset);
		printf("\n<< END: IFD at 0x%.8lx >>\n\n", ifdoffset);
		
		if (nextIFDOffset != 0)
			return report_ifd(file, nextIFDOffset, swp);
		else
			return 1;
		
	} else
		return 0;
}

int generate_report(const char *filepath)
{
	BFile file(filepath, B_READ_ONLY);
	
	if (file.InitCheck() == B_OK) {
	
		uint8 buffer[64];
		
		// Byte Order
		const uint8 kleSig[] = { 0x49, 0x49, 0x2a, 0x00 };
		const uint8 kbeSig[] = { 0x4d, 0x4d, 0x00, 0x2a };
		
		ssize_t nread = file.Read(buffer, 4);
		if (nread != 4) {
			printf("Unable to read first 4 bytes\n");
			return 0;
		}
		
		swap_action swp;
		if (memcmp(buffer, kleSig, 4) == 0) {
			swp = B_SWAP_LENDIAN_TO_HOST;
			printf("Byte Order: little endian\n");
			
		} else if (memcmp(buffer, kbeSig, 4) == 0) {
			swp = B_SWAP_BENDIAN_TO_HOST;
			printf("Byte Order: big endian\n");
			
		} else {
			printf("Invalid byte order value\n");
			return 0;
		}
		
		// Location of first IFD
		uint32 firstIFDOffset = 0;
		nread = file.Read(&firstIFDOffset, 4);
		if (nread != 4) {
			printf("Unable to read first IFD offset\n");
			return 0;
		}
		if (swap_data(B_UINT32_TYPE, &firstIFDOffset, sizeof(uint32), swp) != B_OK) {
			printf("swap_data() error\n");
			return 0;
		}
		printf("First IFD: 0x%.8lx\n", firstIFDOffset);
		
		// print out first IFD
		report_ifd(file, firstIFDOffset, swp);
	
		return 1;
	}
	
	return 0;
}

int main(int argc, char **argv)
{
	printf("\n");
		// put a line break at the beginning of output
		// to improve readability
	
	if (argc == 2) {
	
		printf("TIFF Image: %s\n\n", argv[1]);
		generate_report(argv[1]);
		
	} else {
	
		printf("tiffinfo - reports information about a TIFF image\n");
		printf("\nUsage:\n");
		printf("tiffinfo filename.tif\n\n");	
	}
	
	return 0;
}

