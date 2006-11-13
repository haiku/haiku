/*
 * Copyright 2006, Haiku Inc.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stefano Ceccherini (burton666@libero.it)
 */
 
#ifndef __PICTUREDATAWRITER_H
#define __PICTUREDATAWRITER_H

#include <InterfaceDefs.h>
#include <Font.h>
#include <OS.h>

#include <stack>

class BPositionIO;
class PictureDataWriter {
public:
	PictureDataWriter(BPositionIO *data);
	
	status_t WriteSetOrigin(const BPoint &point);
	status_t WriteInvertRect(const BRect &rect);
	
	status_t WriteSetDrawingMode(const drawing_mode &mode);
	status_t WriteSetPenSize(const float &penSize);
	status_t WriteSetLineMode(const cap_mode &cap, const join_mode &join, const float &miterLimit);	
	status_t WriteSetScale(const float &scale);

	status_t WriteDrawRect(const BRect &rect, const bool &fill);
	status_t WriteDrawRoundRect(const BRect &rect, const BPoint &radius, const bool &fill);
	status_t WriteDrawEllipse(const BRect &rect, const bool &fill);
	status_t WriteDrawArc(const BPoint &center, const BPoint &radius,
				const float &startTheta, const float &arcTheta, const bool &fill);
	status_t WriteStrokeLine(const BPoint &start, const BPoint &end);
	
	status_t WriteSetHighColor(const rgb_color &color);
	status_t WriteSetLowColor(const rgb_color &color);
	
	status_t WriteDrawString(const BPoint &where, const char *string,
				 const int32 &length, const escapement_delta &delta);
	status_t WriteDrawShape(const int32 &opCount, const void *opList,
				const int32 &ptCount, const void *ptList, const bool &fill);
	status_t WriteDrawBitmap(const BRect &srcRect, const BRect &dstRect, const int32 &width, const int32 &height,
				const int32 &bytesPerRow, const int32 &colorSpace, const int32 &flags,
				const void *data, const int32 &length);
	
	status_t WritePushState();
	status_t WritePopState();	
	
private:
	BPositionIO *fData;
	std::stack<off_t> fStack;

	status_t WriteData(const void *data, size_t size);
	template <typename T> status_t Write(const T &data) { return WriteData(&data, sizeof(data)); }
	
	status_t BeginOp(const int16 &op);
	status_t EndOp();	
	
};

#endif // __PICTUREDATAWRITER_H
