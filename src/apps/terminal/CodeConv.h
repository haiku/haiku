/*
 * Copyright (c) 2001-2005, Haiku, Inc.
 * Copyright (c) 2003-4 Kian Duffy <myob@users.sourceforge.net>
 * Parts Copyright (C) 1998,99 Kazuho Okui and Takashi Murai. 
 * Distributed under the terms of the MIT license.
 */
#ifndef CODECONV_H
#define CODECONV_H


#include <SupportDefs.h>
#include "Coding.h"

#define BEGINS_CHAR(byte) ((byte & 0xc0) >= 0x80)

class CodeConv {
	public:
		CodeConv();
		~CodeConv();

		int32 UTF8GetFontWidth(const char *string);

		/* internal(UTF8) -> coding */
		int32 ConvertFromInternal(const char *src, int32 bytes, char *dst, int coding);

		/* coding -> internal(UTF8) */
		int32 ConvertToInternal(const char *src, int32 bytes, char *dst, int coding);

	private:		
		void euc_to_sjis(uchar *buf);

		unsigned short UTF8toUnicode(const char *utf8);

		int fNowCoding;
};

#endif /* CODECONV_H */
