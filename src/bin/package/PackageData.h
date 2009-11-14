/*
 * Copyright 2009, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef PACKAGE_DATA_H
#define PACKAGE_DATA_H


#include <haiku_package.h>


class PackageData {
public:
								PackageData();

			uint64				CompressedSize() const
									{ return fCompressedSize; }
			uint64				UncompressedSize() const
									{ return fUncompressedSize; }
			uint64				Offset() const			{ return fOffset; }
			uint32				Compression() const		{ return fCompression; }
			uint32				ChunkSize() const		{ return fChunkSize; }

			bool				IsEncodedInline() const
									{ return fEncodedInline; }
			const uint8*		InlineData() const		{ return fInlineData; }

			void				SetData(uint64 size, uint64 offset);
			void				SetData(uint8 size, const void* data);

			void				SetCompression(uint32 compression)
									{ fCompression = compression; }
			void				SetUncompressedSize(uint64 size)
									{ fUncompressedSize = size; }
			void				SetChunkSize(uint32 size)
									{ fChunkSize = size; }

private:
			uint64				fCompressedSize;
			uint64				fUncompressedSize;
			union {
				uint64			fOffset;
				uint8			fInlineData[B_HPKG_MAX_INLINE_DATA_SIZE];
			};
			uint32				fChunkSize;
			uint32				fCompression;
			bool				fEncodedInline;
};


#endif	// PACKAGE_DATA_H
