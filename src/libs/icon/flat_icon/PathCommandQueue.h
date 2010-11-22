/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef PATH_COMMAND_QUEUE_H
#define PATH_COMMAND_QUEUE_H


#include "IconBuild.h"
#include "LittleEndianBuffer.h"


class BPoint;


_BEGIN_ICON_NAMESPACE


class VectorPath;

class PathCommandQueue {
 public:
								PathCommandQueue();
	virtual						~PathCommandQueue();

			bool				Write(LittleEndianBuffer& buffer,
									  const VectorPath* path,
									  uint8 pointCount);

			bool				Read(LittleEndianBuffer& buffer,
									 VectorPath* path,
									 uint8 pointCount);

 private:
	// writing
			bool				_AppendHLine(float x);
			bool				_AppendVLine(float y);
			bool				_AppendLine(const BPoint& point);
			bool				_AppendCurve(const BPoint& point,
											 const BPoint& pointIn,
											 const BPoint& pointOut);

			bool				_AppendCommand(uint8 command);

	// reading
			bool				_ReadCommand(uint8& command);

			LittleEndianBuffer	fCommandBuffer;
			LittleEndianBuffer	fPointBuffer;

			uint8				fCommandByte;
			uint8				fCommandPos;

			uint8				fCommandCount;
};


_END_ICON_NAMESPACE


#endif	// PATH_COMMAND_QUEUE_H
