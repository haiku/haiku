/*
 * Copyright 2009-2019, Haiku, Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz <mmlr@mlotz.ch>
 */

#include "RemoteMessage.h"

#ifndef CLIENT_COMPILE
#include "DrawState.h"
#include "ServerBitmap.h"
#include "ServerCursor.h"
#endif

#include <Bitmap.h>
#include <Font.h>
#include <View.h>

#include <Gradient.h>
#include <GradientLinear.h>
#include <GradientRadial.h>
#include <GradientRadialFocus.h>
#include <GradientDiamond.h>
#include <GradientConic.h>

#include <new>


#ifdef CLIENT_COMPILE
#define TRACE_ALWAYS(x...)		printf("RemoteMessage: " x)
#else
#define TRACE_ALWAYS(x...)		debug_printf("RemoteMessage: " x)
#endif

#define TRACE(x...)				/*TRACE_ALWAYS(x)*/
#define TRACE_ERROR(x...)		TRACE_ALWAYS(x)


status_t
RemoteMessage::NextMessage(uint16& code)
{
	if (fDataLeft > 0) {
		// discard remainder of message
		int32 readSize = fSource->Read(NULL, fDataLeft);
		if (readSize < 0) {
			TRACE_ERROR("failed to read from source: %s\n", strerror(readSize));
			return readSize;
		}
	}

	static const uint32 kHeaderSize = sizeof(uint16) + sizeof(uint32);

	fDataLeft = kHeaderSize;
	status_t result = Read(code);
	if (result != B_OK) {
		TRACE_ERROR("failed to read message code: %s\n", strerror(result));
		return result;
	}

	uint32 dataLeft;
	result = Read(dataLeft);
	if (result != B_OK) {
		TRACE_ERROR("failed to read message length: %s\n", strerror(result));
		return result;
	}

	if (dataLeft < kHeaderSize) {
		TRACE_ERROR("message claims %" B_PRIu32 " bytes, needed at least %"
			B_PRIu32 " for the header\n", dataLeft, kHeaderSize);
		return B_ERROR;
	}

	fDataLeft = dataLeft - kHeaderSize;
	fCode = code;
	return B_OK;
}


void
RemoteMessage::Cancel()
{
	fAvailable += fWriteIndex;
	fWriteIndex = 0;
}


#ifndef CLIENT_COMPILE
void
RemoteMessage::AddBitmap(const ServerBitmap& bitmap, bool minimal)
{
	Add(bitmap.Width());
	Add(bitmap.Height());
	Add(bitmap.BytesPerRow());

	if (!minimal) {
		Add(bitmap.ColorSpace());
		Add(bitmap.Flags());
	}

	uint32 bitsLength = bitmap.BitsLength();
	Add(bitsLength);

	if (!_MakeSpace(bitsLength))
		return;

	memcpy(fBuffer + fWriteIndex, bitmap.Bits(), bitsLength);
	fWriteIndex += bitsLength;
	fAvailable -= bitsLength;
}


void
RemoteMessage::AddFont(const ServerFont& font)
{
	Add((uint8)font.Direction());
	Add((uint8)font.Encoding());
	Add(font.Flags());
	Add((uint8)font.Spacing());
	Add(font.Shear());
	Add(font.Rotation());
	Add(font.FalseBoldWidth());
	Add(font.Size());
	Add(font.Face());
	Add(font.GetFamilyAndStyle());
}


void
RemoteMessage::AddDrawState(const DrawState& drawState)
{
	Add(drawState.PenSize());
	Add(drawState.SubPixelPrecise());
	Add(drawState.GetDrawingMode());
	Add(drawState.AlphaSrcMode());
	Add(drawState.AlphaFncMode());
	AddPattern(drawState.GetPattern());
	Add(drawState.LineCapMode());
	Add(drawState.LineJoinMode());
	Add(drawState.MiterLimit());
	Add(drawState.HighColor());
	Add(drawState.LowColor());
}


void
RemoteMessage::AddArrayLine(const ViewLineArrayInfo& line)
{
	Add(line.startPoint);
	Add(line.endPoint);
	Add(line.color);
}


void
RemoteMessage::AddCursor(const ServerCursor& cursor)
{
	Add(cursor.GetHotSpot());
	AddBitmap(cursor);
}


void
RemoteMessage::AddPattern(const Pattern& pattern)
{
	Add(pattern.GetPattern());
}

#else // !CLIENT_COMPILE

