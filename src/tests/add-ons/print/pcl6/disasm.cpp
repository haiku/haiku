/*

PCL6 Disassembler

Copyright (c) 2003 Haiku.

Author: 
	Michael Pfeiffer

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.

*/

#include <stdio.h>
#include <ctype.h>
#include "disasm.h"

// Implementation of InputStream

int InputStream::Read(void* buffer, int size) {
	if (size > 0) {
		uint8* b = (uint8*)buffer;
		int s = 0;
		if (fBufferSize > 0) {
			*b = fBuffer;
			b ++;
			size --;
			s = 1;
			fBufferSize = 0;
		}
		s = s + RawRead(b, size);
		fPos += s;
		return s;
	}
	return 0;
}

bool InputStream::PutUByte(uint8 byte) {
	if (fBufferSize != 0) return false;
	fBufferSize = 1; fBuffer = byte; fPos --;
	return true;
}

bool InputStream::ReadUByte(uint8& v) {
	return sizeof(v) == Read(&v, sizeof(v));
}

bool InputStream::ReadUInt16(uint16& v) {
	return sizeof(v) == Read(&v, sizeof(v));
}

bool InputStream::ReadUInt32(uint32& v) {
	return sizeof(v) == Read(&v, sizeof(v));
}

bool InputStream::ReadSInt16(int16& v) {
	return sizeof(v) == Read(&v, sizeof(v));
}

bool InputStream::ReadSInt32(int32& v) {
	return sizeof(v) == Read(&v, sizeof(v));
}

bool InputStream::ReadReal32(float& v) {
	return sizeof(v) == Read(&v, sizeof(v));
}

// Implementation of File

File::File(const char* filename) {
	fFile = fopen(filename, "r+b");
}

File::~File() {
	if (fFile) {
		fclose(fFile); fFile = NULL;
	}
}

int File::RawRead(void* buffer, int size) {
	return fread(buffer, 1, size, fFile);
}

// Implementation of Disasm

void Disasm::Print() {
	if (!ParsePJL()) {
		Error("Error parsing PJL header");
	} else if (!ParsePCL6()) {
		Error("Error parsing PCL6");
		fprintf(stderr, "File position %d\n", fStream->Pos());
	}
}

void Disasm::Error(const char* text) {
	fprintf(stderr, "%s\n", text);
}

bool Disasm::Expect(const char* text) {
	uint8 byte;
	for (uint8* t = (uint8*)text; *t != '\0'; t++) {
		if (!fStream->ReadUByte(byte) || byte != *t) return false;
	}
	return true;
}

bool Disasm::SkipTo(const char* text) {
	uint8 byte;
	if (!fStream->ReadUByte(byte)) return false;
	startagain:
	for (;;) {
		uint8* t = (uint8*)text;
		// find first character
		if (byte != *t) {
			if (!fStream->ReadUByte(byte)) return false;
			continue;
		} 
		// compare rest
		for (t ++; *t != '\0'; t ++) {
			if (!fStream->ReadUByte(byte)) return false;
			if (byte != *t) goto startagain;
		}
		return true;
	}
	return true;
}

bool Disasm::ReadNumber(int32& n) {
	n = 0;
	uint8 byte;
	while (fStream->ReadUByte(byte)) {
		if (isdigit(byte)) {
			n = 10 * n + (byte - '0');
		} else {
			fStream->PutUByte(byte);
			return true;
		}
	}
	return false;
}

bool Disasm::ParsePJL() {
	if (!Expect("\033%-12345X") || !SkipTo(") HP-PCL XL;")) return false;
	// version";"minor";" ... \n
	int32 version, minor;
	if (ReadNumber(version) && Expect(";") && ReadNumber(minor) && Expect(";") && SkipTo("\n")) {
		printf("PCL XL %d ; %d\n", (int)version, (int)minor);
		return true;
	}
	return false;	
}

void Disasm::DeleteAttr(struct ATTRIBUTE* attr) {
	if (attr) {
		switch (attr->Type) {
		case  HP_UByteArray: delete attr->val.ubyte_array;
			break;
		case  HP_UInt16Array: delete attr->val.uint16_array;
			break;
		case  HP_UInt32Array: delete attr->val.uint32_array;
			break;
		case  HP_SInt16Array: delete attr->val.sint16_array;
			break;
		case  HP_SInt32Array: delete attr->val.sint32_array;
			break;
		case  HP_Real32Array: delete attr->val.real32_array;
			break;
		};
		delete attr;
	}
}

void Disasm::ClearAttrs() {
	for (int i = 0; i < NumOfArgs(); i ++) {
		DeleteAttr(ArgAt(i));
	}
	fArgs.MakeEmpty();
}


