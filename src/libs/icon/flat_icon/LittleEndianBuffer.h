/*
 * Copyright 2006-2007, Haiku. All rights reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Stephan Aßmus <superstippi@gmx.de>
 */
#ifndef LITTLE_ENDIAN_BUFFER_H
#define LITTLE_ENDIAN_BUFFER_H


#include <SupportDefs.h>

#include "IconBuild.h"


_BEGIN_ICON_NAMESPACE


class LittleEndianBuffer {
 public:
								LittleEndianBuffer();
								LittleEndianBuffer(size_t size);
								LittleEndianBuffer(uint8* buffer,
												   size_t size);
								~LittleEndianBuffer();

			bool				Write(uint8 value);
			bool				Write(uint16 value);
			bool				Write(uint32 value);
			bool				Write(float value);
			bool				Write(double value);

			bool				Write(const LittleEndianBuffer& other);
			bool				Write(const uint8* buffer, size_t bytes);

			bool				Read(uint8& value);
			bool				Read(uint16& value);
			bool				Read(uint32& value);
			bool				Read(float& value);
			bool				Read(double& value);
			bool				Read(LittleEndianBuffer& other, size_t bytes);

			void				Skip(size_t bytes);

			uint8*				Buffer() const
									{ return fBuffer; }
			size_t				SizeUsed() const
									{ return fHandle - fBuffer; }

			void				Reset();

 private:
			void				_SetSize(size_t size);

			uint8*				fBuffer;
			uint8*				fHandle;
			uint8*				fBufferEnd;
			size_t				fSize;
			bool				fOwnsBuffer;
};


_END_ICON_NAMESPACE


#endif	// LITTLE_ENDIAN_BUFFER_H
