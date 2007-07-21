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

class CodeConv {
	public:
		static int32 UTF8GetFontWidth(const char *string);

		/* internal(UTF8) -> coding */
		static int32 ConvertFromInternal(const char *src, int32 bytes, char *dst, int coding);

		/* coding -> internal(UTF8) */
		static int32 ConvertToInternal(const char *src, int32 bytes, char *dst, int coding);

	private:		
		static void euc_to_sjis(uchar *buf);
		static unsigned short UTF8toUnicode(const char *utf8);
};

#endif /* CODECONV_H */
