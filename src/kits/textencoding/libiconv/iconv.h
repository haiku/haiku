// This file is required to patch in our iconv header
// to the iconv implementation.  The iconv implementation
// header is cluttered and inappropriate to include.  We
// need to include our own header to ensure compatibility.
// So we need to provide a little of the clutter here.

#include_next <iconv.h>

#include <errno.h>

#define ICONV_CONST
