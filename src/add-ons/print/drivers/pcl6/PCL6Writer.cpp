/* 
** PCL6Writer.cpp
** Copyright 2005, Michael Pfeiffer, laplace@users.sourceforge.net.
** All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include "PCL6Writer.h"

#include <ByteOrder.h>
#include <String.h>

#define BYTE_AT(lvalue, index) (((uint8*)(&lvalue))[index])


PCL6Writer::PCL6Writer(PCL6WriterStream* stream, uint32 bufferSize)
	:
	fStream(stream),
	fBuffer(new uint8[bufferSize]),
	fSize(bufferSize),
	fIndex(0)
{
}


PCL6Writer::~PCL6Writer()
{
	delete[] fBuffer;
	fBuffer = NULL;
}


// throws TransportException
void 
PCL6Writer::Append(uint8 value)
{
	if (fIndex == fSize)
		Flush();
	fBuffer[fIndex] = value;
	fIndex ++;
}


void 
PCL6Writer::Flush()
{
	if (fIndex > 0) {
		fStream->Write(fBuffer, fIndex);
		fIndex = 0;
	}
}


void 
PCL6Writer::Append(int16 value)
{
	int16 v = B_HOST_TO_LENDIAN_INT16(value);
	Append(BYTE_AT(v, 0));  
	Append(BYTE_AT(v, 1));  
}


void 
PCL6Writer::Append(uint16 value)
{
	int16 v = B_HOST_TO_LENDIAN_INT16(value);
	Append(BYTE_AT(v, 0));  
	Append(BYTE_AT(v, 1));  
}


void 
PCL6Writer::Append(int32 value)
{
	int32 v = B_HOST_TO_LENDIAN_INT32(value);
	Append(BYTE_AT(v, 0));  
	Append(BYTE_AT(v, 1));  
	Append(BYTE_AT(v, 2));  
	Append(BYTE_AT(v, 3));  
}


void 
PCL6Writer::Append(uint32 value)
{
	int32 v = B_HOST_TO_LENDIAN_INT32(value);
	Append(BYTE_AT(v, 0));  
	Append(BYTE_AT(v, 1));  
	Append(BYTE_AT(v, 2));  
	Append(BYTE_AT(v, 3));  
}


void 
PCL6Writer::Append(float value)
{
	float v = B_HOST_TO_LENDIAN_FLOAT(value);
	Append(BYTE_AT(v, 0));  
	Append(BYTE_AT(v, 1));  
	Append(BYTE_AT(v, 2));  
	Append(BYTE_AT(v, 3));  
}


void 
PCL6Writer::Append(const uint8* data, uint32 size)
{
	for (uint32 i = 0; i < size; i++) {
		Append(data[i]);
	}
}


void 
PCL6Writer::AppendString(const char* string)
{
	uint8 ch = *string;
	while (ch != 0) {
		Append(ch);
		string ++;
		ch = *string;
	}
}


void 
PCL6Writer::AppendOperator(Operator op)
{
	Append((uint8)op);
}


void 
PCL6Writer::AppendAttribute(Attribute attr)
{
	AppendDataTag(k8BitAttrId);
	Append((uint8)attr);
}


void 
PCL6Writer::AppendDataTag(DataTag tag)
{
	Append((uint8)tag);
}


void 
PCL6Writer::AppendData(uint8 value)
{
	AppendDataTag(kUByteData);
	Append(value);
}


void 
PCL6Writer::AppendData(int16 value)
{
	AppendDataTag(kSInt16Data);
	Append(value);
}


void 
PCL6Writer::AppendData(uint16 value)
{
	AppendDataTag(kUInt16Data);
	Append(value);
}


void 
PCL6Writer::AppendData(int32 value)
{
	AppendDataTag(kSInt32Data);
	Append(value);
}


void 
PCL6Writer::AppendData(uint32 value)
{
	AppendDataTag(kUInt32Data);
	Append(value);
}


void 
PCL6Writer::AppendData(float value)
{
	AppendDataTag(kReal32Data);
	Append(value);
}


void 
PCL6Writer::EmbeddedDataPrefix(uint32 size)
{
	if (size < 256) {
		AppendDataTag(kEmbeddedDataByte);
		Append((uint8)size);
	} else {
		AppendDataTag(kEmbeddedData);
		Append(size);
	}
}


void 
PCL6Writer::EmbeddedDataPrefix32(uint32 size)
{
	AppendDataTag(kEmbeddedData);
	Append(size);
}


void 
PCL6Writer::AppendDataXY(uint8 x, uint8 y)
{
	AppendDataTag(kUByteXY);
	Append(x); Append(y);
}


void 
PCL6Writer::AppendDataXY(int16 x, int16 y)
{
	AppendDataTag(kSInt16XY);
	Append(x); Append(y);
}


void 
PCL6Writer::AppendDataXY(uint16 x, uint16 y)
{
	AppendDataTag(kUInt16XY);
	Append(x); Append(y);
}


void 
PCL6Writer::AppendDataXY(int32 x, int32 y)
{
	AppendDataTag(kSInt32XY);
	Append(x); Append(y);
}


void 
PCL6Writer::AppendDataXY(uint32 x, uint32 y)
{
	AppendDataTag(kUInt32XY);
	Append(x); Append(y);
}


void 
PCL6Writer::AppendDataXY(float x, float y)
{
	AppendDataTag(kReal32XY);
	Append(x); Append(y);
}


void
PCL6Writer::PJLHeader(ProtocolClass protocolClass, int dpi, const char* comment)
{
	BString string;
	AppendString("\033%-12345X@PJL JOB\n@PJL SET RESOLUTION=");
	string << dpi;
	AppendString(string.String());
	AppendString("\n@PJL ENTER LANGUAGE=PCLXL\n) HP-PCL XL;");

	const char* pc = "";
	switch (protocolClass) {
		case kProtocolClass1_1:
			pc = "1;1;";
			break;
		case kProtocolClass2_0:
			pc = "2;0;";
			break;
		case kProtocolClass2_1:
			pc = "2;1;";
			break;
		case kProtocolClass3_0:
			pc = "3;0;";
			break;
	}
	AppendString(pc);
	
	if (comment != NULL) {
		AppendString("Comment ");
		AppendString(comment);
	}
	
	AppendString("\n");
}


void
PCL6Writer::PJLFooter()
{
	AppendString("\033%-12345X@PJL EOJ\n\033%-12345X");
}


void 
PCL6Writer::BeginSession(uint16 xres, uint16 yres, UnitOfMeasure unitOfMeasure,
	ErrorReporting errorReporting)
{
	AppendDataXY(xres, yres);
	AppendAttribute(kUnitsPerMeasure);

	AppendData((uint8)unitOfMeasure);
	AppendAttribute(kMeasure);

	AppendData((uint8)errorReporting);
	AppendAttribute(kErrorReport);

	AppendOperator(kBeginSession);
}


void 
PCL6Writer::EndSession()
{
	AppendOperator(kEndSession);
}


void 
PCL6Writer::OpenDataSource()
{
	AppendData((uint8)kDefaultDataSource);
	AppendAttribute(kSourceType);

	AppendData((uint8)kBinaryLowByteFirst);
	AppendAttribute(kDataOrg);

	AppendOperator(kOpenDataSource);
}


void 
PCL6Writer::CloseDataSource()
{
	AppendOperator(kCloseDataSource);
}


void 
PCL6Writer::BeginPage(Orientation orientation, MediaSize mediaSize,
	MediaSource mediaSource)
{
	AppendData((uint8)orientation);
	AppendAttribute(kOrientation);

	AppendData((uint8)mediaSize);
	AppendAttribute(kMediaSize);
	
	AppendData((uint8)mediaSource);
	AppendAttribute(kMediaSource);

	AppendOperator(kBeginPage);	
}


void 
PCL6Writer::BeginPage(Orientation orientation, MediaSize mediaSize,
	MediaSource mediaSource, DuplexPageMode duplexPageMode, MediaSide mediaSide)
{
	AppendData((uint8)orientation);
	AppendAttribute(kOrientation);

	AppendData((uint8)mediaSize);
	AppendAttribute(kMediaSize);
	
	AppendData((uint8)mediaSource);
	AppendAttribute(kMediaSource);

	AppendData((uint8)duplexPageMode);
	AppendAttribute(kDuplexPageMode);
	
	AppendData((uint8)mediaSide);
	AppendAttribute(kDuplexPageSide);
	
	AppendOperator(kBeginPage);	
}


void 
PCL6Writer::EndPage(uint16 copies)
{
//	if (copies != 1) {
		AppendData(copies);
		AppendAttribute(kPageCopies);
//	}
	
	AppendOperator(kEndPage);
}


void 
PCL6Writer::SetPageOrigin(int16 x, int16 y)
{
	AppendDataXY(x, y);
	AppendAttribute(kPageOrigin);
	
	AppendOperator(kSetPageOrigin);
}


void 
PCL6Writer::SetColorSpace(ColorSpace colorSpace)
{
	AppendData((uint8)colorSpace);
	AppendAttribute(kColorSpace);
	
	AppendOperator(kSetColorSpace);
}


void 
PCL6Writer::SetPaintTxMode(Transparency transparency)
{
	AppendData((uint8)transparency);
	AppendAttribute(kTxMode);
	
	AppendOperator(kSetPaintTxMode);
}


void 
PCL6Writer::SetSourceTxMode(Transparency transparency)
{
	AppendData((uint8)transparency);
	AppendAttribute(kTxMode);
	
	AppendOperator(kSetSourceTxMode);
}


void 
PCL6Writer::SetROP(uint8 rop)
{
	AppendData((uint8)rop);
	AppendAttribute(kROP3);
	
	AppendOperator(kSetROP);
}


void 
PCL6Writer::SetCursor(int16 x, int16 y)
{
	AppendDataXY(x, y);
	AppendAttribute(kPoint);
	
	AppendOperator(kSetCursor);
}


void
PCL6Writer::BeginImage(ColorMapping colorMapping, ColorDepth colorDepth,
	uint16 sourceWidth, uint16 sourceHeight, uint16 destWidth,
	uint16 destHeight)
{
	AppendData((uint8)colorMapping);
	AppendAttribute(kColorMapping);
	
	AppendData((uint8)colorDepth);
	AppendAttribute(kColorDepth);
	
	AppendData(sourceWidth);
	AppendAttribute(kSourceWidth);
	
	AppendData(sourceHeight);
	AppendAttribute(kSourceHeight);
	
	AppendDataXY(destWidth, destHeight);
	AppendAttribute(kDestinationSize);
	
	AppendOperator(kBeginImage);
}


void 
PCL6Writer::ReadImage(Compression compression, uint16 startLine,
	uint16 blockHeight, uint8 padBytes)
{
	AppendData(startLine);
	AppendAttribute(kStartLine);
	
	AppendData(blockHeight);
	AppendAttribute(kBlockHeight);
	
	AppendData((uint8)compression);
	AppendAttribute(kCompressMode);
	
	if (padBytes < 1 || padBytes > 4) {
		// XXX throw exception
		return;
	}
	if (padBytes != 4) {
		AppendData((uint8)padBytes);
		AppendAttribute(kPadBytesMultiple);
	}
	
	AppendOperator(kReadImage);
}


void 
PCL6Writer::EndImage()
{
	AppendOperator(kEndImage);
}
