/*
 * Copyright 2008, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef SHA_256_H
#define SHA_256_H


#include <SupportDefs.h>


#define SHA_DIGEST_LENGTH	32


class SHA256 {
public:
								SHA256();
								~SHA256();

			void				Init();
			void				Update(const void* buffer, size_t size);
			const uint8*		Digest();
			size_t				DigestLength() const
									{ return SHA_DIGEST_LENGTH; }

private:
			void				_ProcessChunk();

private:
			uint32				fHash[8];
			uint32				fDigest[8];
			uint32				fBuffer[64];
			size_t				fBytesInBuffer;
			size_t				fMessageSize;
			bool				fDigested;
};


#endif	// SHA_256_H
