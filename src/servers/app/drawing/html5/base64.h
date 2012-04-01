/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
 */
#ifndef BASE64_H
#define BASE64_H

#include <ctype.h>


extern ssize_t encode_base64(char *out, const char *in, off_t length);
extern ssize_t decode_base64(char *out, const char *in, off_t length);


#endif // BASE64_H