void
RemoteMessage::AddBitmap(const BBitmap& bitmap)
{
	BRect bounds = bitmap.Bounds();
	Add(bounds.IntegerWidth() + 1);
	Add(bounds.IntegerHeight() + 1);
	Add(bitmap.BytesPerRow());
	Add((uint32)bitmap.ColorSpace());
	Add(bitmap.Flags());

	uint32 bitsLength = bitmap.BitsLength();
	Add(bitsLength);

	if (!_MakeSpace(bitsLength))
		return;

	memcpy(fBuffer + fWriteIndex, bitmap.Bits(), bitsLength);
	fWriteIndex += bitsLength;
	fAvailable -= bitsLength;
}
#endif // !CLIENT_COMPILE


void
RemoteMessage::AddGradient(const BGradient& gradient)
{
	Add((uint32)gradient.GetType());

	switch (gradient.GetType()) {
		case BGradient::TYPE_NONE:
			break;

		case BGradient::TYPE_LINEAR:
		{
			const BGradientLinear* linear
				= dynamic_cast<const BGradientLinear *>(&gradient);
			if (linear == NULL)
				return;

			Add(linear->Start());
			Add(linear->End());
			break;
		}

		case BGradient::TYPE_RADIAL:
		{
			const BGradientRadial* radial
				= dynamic_cast<const BGradientRadial *>(&gradient);
			if (radial == NULL)
				return;

			Add(radial->Center());
			Add(radial->Radius());
			break;
		}

		case BGradient::TYPE_RADIAL_FOCUS:
		{
			const BGradientRadialFocus* radialFocus
				= dynamic_cast<const BGradientRadialFocus *>(&gradient);
			if (radialFocus == NULL)
				return;

			Add(radialFocus->Center());
			Add(radialFocus->Focal());
			Add(radialFocus->Radius());
			break;
		}

		case BGradient::TYPE_DIAMOND:
		{
			const BGradientDiamond* diamond
				= dynamic_cast<const BGradientDiamond *>(&gradient);
			if (diamond == NULL)
				return;

			Add(diamond->Center());
			break;
		}

		case BGradient::TYPE_CONIC:
		{
			const BGradientConic* conic
				= dynamic_cast<const BGradientConic *>(&gradient);
			if (conic == NULL)
				return;

			Add(conic->Center());
			Add(conic->Angle());
			break;
		}
	}

	int32 stopCount = gradient.CountColorStops();
	Add(stopCount);

	for (int32 i = 0; i < stopCount; i++) {
		BGradient::ColorStop* stop = gradient.ColorStopAt(i);
		if (stop == NULL)
			return;

		Add(stop->color);
		Add(stop->offset);
	}
}


void
RemoteMessage::AddTransform(const BAffineTransform& transform)
{
	bool isIdentity = transform.IsIdentity();
	Add(isIdentity);

	if (isIdentity)
		return;

	Add(transform.sx);
	Add(transform.shy);
	Add(transform.shx);
	Add(transform.sy);
	Add(transform.tx);
	Add(transform.ty);
}


status_t
RemoteMessage::ReadString(char** _string, size_t& _length)
{
	uint32 length;
	status_t result = Read(length);
	if (result != B_OK)
		return result;

	if (length > fDataLeft)
		return B_ERROR;

	char *string = (char *)malloc(length + 1);
	if (string == NULL)
		return B_NO_MEMORY;

	int32 readSize = fSource->Read(string, length);
	if (readSize < 0) {
		free(string);
		return readSize;
	}

	if ((uint32)readSize != length) {
		free(string);
		return B_ERROR;
	}

	fDataLeft -= readSize;

	string[length] = 0;
	*_string = string;
	_length = length;
	return B_OK;
}


status_t
RemoteMessage::ReadBitmap(BBitmap** _bitmap, bool minimal,
	color_space colorSpace, uint32 flags)
{
	uint32 bitsLength;
	int32 width, height, bytesPerRow;

	Read(width);
	Read(height);
	Read(bytesPerRow);

	if (!minimal) {
		Read(colorSpace);
		Read(flags);
	}

	Read(bitsLength);

	if (bitsLength > fDataLeft)
		return B_ERROR;

#ifndef CLIENT_COMPILE
	flags = B_BITMAP_NO_SERVER_LINK;
#endif

	BBitmap *bitmap = new(std::nothrow) BBitmap(
		BRect(0, 0, width - 1, height - 1), flags, colorSpace, bytesPerRow);
	if (bitmap == NULL)
		return B_NO_MEMORY;

	status_t result = bitmap->InitCheck();
	if (result != B_OK) {
		delete bitmap;
		return result;
	}

	if (bitmap->BitsLength() < (int32)bitsLength) {
		delete bitmap;
		return B_ERROR;
	}

	int32 readSize = fSource->Read(bitmap->Bits(), bitsLength);
	if ((uint32)readSize != bitsLength) {
		delete bitmap;
		return readSize < 0 ? readSize : B_ERROR;
	}

	fDataLeft -= readSize;
	*_bitmap = bitmap;
	return B_OK;
}


