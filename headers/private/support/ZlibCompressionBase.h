/*
 * Copyright 2009-2014, Ingo Weinhold, ingo_weinhold@gmx.de.
 * Distributed under the terms of the MIT License.
 */
#ifndef _ZLIB_COMPRESSION_BASE_H_
#define _ZLIB_COMPRESSION_BASE_H_


#include <SupportDefs.h>


class BDataIO;


class BZlibCompressionBase {
public:
								BZlibCompressionBase(BDataIO* output);
								~BZlibCompressionBase();

	static	status_t			TranslateZlibError(int error);

protected:
			struct ZStream;

protected:
			status_t			CreateStream();
			void				DeleteStream();

protected:
			BDataIO*			fOutput;
			ZStream*			fStream;
};


#endif	// _ZLIB_COMPRESSION_BASE_H_