bool Disasm::DecodeOperator(uint8 byte) {
	switch (byte) {
		case HP_BeginSession: printf("BeginSession\n"); break;
		case HP_EndSession: printf("EndSession\n"); break;
		case HP_BeginPage: printf("BeginPage\n"); break;
		case HP_EndPage: printf("EndPage\n"); break;
		case HP_VendorUnique: printf("VendorUnique\n"); break;
		case HP_Comment: printf("Comment\n"); break;
		case HP_OpenDataSource: printf("OpenDataSource\n"); break;
		case HP_CloseDataSource: printf("CloseDataSource\n"); break;
		case HP_EchoComment: printf("EchoComment\n"); break;
		case HP_Query: printf("Query\n"); break;
		case HP_Diagnostic3: printf("Diagnostic3\n"); break;

		case HP_BeginFontHeader: printf("BeginFontHeader\n"); break;
		case HP_ReadFontHeader: printf("ReadFontHeader\n"); break;
		case HP_EndFontHeader: printf("EndFontHeader\n"); break;
		case HP_BeginChar: printf("BeginChar\n"); break;
		case HP_ReadChar: printf("ReadChar\n"); break;
		case HP_EndChar: printf("EndChar\n"); break;
		case HP_RemoveFont: printf("RemoveFont\n"); break;
		case HP_SetCharAttribute: printf("SetCharAttribute\n"); break;

		case HP_SetDefaultGS: printf("SetDefaultGS\n"); break;
		case HP_SetColorTreatment: printf("SetColorTreatment\n"); break;
		case HP_SetGlobalAttributes: printf("SetGlobalAttributes\n"); break;
		case HP_ClearGlobalAttributes: printf("ClearGlobalAttributes\n"); break;
		case HP_BeginStream: printf("BeginStream\n"); break;
		case HP_ReadStream: printf("ReadStream\n"); break;
		case HP_EndStream: printf("EndStream\n"); break;
		case HP_ExecStream: printf("ExecStream\n"); break;
		case HP_RemoveStream: printf("RemoveStream\n"); break;

		case HP_PopGS: printf("PopGS\n"); break;
		case HP_PushGS: printf("PushGS\n"); break;
		case HP_SetClipReplace: printf("SetClipReplace\n"); break;
		case HP_SetBrushSource: printf("SetBrushSource\n"); break;
		case HP_SetCharAngle: printf("SetCharAngle\n"); break;
		case HP_SetCharScale: printf("SetCharScale\n"); break;
		case HP_SetCharShear: printf("SetCharShear\n"); break;
		case HP_SetClipIntersect: printf("SetClipIntersect\n"); break;
		case HP_SetClipRectangle: printf("SetClipRectangle\n"); break;
		case HP_SetClipToPage: printf("SetClipToPage\n"); break;
		case HP_SetColorSpace: printf("SetColorSpace\n"); break;
		case HP_SetCursor: printf("SetCursor\n"); break;
		case HP_SetCursorRel: printf("SetCursorRel\n"); break;
		case HP_SetHalftoneMethod: printf("SetHalftoneMethod\n"); break;
		case HP_SetFillMode: printf("SetFillMode\n"); break;
		case HP_SetFont: printf("SetFont\n"); break;
		case HP_SetLineDash: printf("SetLineDash\n"); break;
		case HP_SetLineCap: printf("SetLineCap\n"); break;
		case HP_SetLineJoin: printf("SetLineJoin\n"); break;
		case HP_SetMiterLimit: printf("SetMiterLimit\n"); break;
		case HP_SetPageDefaultCTM: printf("SetPageDefaultCTM\n"); break;
		case HP_SetPageOrigin: printf("SetPageOrigin\n"); break;
		case HP_SetPageRotation: printf("SetPageRotation\n"); break;
		case HP_SetPageScale: printf("SetPageScale\n"); break;
		case HP_SetPaintTxMode: printf("SetPaintTxMode\n"); break;
		case HP_SetPenSource: printf("SetPenSource\n"); break;
		case HP_SetPenWidth: printf("SetPenWidth\n"); break;
		case HP_SetROP: printf("SetROP\n"); break;
		case HP_SetSourceTxMode: printf("SetSourceTxMode\n"); break;
		case HP_SetCharBoldValue: printf("SetCharBoldValue\n"); break;
		case HP_SetClipMode: printf("SetClipMode\n"); break;
		case HP_SetPathToClip: printf("SetPathToClip\n"); break;
		case HP_SetCharSubMode: printf("SetCharSubMode\n"); break;
		case HP_BeginUserDefinedLineCap: printf("BeginUserDefinedLineCap\n"); break;
		case HP_EndUserDefinedLineCap: printf("EndUserDefinedLineCap\n"); break;
		case HP_CloseSubPath: printf("CloseSubPath\n"); break;
		case HP_NewPath: printf("NewPath\n"); break;
		case HP_PaintPath: printf("PaintPath\n"); break;
		case HP_BeginBackground: printf("BeginBackground\n"); break;
		case HP_EndBackground: printf("EndBackground\n"); break;
		case HP_DrawBackground: printf("DrawBackground\n"); break;
		case HP_RemoveBackground: printf("RemoveBackground\n"); break;
		case HP_BeginForm: printf("BeginForm\n"); break;
		case HP_EndForm: printf("EndForm\n"); break;
		case HP_DrawForm: printf("DrawForm\n"); break;
		case HP_RemoveForm: printf("RemoveForm\n"); break;
		case HP_RegisterFormAsPattern: printf("RegisterFormAsPattern\n"); break;

		case HP_ArcPath: printf("ArcPath\n"); break;

		case HP_BezierPath: printf("BezierPath\n"); break;
		case HP_BezierRelPath: printf("BezierRelPath\n"); break;
		case HP_Chord: printf("Chord\n"); break;
		case HP_ChordPath: printf("ChordPath\n"); break;
		case HP_Ellipse: printf("Ellipse\n"); break;
		case HP_EllipsePath: printf("EllipsePath\n"); break;

		case HP_LinePath: printf("LinePath\n"); break;

		case HP_LineRelPath: printf("LineRelPath\n"); break;
		case HP_Pie: printf("Pie\n"); break;
		case HP_PiePath: printf("PiePath\n"); break;

		case HP_Rectangle: printf("Rectangle\n"); break;
		case HP_RectanglePath: printf("RectanglePath\n"); break;
		case HP_RoundRectangle: printf("RoundRectangle\n"); break;
		case HP_RoundRectanglePath: printf("RoundRectanglePath\n"); break;

		case HP_Text: printf("Text\n"); break;
		case HP_TextPath: printf("TextPath\n"); break;

		case HP_SystemText: printf("SystemText\n"); break;

		case HP_BeginImage: printf("BeginImage\n"); break;
		case HP_ReadImage: printf("ReadImage\n"); break;
		case HP_EndImage: printf("EndImage\n"); break;
		case HP_BeginRastPattern: printf("BeginRastPattern\n"); break;
		case HP_ReadRastPattern: printf("ReadRastPattern\n"); break;
		case HP_EndRastPattern: printf("EndRastPattern\n"); break;
		case HP_BeginScan: printf("BeginScan\n"); break;
		case HP_EndScan: printf("EndScan\n"); break;
		case HP_ScanLineRel: printf("ScanLineRel\n"); break;

		case HP_PassThrough: printf("PassThrough\n"); break;
		default: 
			printf("Unsupported operator %d %x\n", (int)byte, (int)byte);
			return false;
	}
	return true;
}

bool Disasm::ReadArrayLength(uint32& length) {
	uint8 type;
	if (fStream->ReadUByte(type)) {
		struct ATTRIBUTE* attr = ReadData(type);
		if (attr == NULL) return false;
		if (attr->Type == HP_UByteData) {
			length = attr->val.ubyte;
		} else if (attr->Type == HP_UInt16Data) {
			length = attr->val.uint16;
		} else {
			DeleteAttr(attr); return false;
		}
		DeleteAttr(attr);return true;
	}
	return false;
}

#define READ_ARRAY(array_name, c_type, type) \
	attr->val.array_name = new c_type[length]; \
	attr->arrayLen = length; \
	for (uint32 i = 0; i < length; i ++) { \
		c_type data; \
		if (!fStream->Read##type(data)) { \
			delete attr->val.array_name; return false; \
		} \
		attr->val.array_name[i] = data; \
	}

