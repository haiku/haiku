/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */

#include <DataIO.h>
#include <Point.h>
#include <Rect.h>

#include <PictureDataWriter.h>
#include <PictureProtocol.h>

PictureDataWriter::PictureDataWriter(BPositionIO *data)
	:
	fData(data)
{
}


status_t
PictureDataWriter::WriteSetOrigin(const BPoint &point)
{
	BeginOp(B_PIC_SET_ORIGIN);
	Write<BPoint>(point);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteInvertRect(const BRect &rect)
{
	WriteSetDrawingMode(B_OP_INVERT);
		
	BeginOp(B_PIC_FILL_RECT);
	Write<BRect>(rect);
	EndOp();

	WriteSetDrawingMode(B_OP_COPY);
	return B_OK;
}


status_t
PictureDataWriter::WriteSetDrawingMode(const drawing_mode &mode)
{
	BeginOp(B_PIC_SET_DRAWING_MODE);
	Write<int16>((int16)mode);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteSetPenSize(const float &penSize)
{
	BeginOp(B_PIC_SET_PEN_SIZE);
	Write<float>(penSize);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteSetLineMode(const cap_mode &cap, const join_mode &join, const float &miterLimit)
{
	BeginOp(B_PIC_SET_LINE_MODE);
	Write<int16>((int16)cap);
	Write<int16>((int16)join);
	Write<float>(miterLimit);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteSetScale(const float &scale)
{
	BeginOp(B_PIC_SET_SCALE);
	Write<float>(scale);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteSetHighColor(const rgb_color &color)
{
	BeginOp(B_PIC_SET_FORE_COLOR);
	Write<rgb_color>(color);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteSetLowColor(const rgb_color &color)
{
	BeginOp(B_PIC_SET_BACK_COLOR);
	Write<rgb_color>(color);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawRect(const BRect &rect, const bool &fill)
{
	BeginOp(fill ? B_PIC_FILL_RECT : B_PIC_STROKE_RECT);
	Write<BRect>(rect);	
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawRoundRect(const BRect &rect, const BPoint &radius, const bool &fill)
{
	BeginOp(fill ? B_PIC_FILL_ROUND_RECT : B_PIC_STROKE_ROUND_RECT);
	Write<BRect>(rect);
	Write<BPoint>(radius);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawEllipse(const BRect &rect, const bool &fill)
{
	BeginOp(fill ? B_PIC_FILL_ELLIPSE : B_PIC_STROKE_ELLIPSE);
	Write<BRect>(rect);	
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawArc(const BPoint &center, const BPoint &radius,
				const float &startTheta, const float &arcTheta, const bool &fill)
{
	BeginOp(fill ? B_PIC_FILL_ARC : B_PIC_STROKE_ARC);
	Write<BPoint>(center);
	Write<BPoint>(radius);
	Write<float>(startTheta);
	Write<float>(arcTheta);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteStrokeLine(const BPoint &start, const BPoint &end)
{
	BeginOp(B_PIC_STROKE_LINE);
	Write<BPoint>(start);
	Write<BPoint>(end);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawString(const BPoint &where, const char *string,
				   const int32 &length, const escapement_delta &escapement)
{
	BeginOp(B_PIC_SET_PEN_LOCATION);
	Write<BPoint>(where);
	EndOp();
		
	BeginOp(B_PIC_DRAW_STRING);
	Write<int32>(length);
	WriteData(string, length);
	Write<float>(escapement.space);
	Write<float>(escapement.nonspace);
	EndOp();

	return B_OK;
}


status_t
PictureDataWriter::WriteDrawShape(const int32 &opCount, const void *opList,
				const int32 &ptCount, const void *ptList, const bool &fill)
{
	BeginOp(fill ? B_PIC_FILL_SHAPE : B_PIC_STROKE_SHAPE);
	Write<int32>(opCount);
	WriteData(opList, opCount * sizeof(uint32));
	Write<int32>(ptCount);
	WriteData(ptList, ptCount * sizeof(BPoint));
	EndOp();
	
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawBitmap(const BRect &srcRect, const BRect &dstRect, const int32 &width, const int32 &height,
				const int32 &bytesPerRow, const int32 &colorSpace, const int32 &flags,
				const void *data, const int32 &length)
{
	BeginOp(B_PIC_DRAW_PIXELS);
	Write<BRect>(srcRect);
	Write<BRect>(dstRect);
	Write<int32>(width);
	Write<int32>(height);
	Write<int32>(bytesPerRow);
	Write<int32>(colorSpace);
	Write<int32>(flags);
	Write<int32>(length);
	WriteData(data, length);
	EndOp();
	return B_OK;
}


status_t
PictureDataWriter::WritePushState()
{
	BeginOp(B_PIC_PUSH_STATE);
	EndOp();
	return B_OK;
}



status_t
PictureDataWriter::WritePopState()
{
	BeginOp(B_PIC_POP_STATE);
	EndOp();
	return B_OK;
}


// private
status_t
PictureDataWriter::BeginOp(const int16 &op)
{
	fStack.push(fData->Position());
	fData->Write(&op, sizeof(op));
	
	// Init the size of the opcode block to 0
	size_t size = 0;
	fData->Write(&size, sizeof(size));
	return B_OK;
}


status_t
PictureDataWriter::EndOp()
{
	off_t curPos = fData->Position();
	off_t stackPos = fStack.top();
	fStack.pop();
	
	// The size of the op is calculated like this:
	// current position on the stream minus the position on the stack,
	// minus the space occupied by the op code itself (int16)
	// and the space occupied by the size field (size_t) 
	size_t size = curPos - stackPos - sizeof(size_t) - sizeof(int16);

	// Size was set to 0 in BeginOp(). Now we overwrite it with the correct value
	fData->Seek(stackPos + sizeof(int16), SEEK_SET);
	fData->Write(&size, sizeof(size));
	fData->Seek(curPos, SEEK_SET);
	return B_OK;
}


status_t
PictureDataWriter::WriteData(const void *data, size_t size)
{
	return fData->Write(data, size);
}
