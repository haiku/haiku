#ifndef ZOIDBERG_MAIL_ENCODING_H
#define ZOIDBERG_MAIL_ENCODING_H
/* mail encoding - mail de-/encoding functions (base64 and friends)
**
** Copyright 2001 Dr. Zoidberg Enterprises. All rights reserved.
*/


#include <size_t.h>
#include <sys/types.h>


#define B_MAIL_NULL_CONVERSION ((uint32) -1)
#define B_MAIL_UTF8_CONVERSION ((uint32) -2)
// For specifying the UTF-8 character set when converting to/from UTF-8.
#define B_MAIL_US_ASCII_CONVERSION ((uint32) -3)
// Plain 7 bit ASCII character set.  A subset of UTF-8, but some mail software
// specifies it so we need to recognize it.

#define BASE64_LINELENGTH 76

typedef enum {
	base64 				= 'b',
	quoted_printable 	= 'q',
	seven_bit			= '7',
	eight_bit			= '8',
	uuencode			= 'u', // Invalid to encode something using uuencode.
	null_encoding		= 0,   // For not changing existing settings.
	no_encoding			= -1
} mail_encoding;

#ifdef __cplusplus
extern "C" {
#endif

ssize_t encode(mail_encoding encoding, char *out, const char *in,
	off_t length, int headerMode);
	// boolean headerMode only matters for quoted_printable and base64.
ssize_t decode(mail_encoding encoding, char *out, const char *in, off_t length,
	int underscore_is_space);
	// the value of underscore_is_space only matters for quoted_printable.

ssize_t max_encoded_length(mail_encoding encoding, off_t cur_length);
mail_encoding encoding_for_cte(const char *content_transfer_encoding);

ssize_t	encode_base64(char *out, const char *in, off_t length, int headerMode);
ssize_t	decode_base64(char *out, const char *in, off_t length);

ssize_t	encode_qp(char *out, const char *in, off_t length, int headerMode);
ssize_t	decode_qp(char *out, const char *in, off_t length, int underscore_is_space);
	//---underscore_is_space should be 1 if and only if you are decoding a header field

ssize_t	uu_decode(char *out, const char *in, off_t length);

#ifdef __cplusplus
}
#endif

#endif	/* ZOIDBERG_MAIL_ENCODING_H */
