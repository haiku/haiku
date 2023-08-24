/*
 * Copyright 2006, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */

#include "PathCommandQueue.h"

#include <stdio.h>

#include <Point.h>

#include "FlatIconFormat.h"
#include "VectorPath.h"

_USING_ICON_NAMESPACE;


// constructor
PathCommandQueue::PathCommandQueue()
	: fCommandBuffer(),
	  fPointBuffer(),

	  fCommandByte(0),
	  fCommandPos(0),

	  fCommandCount(0)
{
}

// destructor
PathCommandQueue::~PathCommandQueue()
{
}

// Write
bool
PathCommandQueue::Write(LittleEndianBuffer& buffer,
						const VectorPath* path, uint8 pointCount)
{
	// reset
	fCommandCount = 0;
	fCommandByte = 0;
	fCommandPos = 0;
	fCommandBuffer.Reset();
	fPointBuffer.Reset();

	BPoint last(B_ORIGIN);

	for (uint32 p = 0; p < pointCount; p++) {
		BPoint point;
		BPoint pointIn;
		BPoint pointOut;
		if (!path->GetPointsAt(p, point, pointIn, pointOut))
			return false;

		if (point == pointIn && point == pointOut) {
			// single point is sufficient
			if (point.x == last.x) {
				// vertical line
				if (!_AppendVLine(point.y))
					return false;
			} else if (point.y == last.y) {
				// horizontal line
				if (!_AppendHLine(point.x))
					return false;
			} else {
				// line
				if (!_AppendLine(point))
					return false;
			}
		} else {
			// needing to write all three points
			if (!_AppendCurve(point, pointIn, pointOut))
				return false;
		}
	
		last = point;
	}

	if (fCommandPos > 0) {
		// the last couple commands have not been written
		if (!fCommandBuffer.Write(fCommandByte))
			return false;
	}

	return buffer.Write(fCommandBuffer) && buffer.Write(fPointBuffer);
}

// Read
bool
PathCommandQueue::Read(LittleEndianBuffer& buffer,
					   VectorPath* path, uint8 pointCount)
{
	// reset
	fCommandCount = 0;
	fCommandByte = 0;
	fCommandPos = 0;

	fCommandBuffer.Reset();

	// NOTE: fPointBuffer is not used for reading
	// we read the command buffer and then use the
	// buffer directly for the coords

	// read command buffer
	uint8 commandBufferSize = (pointCount + 3) / 4;
	if (!buffer.Read(fCommandBuffer, commandBufferSize))
		return false;

	BPoint last(B_ORIGIN);
	for (uint32 p = 0; p < pointCount; p++) {

		uint8 command;
		if (!_ReadCommand(command))
			return false;

		BPoint point;
		BPoint pointIn;
		BPoint pointOut;

		switch (command) {
			case PATH_COMMAND_H_LINE:
				if (!read_coord(buffer, point.x))
					return false;
				point.y = last.y;
				pointIn = point;
				pointOut = point;
				break;
			case PATH_COMMAND_V_LINE:
				if (!read_coord(buffer, point.y))
					return false;
				point.x = last.x;
				pointIn = point;
				pointOut = point;
				break;
			case PATH_COMMAND_LINE:
				if (!read_coord(buffer, point.x)
					|| !read_coord(buffer, point.y))
					return false;
				pointIn = point;
				pointOut = point;
				break;
			case PATH_COMMAND_CURVE:
				if (!read_coord(buffer, point.x)
					|| !read_coord(buffer, point.y)
					|| !read_coord(buffer, pointIn.x)
					|| !read_coord(buffer, pointIn.y)
					|| !read_coord(buffer, pointOut.x)
					|| !read_coord(buffer, pointOut.y))
					return false;
				break;
		}

		if (!path->AddPoint(point, pointIn, pointOut, false))
			return false;

		last = point;
	}

	return true;
}

// #pragma mark -

// _AppendHLine
bool
PathCommandQueue::_AppendHLine(float x)
{
	return _AppendCommand(PATH_COMMAND_H_LINE)
		&& write_coord(fPointBuffer, x);
}

// _AppendVLine
bool
PathCommandQueue::_AppendVLine(float y)
{
	return _AppendCommand(PATH_COMMAND_V_LINE)
		&& write_coord(fPointBuffer, y);
}

// _AppendLine
bool
PathCommandQueue::_AppendLine(const BPoint& point)
{
	return _AppendCommand(PATH_COMMAND_LINE)
		&& write_coord(fPointBuffer, point.x)
		&& write_coord(fPointBuffer, point.y);
}

// _AppendCurve
bool
PathCommandQueue::_AppendCurve(const BPoint& point,
							   const BPoint& pointIn,
							   const BPoint& pointOut)
{
	return _AppendCommand(PATH_COMMAND_CURVE)
		&& write_coord(fPointBuffer, point.x)
		&& write_coord(fPointBuffer, point.y)
		&& write_coord(fPointBuffer, pointIn.x)
		&& write_coord(fPointBuffer, pointIn.y)
		&& write_coord(fPointBuffer, pointOut.x)
		&& write_coord(fPointBuffer, pointOut.y);
}

// #pragma mark -

// _AppendCommand
bool
PathCommandQueue::_AppendCommand(uint8 command)
{
	// NOTE: a path command uses 2 bits, so 4 of
	// them fit into a single byte
	// after we have appended the fourth command,
	// the byte is written to fCommandBuffer and
	// the cycle repeats

	if (fCommandCount == 255) {
		printf("PathCommandQueue::_AppendCommand() - "
			"maximum path section count reached\n");
		return false;
	}

	fCommandByte |= command << fCommandPos;
	fCommandPos += 2;
	fCommandCount++;

	if (fCommandPos == 8) {
		uint8 commandByte = fCommandByte;
		fCommandByte = 0;
		fCommandPos = 0;
		return fCommandBuffer.Write(commandByte);
	}

	return true;
}


// _ReadCommand
bool
PathCommandQueue::_ReadCommand(uint8& command)
{
	if (fCommandCount == 255) {
		printf("PathCommandQueue::_NextCommand() - "
			"maximum path section count reached\n");
		return false;
	}

	if (fCommandPos == 0) {
		// fetch the next four commands from the buffer
		if (!fCommandBuffer.Read(fCommandByte))
			return false;
	}

	command = (fCommandByte >> fCommandPos) & 0x03;
	fCommandPos += 2;
	fCommandCount++;

	if (fCommandPos == 8)
		fCommandPos = 0;

	return true;
}


