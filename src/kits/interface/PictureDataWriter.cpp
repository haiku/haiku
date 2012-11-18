/*
 * Copyright 2006-2009, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (stefano.ceccherini@gmail.com)
 */

#include <PictureDataWriter.h>

#include <stdio.h>
#include <string.h>

#include <DataIO.h>
#include <Point.h>
#include <Rect.h>
#include <Region.h>

#include <PictureProtocol.h>

#define THROW_ERROR(error) throw (status_t)(error)

// TODO: Review writing of strings. AFAIK in the picture data format
// They are not supposed to be NULL terminated
// (at least, it's not mandatory) so we should write their size too.
PictureDataWriter::PictureDataWriter()
	:
	fData(NULL)
{
}


PictureDataWriter::PictureDataWriter(BPositionIO *data)
	:
	fData(data)
{
}


PictureDataWriter::~PictureDataWriter()
{
}


status_t
PictureDataWriter::SetTo(BPositionIO *data)
{
	if (data == NULL)
		return B_BAD_VALUE;
	fData = data;
	return B_OK;
}


status_t
PictureDataWriter::WriteSetOrigin(const BPoint &point)
{
	try {
		BeginOp(B_PIC_SET_ORIGIN);
		Write<BPoint>(point);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteInvertRect(const BRect &rect)
{
	try {
		WriteSetDrawingMode(B_OP_INVERT);
			
		BeginOp(B_PIC_FILL_RECT);
		Write<BRect>(rect);
		EndOp();

		WriteSetDrawingMode(B_OP_COPY);
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetDrawingMode(const drawing_mode &mode)
{
	try {
		BeginOp(B_PIC_SET_DRAWING_MODE);
		Write<int16>((int16)mode);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetPenLocation(const BPoint &point)
{
	try {
		BeginOp(B_PIC_SET_PEN_LOCATION);
		Write<BPoint>(point);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetPenSize(const float &penSize)
{
	try {
		BeginOp(B_PIC_SET_PEN_SIZE);
		Write<float>(penSize);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetLineMode(const cap_mode &cap,
	const join_mode &join, const float &miterLimit)
{
	try {
		BeginOp(B_PIC_SET_LINE_MODE);
		Write<int16>((int16)cap);
		Write<int16>((int16)join);
		Write<float>(miterLimit);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetScale(const float &scale)
{
	try {
		BeginOp(B_PIC_SET_SCALE);
		Write<float>(scale);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetPattern(const pattern &pat)
{
	try {
		BeginOp(B_PIC_SET_STIPLE_PATTERN);
		Write<pattern>(pat);
		EndOp();
	} catch (status_t &status) {
		return status;
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetClipping(const BRegion &region)
{
	// TODO: I don't know if it's compatible with R5's BPicture version
	try {
		const int32 numRects = region.CountRects();
		if (numRects > 0 && region.Frame().IsValid()) {	
			BeginOp(B_PIC_SET_CLIPPING_RECTS);
			Write<uint32>(numRects); 
			for (int32 i = 0; i < numRects; i++) {
				Write<BRect>(region.RectAt(i));			
			}			
			EndOp();
		} else
			WriteClearClipping();
	} catch (status_t &status) {
		return status;
	}

	return B_OK;
}


status_t
PictureDataWriter::WriteClearClipping()
{
	try {
		BeginOp(B_PIC_CLEAR_CLIPPING_RECTS);
		EndOp();
	} catch (status_t &status) {
		return status;
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetHighColor(const rgb_color &color)
{
	try {
		BeginOp(B_PIC_SET_FORE_COLOR);
		Write<rgb_color>(color);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetLowColor(const rgb_color &color)
{
	try {
		BeginOp(B_PIC_SET_BACK_COLOR);
		Write<rgb_color>(color);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawRect(const BRect &rect,
	const bool &fill)
{
	try {
		BeginOp(fill ? B_PIC_FILL_RECT : B_PIC_STROKE_RECT);
		Write<BRect>(rect);	
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawRoundRect(const BRect &rect,
	const BPoint &radius, const bool &fill)
{
	try {
		BeginOp(fill ? B_PIC_FILL_ROUND_RECT : B_PIC_STROKE_ROUND_RECT);
		Write<BRect>(rect);
		Write<BPoint>(radius);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawEllipse(const BRect &rect, const bool &fill)
{
	try {
		BeginOp(fill ? B_PIC_FILL_ELLIPSE : B_PIC_STROKE_ELLIPSE);
		Write<BRect>(rect);	
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawArc(const BPoint &center,
	const BPoint &radius, const float &startTheta,
	const float &arcTheta, const bool &fill)
{
	try {
		BeginOp(fill ? B_PIC_FILL_ARC : B_PIC_STROKE_ARC);
		Write<BPoint>(center);
		Write<BPoint>(radius);
		Write<float>(startTheta);
		Write<float>(arcTheta);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawPolygon(const int32 &numPoints,
	BPoint *points, const bool &isClosed, const bool &fill)
{
	try {
		BeginOp(fill ? B_PIC_FILL_POLYGON : B_PIC_STROKE_POLYGON);
		Write<int32>(numPoints);
		for (int32 i = 0; i < numPoints; i++)
			Write<BPoint>(points[i]);
		if (!fill)
			Write<uint8>((uint8)isClosed);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawBezier(const BPoint points[4],
	const bool &fill)
{
	try {
		BeginOp(fill ? B_PIC_FILL_BEZIER : B_PIC_STROKE_BEZIER);
		for (int32 i = 0; i < 4; i++)
			Write<BPoint>(points[i]);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteStrokeLine(const BPoint &start,
	const BPoint &end)
{
	try {
		BeginOp(B_PIC_STROKE_LINE);
		Write<BPoint>(start);
		Write<BPoint>(end);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawString(const BPoint &where,
	const char *string, const int32 &length,
	const escapement_delta &escapement)
{
	try {
		BeginOp(B_PIC_SET_PEN_LOCATION);
		Write<BPoint>(where);
		EndOp();
			
		BeginOp(B_PIC_DRAW_STRING);
		Write<float>(escapement.space);
		Write<float>(escapement.nonspace);
		//WriteData(string, length + 1);
			// TODO: is string 0 terminated? why is length given?
		WriteData(string, length);
		Write<uint8>(0);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}

	return B_OK;
}


status_t
PictureDataWriter::WriteDrawShape(const int32 &opCount,
	const void *opList, const int32 &ptCount, const void *ptList,
	const bool &fill)
{
	try {
		BeginOp(fill ? B_PIC_FILL_SHAPE : B_PIC_STROKE_SHAPE);
		Write<int32>(opCount);
		Write<int32>(ptCount);
		WriteData(opList, opCount * sizeof(uint32));
		WriteData(ptList, ptCount * sizeof(BPoint));
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawBitmap(const BRect &srcRect,
	const BRect &dstRect, const int32 &width, const int32 &height,
	const int32 &bytesPerRow, const int32 &colorSpace,
	const int32 &flags, const void *data, const int32 &length)
{
	if (length != height * bytesPerRow)
		debugger("PictureDataWriter::WriteDrawBitmap: invalid length");
	try {	
		BeginOp(B_PIC_DRAW_PIXELS);
		Write<BRect>(srcRect);
		Write<BRect>(dstRect);
		Write<int32>(width);
		Write<int32>(height);
		Write<int32>(bytesPerRow);
		Write<int32>(colorSpace);
		Write<int32>(flags);
		WriteData(data, length);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteDrawPicture(const BPoint &where,
	const int32 &token)
{
	// TODO: I'm not sure about this function. I think we need
	// to attach the picture data too.
	// The token won't be sufficient in many cases (for example, when
	// we archive/flatten the picture.
	try {
		BeginOp(B_PIC_DRAW_PICTURE);
		Write<BPoint>(where);
		Write<int32>(token);	
		EndOp();
	} catch (status_t &status) {
		return status;
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetFontFamily(const font_family family)
{
	try {
		BeginOp(B_PIC_SET_FONT_FAMILY);
		WriteData(family, strlen(family));
		Write<uint8>(0);
		EndOp();
	} catch (status_t &status) {
		return status;
	}	
	return B_OK;
}


status_t
PictureDataWriter::WriteSetFontStyle(const font_style style)
{
	try {
		BeginOp(B_PIC_SET_FONT_STYLE);
		WriteData(style, strlen(style));
		Write<uint8>(0);
		EndOp();
	} catch (status_t &status) {
		return status;
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetFontSpacing(const int32 &spacing)
{
	try {
		BeginOp(B_PIC_SET_FONT_SPACING);
		Write<int32>(spacing);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetFontSize(const float &size)
{
	try {
		BeginOp(B_PIC_SET_FONT_SIZE);
		Write<float>(size);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetFontRotation(const float &rotation)
{
	try {
		BeginOp(B_PIC_SET_FONT_ROTATE);
		Write<float>(rotation);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetFontEncoding(const int32 &encoding)
{
	try {
		BeginOp(B_PIC_SET_FONT_ENCODING);
		Write<int32>(encoding);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetFontFlags(const int32 &flags)
{
	try {
		BeginOp(B_PIC_SET_FONT_FLAGS);
		Write<int32>(flags);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetFontShear(const float &shear)
{
	try {
		BeginOp(B_PIC_SET_FONT_SHEAR);
		Write<float>(shear);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WriteSetFontFace(const int32 &face)
{
	try {
		BeginOp(B_PIC_SET_FONT_FACE);
		Write<int32>(face);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WritePushState()
{
	try {
		BeginOp(B_PIC_PUSH_STATE);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


status_t
PictureDataWriter::WritePopState()
{
	try {
		BeginOp(B_PIC_POP_STATE);
		EndOp();
	} catch (status_t &status) {
		return status;	
	}
	return B_OK;
}


// private
void
PictureDataWriter::BeginOp(const int16 &op)
{
	if (fData == NULL)
		THROW_ERROR(B_NO_INIT);

	fStack.push(fData->Position());
	fData->Write(&op, sizeof(op));
	
	// Init the size of the opcode block to 0
	int32 size = 0;
	fData->Write(&size, sizeof(size));
}


void
PictureDataWriter::EndOp()
{
	if (fData == NULL)
		THROW_ERROR(B_NO_INIT);

	off_t curPos = fData->Position();
	off_t stackPos = fStack.top();
	fStack.pop();
	
	// The size of the op is calculated like this:
	// current position on the stream minus the position on the stack,
	// minus the space occupied by the op code itself (int16)
	// and the space occupied by the size field (int32)
	int32 size = curPos - stackPos - sizeof(int32) - sizeof(int16);

	// Size was set to 0 in BeginOp()
	// Now we overwrite it with the correct value
	fData->Seek(stackPos + sizeof(int16), SEEK_SET);
	fData->Write(&size, sizeof(size));
	fData->Seek(curPos, SEEK_SET);
}


void
PictureDataWriter::WriteData(const void *data, size_t size)
{
	ssize_t result = fData->Write(data, size);
	if (result < 0)
		THROW_ERROR(result);
	if ((size_t)result != size)
		THROW_ERROR(B_IO_ERROR); 
}