bool Disasm::ReadArray(uint8 type, uint32 length, struct ATTRIBUTE* attr) { 
	switch (type) {
		case  HP_UByteArray: READ_ARRAY(ubyte_array, uint8, UByte);
			break;
		case  HP_UInt16Array: READ_ARRAY(uint16_array, uint16, UInt16);
			break;
		case  HP_UInt32Array: READ_ARRAY(uint32_array, uint32, UInt32);
			break;
		case  HP_SInt16Array: READ_ARRAY(sint16_array, int16, SInt16);
			break;
		case  HP_SInt32Array: READ_ARRAY(sint32_array, int32, SInt32);
			break;
		case  HP_Real32Array: READ_ARRAY(real32_array, float, Real32);
			break;
	}
	return true;
}

#define PRINT_ARRAY(array_name, format, c_type, type) \
			printf(array_name "[%u] = \n", (unsigned int)attr->arrayLen); \
			for (uint32 i = 0; i < attr->arrayLen; i ++) { \
				if (i > 0 && i % 16 == 0) printf("\n"); \
				printf(format, (c_type)attr->val.type##_array[i]); \
			} \
			printf(";\n");
	



void Disasm::PrintAttr(struct ATTRIBUTE* attr) {
	switch (attr->Type) {
		case  HP_UByteData: printf("ubyte %d ", (int)attr->val.ubyte);
			break;
		case  HP_UInt16Data: printf("uint16 %u ", (unsigned int)attr->val.uint16);
			break;
		case  HP_UInt32Data: printf("uint32 %u ", (unsigned int)attr->val.uint32);
			break;
		case  HP_SInt16Data: printf("uint16 %d ", (int)attr->val.sint16);
			break;
		case  HP_SInt32Data: printf("sint16 %d ", (int)attr->val.sint32);
			break;
		case  HP_Real32Data:  printf("real32 %f ", attr->val.real32);
			break;

		case  HP_String: fprintf(stderr, "HP_String not implemented!\n");
			break;
		case  HP_UByteArray: PRINT_ARRAY("ubyte_array", "%d ", int, ubyte);
			break;
		case  HP_UInt16Array: PRINT_ARRAY("uint16_array", "%u ", unsigned int, uint16);
			break;
		case  HP_UInt32Array: PRINT_ARRAY("uint32_array", "%u ", unsigned int, uint32);
			break;
		case  HP_SInt16Array: PRINT_ARRAY("sint16_array", "%d ", int, sint16);
			break;
		case  HP_SInt32Array: PRINT_ARRAY("sint32_array", "%d ", int, sint32);
			break;
		case  HP_Real32Array: PRINT_ARRAY("real32_array", "%f ", float, real32);		
			break;

		case  HP_UByteXy:
			printf("ubyte [%d, %d] ", (int)attr->val.UByte_XY.x, (int)attr->val.UByte_XY.y);
			break;
		case  HP_UInt16Xy:
			printf("uint16 [%u, %u] ", (unsigned int)attr->val.UInt16_XY.x, (unsigned int)attr->val.UInt16_XY.y);
			break;
		case  HP_UInt32Xy:
			printf("uint32 [%u, %u] ", (unsigned int)attr->val.UInt32_XY.x, (unsigned int)attr->val.UInt32_XY.y);
			break;
		case  HP_SInt16Xy:
			printf("sint16 [%d, %d] ", (int)attr->val.SInt16_XY.x, (int)attr->val.SInt16_XY.y);
			break;
		case  HP_SInt32Xy:
			printf("sint32 [%d, %d] ", (int)attr->val.SInt32_XY.x, (int)attr->val.SInt32_XY.y);
			break;
		case  HP_Real32Xy:
			printf("real32 [%f, %f] ", (float)attr->val.Real32_XY.x, (float)attr->val.Real32_XY.y);
			break;

		case  HP_UByteBox:
			printf("ubyte [%d, %d, %d, %d] ", (int)attr->val.UByte_BOX.x1, (int)attr->val.UByte_BOX.y1, (int)attr->val.UByte_BOX.x2, (int)attr->val.UByte_BOX.y2);
			break;
		case  HP_UInt16Box:
			printf("uint16 [%u, %u, %u, %u] ", (unsigned int)attr->val.UInt16_BOX.x1, (unsigned int)attr->val.UInt16_BOX.y1, (unsigned int)attr->val.UInt16_BOX.x2, (unsigned int)attr->val.UInt16_BOX.y2);
			break;
		case  HP_UInt32Box:
			printf("uint32 [%u, %u, %u, %u] ", (unsigned int)attr->val.UInt32_BOX.x1, (unsigned int)attr->val.UInt32_BOX.y1, (unsigned int)attr->val.UInt32_BOX.x2, (unsigned int)attr->val.UInt32_BOX.y2);
			break;
		case  HP_SInt16Box:
			printf("sint16 [%d, %d, %d, %d] ", (int)attr->val.SInt16_BOX.x1, (int)attr->val.SInt16_BOX.y1, (int)attr->val.SInt16_BOX.x2, (int)attr->val.SInt16_BOX.y2);
			break;
		case  HP_SInt32Box:
			printf("sint32 [%d, %d, %d, %d] ", (int)attr->val.SInt32_BOX.x1, (int)attr->val.SInt32_BOX.y1, (int)attr->val.SInt32_BOX.x2, (int)attr->val.SInt32_BOX.y2);
			break;
		case  HP_Real32Box:
			printf("real32 [%f, %f, %f, %f] ", (float)attr->val.Real32_BOX.x1, (float)attr->val.Real32_BOX.y1, (float)attr->val.Real32_BOX.x2, (float)attr->val.Real32_BOX.y2);
			break;
	}
}

struct ATTRIBUTE* Disasm::ReadData(uint8 byte) {
	struct ATTRIBUTE attr(0, byte);
	uint32 length;
	switch (byte) {
		case  HP_UByteData:
			if (!fStream->ReadUByte(attr.val.ubyte)) return NULL;
			break;
		case  HP_UInt16Data:
			if (!fStream->ReadUInt16(attr.val.uint16)) return NULL;
			break;
		case  HP_UInt32Data:
			if (!fStream->ReadUInt32(attr.val.uint32)) return NULL;
			break;
		case  HP_SInt16Data:
			if (!fStream->ReadSInt16(attr.val.sint16)) return NULL;
			break;
		case  HP_SInt32Data:
			if (!fStream->ReadSInt32(attr.val.sint32)) return NULL;
			break;
		case  HP_Real32Data:
			if (!fStream->ReadReal32(attr.val.real32)) return NULL;
			break;

		case  HP_String: fprintf(stderr, "HP_String not implemented!\n"); return NULL;
			break;
		case  HP_UByteArray:
		case  HP_UInt16Array:
		case  HP_UInt32Array:
		case  HP_SInt16Array:
		case  HP_SInt32Array:
		case  HP_Real32Array:			
			if (!ReadArrayLength(length) || !ReadArray(byte, length, &attr)) return NULL; 
			break;

		case  HP_UByteXy:
			if (!fStream->ReadUByte(attr.val.UByte_XY.x) || !fStream->ReadUByte(attr.val.UByte_XY.y)) return NULL;
			break;
		case  HP_UInt16Xy:
			if (!fStream->ReadUInt16(attr.val.UInt16_XY.x) || !fStream->ReadUInt16(attr.val.UInt16_XY.y)) return NULL;
			break;
		case  HP_UInt32Xy:
			if (!fStream->ReadUInt32(attr.val.UInt32_XY.x) || !fStream->ReadUInt32(attr.val.UInt32_XY.y)) return NULL;
			break;
		case  HP_SInt16Xy:
			if (!fStream->ReadSInt16(attr.val.SInt16_XY.x) || !fStream->ReadSInt16(attr.val.SInt16_XY.y)) return NULL;
			break;
		case  HP_SInt32Xy:
			if (!fStream->ReadSInt32(attr.val.SInt32_XY.x) || !fStream->ReadSInt32(attr.val.SInt32_XY.y)) return NULL;
			break;
		case  HP_Real32Xy:
			if (!fStream->ReadReal32(attr.val.Real32_XY.x) || !fStream->ReadReal32(attr.val.Real32_XY.y)) return NULL;
			break;

		case  HP_UByteBox:
			if (!fStream->ReadUByte(attr.val.UByte_BOX.x1) || !fStream->ReadUByte(attr.val.UByte_BOX.y1) ||
				!fStream->ReadUByte(attr.val.UByte_BOX.x2) || !fStream->ReadUByte(attr.val.UByte_BOX.y2)) return NULL;
			break;
		case  HP_UInt16Box:
			if (!fStream->ReadUInt16(attr.val.UInt16_BOX.x1) || !fStream->ReadUInt16(attr.val.UInt16_BOX.y1) ||
				!fStream->ReadUInt16(attr.val.UInt16_BOX.x2) || !fStream->ReadUInt16(attr.val.UInt16_BOX.y2)) return NULL;
			break;
		case  HP_UInt32Box:
			if (!fStream->ReadUInt32(attr.val.UInt32_BOX.x1) || !fStream->ReadUInt32(attr.val.UInt32_BOX.y1) ||
				!fStream->ReadUInt32(attr.val.UInt32_BOX.x2) || !fStream->ReadUInt32(attr.val.UInt32_BOX.y2)) return NULL;
			break;
		case  HP_SInt16Box:
			if (!fStream->ReadSInt16(attr.val.SInt16_BOX.x1) || !fStream->ReadSInt16(attr.val.SInt16_BOX.y1) ||
				!fStream->ReadSInt16(attr.val.SInt16_BOX.x2) || !fStream->ReadSInt16(attr.val.SInt16_BOX.y2)) return NULL;
			break;
		case  HP_SInt32Box:
			if (!fStream->ReadSInt32(attr.val.SInt32_BOX.x1) || !fStream->ReadSInt32(attr.val.SInt32_BOX.y1) ||
				!fStream->ReadSInt32(attr.val.SInt32_BOX.x2) || !fStream->ReadSInt32(attr.val.SInt32_BOX.y2)) return NULL;
			break;
		case  HP_Real32Box:
			if (!fStream->ReadReal32(attr.val.Real32_BOX.x1) || !fStream->ReadReal32(attr.val.Real32_BOX.y1) ||
				!fStream->ReadReal32(attr.val.Real32_BOX.x2) || !fStream->ReadReal32(attr.val.Real32_BOX.y2)) return NULL;
			break;
		default:
			fprintf(stderr, "Unknown data tag %d %x\n", (int)byte, (int)byte);
			return NULL;
	}
	struct ATTRIBUTE* pAttr = new struct ATTRIBUTE(0, 0);
	*pAttr = attr;
	return pAttr;
}

bool Disasm::PushData(uint8 byte) {
	struct ATTRIBUTE* attr = ReadData(byte);
	if (attr) {
		PushArg(attr); return true;
	} else {
		Error("Reading data");
		return false;
	}
}

const char* Disasm::AttributeName(uint8 id) {
	switch (id) {
		case HP_CMYColor: return "CMYColor";
		case HP_PaletteDepth: return "PaletteDepth";
		case HP_ColorSpace: return "ColorSpace";
		case HP_NullBrush: return "NullBrush";
		case HP_NullPen: return "NullPen";
		case HP_PaletteData: return "PaletteData";
		case HP_PaletteIndex: return "PaletteIndex";
		case HP_PatternSelectID: return "PatternSelectID";
		case HP_GrayLevel: return "GrayLevel";

		case HP_RGBColor: return "RGBColor";
		case HP_PatternOrigin: return "PatternOrigin";
		case HP_NewDestinationSize: return "NewDestinationSize";

		case HP_PrimaryArray: return "PrimaryArray";
		case HP_PrimaryDepth: return "PrimaryDepth";
		case HP_ColorimetricColorSpace: return "ColorimetricColorSpace";
		case HP_XYChromaticities: return "XYChromaticities";
		case HP_WhitePointReference: return "WhitePointReference";
		case HP_CRGBMinMax: return "CRGBMinMax";
		case HP_GammaGain: return "GammaGain";


		case HP_DeviceMatrix: return "DeviceMatrix";
		case HP_DitherMatrixDataType: return "DitherMatrixDataType";
		case HP_DitherOrigin: return "DitherOrigin";
		case HP_MediaDest: return "MediaDest";
		case HP_MediaSize: return "MediaSize";
		case HP_MediaSource: return "MediaSource";
		case HP_MediaType: return "MediaType";
		case HP_Orientation: return "Orientation";
		case HP_PageAngle: return "PageAngle";
		case HP_PageOrigin: return "PageOrigin";
		case HP_PageScale: return "PageScale";
		case HP_ROP3: return "ROP3";
		case HP_TxMode: return "TxMode";
		case HP_CustomMediaSize: return "CustomMediaSize";
		case HP_CustomMediaSizeUnits: return "CustomMediaSizeUnits";
		case HP_PageCopies: return "PageCopies";
		case HP_DitherMatrixSize: return "DitherMatrixSize";
		case HP_DitherMatrixDepth: return "DitherMatrixDepth";
		case HP_SimplexPageMode: return "SimplexPageMode";
		case HP_DuplexPageMode: return "DuplexPageMode";
		case HP_DuplexPageSide: return "DuplexPageSide";

		case HP_LineStartCapStyle: return "LineStartCapStyle";
		case HP_LineEndCapStyle: return "LineEndCapStyle";
		case HP_ArcDirection: return "ArcDirection";
		case HP_BoundingBox: return "BoundingBox";
		case HP_DashOffset: return "DashOffset";
		case HP_EllipseDimension: return "EllipseDimension";
		case HP_EndPoint: return "EndPoint";
		case HP_FillMode: return "FillMode";
		case HP_LineCapStyle: return "LineCapStyle";
		case HP_LineJoinStyle: return "LineJoinStyle";
		case HP_MiterLength: return "MiterLength";
		case HP_LineDashStyle: return "LineDashStyle";
		case HP_PenWidth: return "PenWidth";
		case HP_Point: return "Point";
		case HP_NumberOfPoints: return "NumberOfPoints";
		case HP_SolidLine: return "SolidLine";
		case HP_StartPoint: return "StartPoint";
		case HP_PointType: return "PointType";
		case HP_ControlPoint1: return "ControlPoint1";
		case HP_ControlPoint2: return "ControlPoint2";
		case HP_ClipRegion: return "ClipRegion";
		case HP_ClipMode: return "ClipMode";


		case HP_ColorDepthArray: return "ColorDepthArray";
		case HP_ColorDepth: return "ColorDepth";
		case HP_BlockHeight: return "BlockHeight";
		case HP_ColorMapping: return "ColorMapping";
		case HP_CompressMode: return "CompressMode";
		case HP_DestinationBox: return "DestinationBox";
		case HP_DestinationSize: return "DestinationSize";
		case HP_PatternPersistence: return "PatternPersistence";
		case HP_PatternDefineID: return "PatternDefineID";
		case HP_SourceHeight: return "SourceHeight";
		case HP_SourceWidth: return "SourceWidth";
		case HP_StartLine: return "StartLine";
		case HP_PadBytesMultiple: return "PadBytesMultiple";
		case HP_BlockByteLength: return "BlockByteLength";
		case HP_NumberOfScanLines: return "NumberOfScanLines";

		case HP_ColorTreatment: return "ColorTreatment";
		case HP_FileName: return "FileName";
		case HP_BackgroundName: return "BackgroundName";
		case HP_FormName: return "FormName";
		case HP_FormType: return "FormType";
		case HP_FormSize: return "FormSize";
		case HP_UDLCName: return "UDLCName";

		case HP_CommentData: return "CommentData";
		case HP_DataOrg: return "DataOrg";
		case HP_Measure: return "Measure";
		case HP_SourceType: return "SourceType";
		case HP_UnitsPerMeasure: return "UnitsPerMeasure";
		case HP_QueryKey: return "QueryKey";
		case HP_StreamName: return "StreamName";
		case HP_StreamDataLength: return "StreamDataLength";

		case HP_ErrorReport: return "ErrorReport";
		case HP_IOReadTimeOut: return "IOReadTimeOut";

		case HP_WritingMode: return "WritingMode";

		case HP_VUExtension: return "VUExtension";
		case HP_VUDataLength: return "VUDataLength";

		case HP_VUAttr1: return "VUAttr1";
		case HP_VUAttr2: return "VUAttr2";
		case HP_VUAttr3: return "VUAttr3";
		case HP_VUAttr4: return "VUAttr4";
		case HP_VUAttr5: return "VUAttr5";
		case HP_VUAttr6: return "VUAttr6";
		case HP_VUAttr7: return "VUAttr7";
		case HP_VUAttr8: return "VUAttr8";
		case HP_VUAttr9: return "VUAttr9";
		case HP_VUAttr10: return "VUAttr10";
		case HP_VUAttr11: return "VUAttr11";
		case HP_VUAttr12: return "VUAttr12";

//		case HP_PassThroughCommand: return "PassThroughCommand";
		case HP_PassThroughArray: return "PassThroughArray";
		case HP_Diagnostics: return "Diagnostics";
		case HP_CharAngle: return "CharAngle";
		case HP_CharCode: return "CharCode";
		case HP_CharDataSize: return "CharDataSize";
		case HP_CharScale: return "CharScale";
		case HP_CharShear: return "CharShear";
		case HP_CharSize: return "CharSize";
		case HP_FontHeaderLength: return "FontHeaderLength";
		case HP_FontName: return "FontName";
		case HP_FontFormat: return "FontFormat";
		case HP_SymbolSet: return "SymbolSet";
		case HP_TextData: return "TextData";
		case HP_CharSubModeArray: return "CharSubModeArray";
//		case HP_WritingMode: return "WritingMode";
		case HP_BitmapCharScale: return "BitmapCharScale";
		case HP_XSpacingData: return "XSpacingData";
		case HP_YSpacingData: return "YSpacingData";
		case HP_CharBoldValue: return "CharBoldValue";
	}
	return "Unknown Attribute";
}


static AttrValue gEnableEnum[] = {
	{HP_eOn, "eOn"}, 
	{HP_eOff, "eOff"} 
};

static AttrValue gBooleanEnum[] = {
	{HP_eFalse, "eFalse"},
	{HP_eTrue, "eTrue"} 
};

static AttrValue gWriteEnum[] = {
	{HP_eWriteHorizontal, "eWriteHorizontal"}, 
	{HP_eWriteVertical, "eWriteVertical"} 
};


static AttrValue gBitmapCharScalingEnum[] = {
	{HP_eDisable, "eDisable"}, 
	{HP_eEnable, "eEnable"} 
};

static AttrValue gUnitOfMeasureEnum[] = {
	{HP_eInch, "eInch"}, 
	{HP_eMillimeter, "eMillimeter"}, 
	{HP_eTenthsOfAMillimeter, "eTenthsOfAMillimeter"} 
};

static AttrValue gErrorReportingEnum[] = {
	{HP_eNoReporting, "eNoReporting"}, 
	{HP_eBackChannel, "eBackChannel"}, 
	{HP_eErrorPage, "eErrorPage"}, 
	{HP_eBackChAndErrPage, "eBackChAndErrPage"}, 
	{HP_eBackChanAndErrPage, "eBackChanAndErrPage"}, 
	{HP_eNWBackChannel, "eNWBackChannel"}, 
	{HP_eNWErrorPage, "eNWErrorPage"}, 
	{HP_eNWBackChAndErrPage, "eNWBackChAndErrPage"} 
};

static AttrValue gDataOrganizationEnum[] = {
	 {HP_eBinaryHighByteFirst, "eBinaryHighByteFirst"},
	 {HP_eBinaryLowByteFirst, "eBinaryLowByteFirst"}
};

static AttrValue gDuplexPageModeEnum[] = {
	 {HP_eDuplexHorizontalBinding, "eDuplexHorizontalBinding"},
	 {HP_eDuplexVerticalBinding, "eDuplexVerticalBinding"}
};

static AttrValue gDuplexPageSideEnum[] = {
	 {HP_eFrontMediaSide, "eFrontMediaSide"},
	 {HP_eBackMediaSide, "eBackMediaSide"}
};

static AttrValue gSimplexPageModeEnum[] = {
	 {HP_eSimplexFrontSide, "eSimplexFrontSide"}
};

static AttrValue gOrientationEnum[] = {
	 {HP_ePortraitOrientation, "ePortraitOrientation"},
	 {HP_eLandscapeOrientation, "eLandscapeOrientation"},
	 {HP_eReversePortrait, "eReversePortrait"},
	 {HP_eReverseLandscape, "eReverseLandscape"}
};

static AttrValue gMediaSizeEnum[] = {
	 {HP_eLetterPaper, "eLetterPaper"},
	 {HP_eLegalPaper, "eLegalPaper"},
	 {HP_eA4Paper, "eA4Paper"},
	 {HP_eExecPaper, "eExecPaper"},
	 {HP_eLedgerPaper, "eLedgerPaper"},
	 {HP_eA3Paper, "eA3Paper"},
	 {HP_eCOM10Envelope, "eCOM10Envelope"},
	 {HP_eMonarchEnvelope, "eMonarchEnvelope"},
	 {HP_eC5Envelope, "eC5Envelope"},
	 {HP_eDLEnvelope, "eDLEnvelope"},
	 {HP_eJB4Paper, "eJB4Paper"},
	 {HP_eJB5Paper, "eJB5Paper"},
	 {HP_eB5Envelope, "eB5Envelope"},
	 {HP_eB5Paper, "eB5Paper"},
	 {HP_eJPostcard, "eJPostcard"},
	 {HP_eJDoublePostcard, "eJDoublePostcard"},
	 {HP_eA5Paper, "eA5Paper"},
	 {HP_eA6Paper, "eA6Paper"},
	 {HP_eJB6Paper, "eJB6Paper"},
	 {HP_eJIS8KPaper, "eJIS8KPaper"},
	 {HP_eJIS16KPaper, "eJIS16KPaper"},
	 {HP_eJISExecPaper, "eJISExecPaper"}
};

static AttrValue gMediaSourceEnum[] = {
	 {HP_eDefaultSource, "eDefaultSource"},
	 {HP_eAutoSelect, "eAutoSelect"},
	 {HP_eManualFeed, "eManualFeed"},
	 {HP_eMultiPurposeTray, "eMultiPurposeTray"},
	 {HP_eUpperCassette, "eUpperCassette"},
	 {HP_eLowerCassette, "eLowerCassette"},
	 {HP_eEnvelopeTray, "eEnvelopeTray"},
	 {HP_eThirdCassette, "eThirdCassette"}
};

static AttrValue gMediaDestinationEnum[] = {
	 {HP_eDefaultBin, "eDefaultBin"},
	 {HP_eFaceDownBin, "eFaceDownBin"},
	 {HP_eFaceUpBin, "eFaceUpBin"},
	 {HP_eJobOffsetBin, "eJobOffsetBin"}
};

static AttrValue gCompressionEnum[] = {
	 {HP_eNoCompression, "eNoCompression"},
	 {HP_eRLECompression, "eRLECompression"},
	 {HP_eJPEGCompression, "eJPEGCompression"},
	 {HP_eDeltaRowCompression, "eDeltaRowCompression"}
};

static AttrValue gArcDirectionEnum[] = {
	 {HP_eClockWise, "eClockWise"},
	 {HP_eCounterClockWise, "eCounterClockWise"}
};

static AttrValue gFillModeEnum[] = {
	 {HP_eNonZeroWinding, "eNonZeroWinding"},
	 {HP_eEvenOdd, "eEvenOdd"}
};

static AttrValue gLineEndEnum[] = {
	 {HP_eButtCap, "eButtCap"},
	 {HP_eRoundCap, "eRoundCap"},
	 {HP_eSquareCap, "eSquareCap"},
	 {HP_eTriangleCap, "eTriangleCap"}
//	 {HP_eButtEnd, "eButtEnd"},
//	 {HP_eRoundEnd, "eRoundEnd"},
//	 {HP_eSquareEnd, "eSquareEnd"},
//	 {HP_eTriangleEnd, "eTriangleEnd"}
};

static AttrValue gCharSubModeEnum[] = {
	 {HP_eNoSubstitution, "eNoSubstitution"},
	 {HP_eVerticalSubstitution, "eVerticalSubstitution"}
};

static AttrValue gLineJoinEnum[] = {
	 {HP_eMiterJoin, "eMiterJoin"},
	 {HP_eRoundJoin, "eRoundJoin"},
	 {HP_eBevelJoin, "eBevelJoin"},
	 {HP_eNoJoin, "eNoJoin"}
};

static AttrValue gDitherMatrixEnum[] = {
	 {HP_eDeviceBest, "eDeviceBest"},
	 {HP_eDeviceIndependent, "eDeviceIndependent"}
};

static AttrValue gDataSourceEnum[] = {
	 {HP_eDefaultDataSource, "eDefaultDataSource"}
};

static AttrValue gColorSpaceEnum[] = {
	 {HP_eBiLevel, "eBiLevel"},
	 {HP_eGray, "eGray"},
	 {HP_eRGB, "eRGB"},
	 {HP_eCMY, "eCMY"},
	 {HP_eCIELab, "eCIELab"},
	 {HP_eCRGB, "eCRGB"},
	 {HP_eSRGB, "eSRGB"}
};

static AttrValue gColorDepthEnum[] = {
	 {HP_e1Bit, "e1Bit"},
	 {HP_e4Bit, "e4Bit"},
	 {HP_e8Bit, "e8Bit"}
};

static AttrValue gColorMappingEnum[] = {
	 {HP_eDirectPixel, "eDirectPixel"},
	 {HP_eIndexedPixel, "eIndexedPixel"},
	 {HP_eDirectPlane, "eDirectPlane"}
};

static AttrValue gDiagnosticEnum[] = {
	 {HP_eNoDiag, "eNoDiag"},
	 {HP_eFilterDiag, "eFilterDiag"},
	 {HP_eCommandsDiag, "eCommandsDiag"},
	 {HP_ePersonalityDiag, "ePersonalityDiag"},
	 {HP_ePageDiag, "ePageDiag"}
};

static AttrValue gClipModeEnum[] = {
	 {HP_eInterior, "eInterior"},
	 {HP_eExterior, "eExterior"}
};

static AttrValue gDataTypeEnum[] = {
	 {HP_eUByte, "eUByte"},
	 {HP_eSByte, "eSByte"},
	 {HP_eUInt16, "eUInt16"},
	 {HP_eSInt16, "eSInt16"},
	 {HP_eReal32, "eReal32"}
};

static AttrValue gPatternPersistenceEnum[] = {
	 {HP_eTempPattern, "eTempPattern"},
	 {HP_ePagePattern, "ePagePattern"},
	 {HP_eSessionPattern, "eSessionPattern"}
};

static AttrValue gTransparancyEnum[] = {
	 {HP_eOpaque, "eOpaque"},
	 {HP_eTransparent, "eTransparent"}
};

static AttrValue gFormTypeEnum[] = {
	 {HP_eQuality, "eQuality"},
	 {HP_ePerformance, "ePerformance"},
	 {HP_eStatic, "eStatic"},
	 {HP_eBackground, "eBackground"}
};

static AttrValue gColorEnum[] = {
	 {HP_eNoTreatment, "eNoTreatment"},
	 {HP_eVivid, "eVivid"},
	 {HP_eScreenMatch, "eScreenMatch"}
};

void Disasm::PrintAttributeValue(uint8 id, const AttrValue* table, int size) {
	if (NumOfArgs() == 1 && ArgAt(0)->Type == HP_UByteData) {
		uint8 value = ArgAt(0)->val.ubyte;
		for (int i = 0; i < size; i ++, table ++) {
			if (table->value == value) {
				if (fVerbose) PrintAttr(ArgAt(0));
				printf("%s %s\n", table->name, AttributeName(id));
				return;
			}
		}
	}
	GenericPrintAttribute(id);
}

void Disasm::GenericPrintAttribute(uint8 id) {
	for (int i = 0; i < NumOfArgs(); i ++) {
		PrintAttr(ArgAt(i));
	}
	printf(" %s\n", AttributeName(id));
}

#define ATTR_ENUM(name) name, NUM_OF_ELEMS(name, AttrValue)

bool Disasm::DecodeAttribute(uint8 byte) {
	if (byte == HP_8BitAttrId) {
		uint8 id;
		if (!fStream->ReadUByte(id)) {
			Error("Could not read attribute id");
			return false;
		}
		switch (id) {
			case HP_ArcDirection:
				PrintAttributeValue(id, ATTR_ENUM(gArcDirectionEnum));
				break;
			case HP_CharSubModeArray:
				PrintAttributeValue(id, ATTR_ENUM(gCharSubModeEnum));
				break;
			case HP_ClipMode:
				PrintAttributeValue(id, ATTR_ENUM(gClipModeEnum));
				break;
			case HP_ClipRegion:
				PrintAttributeValue(id, ATTR_ENUM(gClipModeEnum));
				break;
			case HP_ColorDepth:
				PrintAttributeValue(id, ATTR_ENUM(gColorDepthEnum));
				break;
			case HP_ColorMapping:
				PrintAttributeValue(id, ATTR_ENUM(gColorMappingEnum));
				break;
			case HP_ColorSpace:
				PrintAttributeValue(id, ATTR_ENUM(gColorSpaceEnum));
				break;
			case HP_CompressMode:
				PrintAttributeValue(id, ATTR_ENUM(gCompressionEnum));
				break;
			case HP_DataOrg:
				PrintAttributeValue(id, ATTR_ENUM(gDataOrganizationEnum));
				break;
			case HP_SourceType:
				PrintAttributeValue(id, ATTR_ENUM(gDataSourceEnum));
				break;
			// DataType?
			case HP_DitherMatrixDataType:
				PrintAttributeValue(id, ATTR_ENUM(gDataTypeEnum));
				break;
			case HP_DuplexPageMode:
				PrintAttributeValue(id, ATTR_ENUM(gDuplexPageModeEnum));
				break;
			case HP_DuplexPageSide:
				PrintAttributeValue(id, ATTR_ENUM(gDuplexPageSideEnum));
				break;
			case HP_ErrorReport:
				PrintAttributeValue(id, ATTR_ENUM(gErrorReportingEnum));
				break;
			case HP_FillMode:
				PrintAttributeValue(id, ATTR_ENUM(gFillModeEnum));
				break;
			case HP_LineStartCapStyle:
				PrintAttributeValue(id, ATTR_ENUM(gLineEndEnum));
				break;
			case HP_LineEndCapStyle:
				PrintAttributeValue(id, ATTR_ENUM(gLineEndEnum));
				break;
			case HP_LineJoinStyle:
				PrintAttributeValue(id, ATTR_ENUM(gLineJoinEnum));
				break;
			case HP_Measure:
				PrintAttributeValue(id, ATTR_ENUM(gUnitOfMeasureEnum));
				break;
			case HP_MediaSize:
				PrintAttributeValue(id, ATTR_ENUM(gMediaSizeEnum));
				break;
			case HP_MediaSource:
				PrintAttributeValue(id, ATTR_ENUM(gMediaSourceEnum));
				break;
			case HP_MediaDest:
				PrintAttributeValue(id, ATTR_ENUM(gMediaDestinationEnum));
				break;
			case HP_Orientation:
				PrintAttributeValue(id, ATTR_ENUM(gOrientationEnum));
				break;
			case HP_PatternPersistence:
				PrintAttributeValue(id, ATTR_ENUM(gPatternPersistenceEnum));
				break;
			// Symbolset?			
			case HP_SimplexPageMode:
				PrintAttributeValue(id, ATTR_ENUM(gSimplexPageModeEnum));
				break;
			case HP_TxMode:
				PrintAttributeValue(id, ATTR_ENUM(gTransparancyEnum));
				break;
			case HP_WritingMode:
				PrintAttributeValue(id, ATTR_ENUM(gWriteEnum));
				break;
			case HP_FormType:
				PrintAttributeValue(id, ATTR_ENUM(gFormTypeEnum));
				break;
			case HP_LineCapStyle:
				PrintAttributeValue(id, ATTR_ENUM(gLineEndEnum));
				break;
			case HP_Diagnostics:
				PrintAttributeValue(id, ATTR_ENUM(gDiagnosticEnum));
				break;
						
			case HP_MediaType:
			case HP_UnitsPerMeasure: 
			case HP_CMYColor:
			case HP_PaletteDepth:
			case HP_NullBrush:
			case HP_NullPen:
			case HP_PaletteData:
			case HP_PaletteIndex:
			case HP_PatternSelectID:
			case HP_GrayLevel:

			case HP_RGBColor:
			case HP_PatternOrigin:
			case HP_NewDestinationSize:

			case HP_PrimaryArray:
			case HP_PrimaryDepth:
			case HP_ColorimetricColorSpace:
			case HP_XYChromaticities:
			case HP_WhitePointReference:
			case HP_CRGBMinMax:
			case HP_GammaGain:


			case HP_DeviceMatrix:
			case HP_DitherOrigin:
			case HP_PageAngle:
			case HP_PageOrigin:
			case HP_PageScale:
			case HP_ROP3:
			case HP_CustomMediaSize:
			case HP_CustomMediaSizeUnits:
			case HP_PageCopies:
			case HP_DitherMatrixSize:
			case HP_DitherMatrixDepth:

			case HP_BoundingBox:
			case HP_DashOffset:
			case HP_EllipseDimension:
			case HP_EndPoint:
			case HP_MiterLength:
			case HP_LineDashStyle:
			case HP_PenWidth:
			case HP_Point:
			case HP_NumberOfPoints:
			case HP_SolidLine:
			case HP_StartPoint:
			case HP_PointType:
			case HP_ControlPoint1:
			case HP_ControlPoint2:


			case HP_ColorDepthArray:
			case HP_BlockHeight:
			case HP_DestinationBox:
			case HP_DestinationSize:
			case HP_PatternDefineID:
			case HP_SourceHeight:
			case HP_SourceWidth:
			case HP_StartLine:
			case HP_PadBytesMultiple:
			case HP_BlockByteLength:
			case HP_NumberOfScanLines:

			case HP_ColorTreatment:
			case HP_FileName:
			case HP_BackgroundName:
			case HP_FormName:
			case HP_FormSize:
			case HP_UDLCName:

			case HP_CommentData:
			case HP_QueryKey:
			case HP_StreamName:
			case HP_StreamDataLength:

			case HP_IOReadTimeOut:


/* Generic VI attributes */

			case HP_VUExtension:
			case HP_VUDataLength:

			case HP_VUAttr1:
			case HP_VUAttr2:
			case HP_VUAttr3:
			case HP_VUAttr4:
			case HP_VUAttr5:
			case HP_VUAttr6:
			case HP_VUAttr7:
			case HP_VUAttr8:
			case HP_VUAttr9:
			case HP_VUAttr10:
			case HP_VUAttr11:
			case HP_VUAttr12:

//			case HP_PassThroughCommand:
			case HP_PassThroughArray:
			case HP_CharAngle:
			case HP_CharCode:
			case HP_CharDataSize:
			case HP_CharScale:
			case HP_CharShear:
			case HP_CharSize:
			case HP_FontHeaderLength:
			case HP_FontName:
			case HP_FontFormat:
			case HP_SymbolSet:
			case HP_TextData:
//			case HP_WritingMode:
			case HP_BitmapCharScale:
			case HP_XSpacingData:
			case HP_YSpacingData:
			case HP_CharBoldValue:
				GenericPrintAttribute(id);
				break;
			default:
				fprintf(stderr, "Unsupported attribute id %d %2.2x\nContinue...\n", (int)id, (int)id);
		}
		return true;
	} else {
		fprintf(stderr, "Unsupported attribute tag %d %2.2x\n", (int)byte, (int)byte);
	}
	return false;
}

bool Disasm::DecodeEmbedData(uint8 byte) {
	uint32 length;
	if (byte == HP_EmbeddedData) {
		if (!fStream->ReadUInt32(length)) {
			Error("Could not read length of dataLength");
			return false;
		}
	} else {
		uint8 type;
		if (!fStream->ReadUByte(type)) {
			Error("Could not read type tag of length of dataLengthByte");
			return false;
		}
		struct ATTRIBUTE* attr = ReadData(type);
		if (attr == NULL) {
			Error("Could not read length of dataLengthByte");
			return false;
		}
		if (attr->Type == HP_UByteData) {
			length = attr->val.ubyte;
		} else if (attr->Type == HP_UInt32Data) {
			length = attr->val.uint32;
		} else {
			fprintf(stderr, "Unsupported datatype %d %x for embed data\n", (int)type, (int)type);
			DeleteAttr(attr); return false;
		}
		DeleteAttr(attr);
	}
	if (byte == HP_EmbeddedData) {
		printf("dataLength");
	} else if (byte == HP_EmbeddedDataByte) {
		printf("dataLengthByte");
	}	
	printf(" size = %u [\n", (unsigned int)length);
	for (uint32 i = 0; i < length; i ++) {
		if (i > 0 && i % 16 == 15) printf("\n");
		uint8 data;
		if (!fStream->ReadUByte(data)) {
			Error("Out of data for dataLength[Byte]\n");
			return false;
		}
		printf("%2.2x ", (int)data);
	}
	printf("\n]\n");
	return true;
}

bool Disasm::ParsePCL6() {
	uint8 byte;
	while (fStream->ReadUByte(byte)) {
		if (IsOperator(byte)) {
			if (!DecodeOperator(byte)) return false;
		} else if (IsDataType(byte)) {
			if (!PushData(byte)) return false;
		} else if (IsAttribute(byte)) {
			bool ok = DecodeAttribute(byte);
			ClearAttrs();
			if (!ok) return false;
		} else if (IsEmbedData(byte)) {
			if (!DecodeEmbedData(byte)) return false;
		} else if (IsWhiteSpace(byte)) {
			// nothing to do
		} else if (IsEsc(byte)) {
			return true;
		} else {
			Error("Unknown byte in input stream");
			return false;
		}
	}
	return true;
}

// main entry

int main(int argc, char* argv[]) {
	const char* program = argv[0];
	if (argc >= 2) {
		int i = 1;
		bool verbose = false;
		if (strcmp(argv[1], "-v") == 0) { verbose = true; i ++; }
		if (argc > i) { 
			const char* filename = argv[i];
			File file(filename);
			if (file.InitCheck()) {
				Disasm disasm(&file);
				disasm.SetVerbose(verbose);
				disasm.Print();
				return 0;
			} else {
				fprintf(stderr, "%s error: could not open file '%s'\n", program, filename);
				return 0;
			}
		}
	}
	fprintf(stderr, "%s [-v] pcl6_filename\n", program);
}