status_t
RemoteMessage::ReadFontState(BFont& font)
{
	uint8 direction;
	uint8 encoding;
	uint8 spacing;
	uint16 face;
	uint32 flags, familyAndStyle;
	float falseBoldWidth, rotation, shear, size;

	Read(direction);
	Read(encoding);
	Read(flags);
	Read(spacing);
	Read(shear);
	Read(rotation);
	Read(falseBoldWidth);
	Read(size);
	Read(face);
	status_t result = Read(familyAndStyle);
	if (result != B_OK)
		return result;

	font.SetFamilyAndStyle(familyAndStyle);
	font.SetEncoding(encoding);
	font.SetFlags(flags);
	font.SetSpacing(spacing);
	font.SetShear(shear);
	font.SetRotation(rotation);
	font.SetFalseBoldWidth(falseBoldWidth);
	font.SetSize(size);
	font.SetFace(face);
	return B_OK;
}


status_t
RemoteMessage::ReadViewState(BView& view, ::pattern& pattern)
{
	bool subPixelPrecise;
	float penSize, miterLimit;
	drawing_mode drawingMode;
	source_alpha sourceAlpha;
	alpha_function alphaFunction;
	cap_mode capMode;
	join_mode joinMode;
	rgb_color highColor, lowColor;

	Read(penSize);
	Read(subPixelPrecise);
	Read(drawingMode);
	Read(sourceAlpha);
	Read(alphaFunction);
	Read(pattern);
	Read(capMode);
	Read(joinMode);
	Read(miterLimit);
	Read(highColor);
	status_t result = Read(lowColor);
	if (result != B_OK)
		return result;

	uint32 flags = view.Flags() & ~B_SUBPIXEL_PRECISE;
	view.SetFlags(flags | (subPixelPrecise ? B_SUBPIXEL_PRECISE : 0));
	view.SetPenSize(penSize);
	view.SetDrawingMode(drawingMode);
	view.SetBlendingMode(sourceAlpha, alphaFunction);
	view.SetLineMode(capMode, joinMode, miterLimit);
	view.SetHighColor(highColor);
	view.SetLowColor(lowColor);
	return B_OK;
}


status_t
RemoteMessage::ReadGradient(BGradient** _gradient)
{
	BGradient::Type type;
	Read(type);

	BGradient *gradient = NULL;
	switch (type) {
		case BGradient::TYPE_NONE:
			break;

		case BGradient::TYPE_LINEAR:
		{
			BPoint start, end;

			Read(start);
			Read(end);

			gradient = new(std::nothrow) BGradientLinear(start, end);
			break;
		}

		case BGradient::TYPE_RADIAL:
		{
			BPoint center;
			float radius;

			Read(center);
			Read(radius);

			gradient = new(std::nothrow) BGradientRadial(center, radius);
			break;
		}

		case BGradient::TYPE_RADIAL_FOCUS:
		{
			BPoint center, focal;
			float radius;

			Read(center);
			Read(focal);
			Read(radius);

			gradient = new(std::nothrow) BGradientRadialFocus(center, radius,
				focal);
			break;
		}

		case BGradient::TYPE_DIAMOND:
		{
			BPoint center;

			Read(center);

			gradient = new(std::nothrow) BGradientDiamond(center);
			break;
		}

		case BGradient::TYPE_CONIC:
		{
			BPoint center;
			float angle;

			Read(center);
			Read(angle);

			gradient = new(std::nothrow) BGradientConic(center, angle);
			break;
		}
	}

	if (gradient == NULL)
		return B_NO_MEMORY;

	int32 stopCount;
	status_t result = Read(stopCount);
	if (result != B_OK) {
		delete gradient;
		return result;
	}

	for (int32 i = 0; i < stopCount; i++) {
		rgb_color color;
		float offset;

		Read(color);
		result = Read(offset);
		if (result != B_OK) {
			delete gradient;
			return result;
		}

		gradient->AddColor(color, offset);
	}

	*_gradient = gradient;
	return B_OK;
}


status_t
RemoteMessage::ReadTransform(BAffineTransform& transform)
{
	bool isIdentity;
	status_t result = Read(isIdentity);
	if (result != B_OK)
		return result;

	if (isIdentity) {
		transform = BAffineTransform();
		return B_OK;
	}

	Read(transform.sx);
	Read(transform.shy);
	Read(transform.shx);
	Read(transform.sy);
	Read(transform.tx);
	return Read(transform.ty);
}


status_t
RemoteMessage::ReadArrayLine(BPoint& startPoint, BPoint& endPoint,
	rgb_color& color)
{
	Read(startPoint);
	Read(endPoint);
	return Read(color);
}
