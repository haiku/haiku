/*
 * Copyright 2011, Haiku, Inc. All rights reserved.
 * Copyright 2001-2003 Dr. Zoidberg Enterprises. All rights reserved.
 */


#include <ctype.h>
#include <string.h>

#include <mail_encoding.h>
#include <SupportDefs.h>

static const char kBase64Alphabet[64] = {
  'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
  'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
  'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
  'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
  '0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
  '+',
  '/'
 };

static const char kHexAlphabet[16] = {'0', '1', '2', '3', '4', '5', '6', '7',
	'8','9','A','B','C','D','E','F'};


ssize_t
encode_base64(char *out, const char *in, off_t length)
{
	uint32 concat;
	int i = 0;
	int k = 0;

	while (i < length) {
		concat = ((in[i] & 0xff) << 16);

		if ((i+1) < length)
			concat |= ((in[i+1] & 0xff) << 8);
		if ((i+2) < length)
			concat |= (in[i+2] & 0xff);

		i += 3;

		out[k++] = kBase64Alphabet[(concat >> 18) & 63];
		out[k++] = kBase64Alphabet[(concat >> 12) & 63];
		if ((i+1) < length)
			out[k++] = kBase64Alphabet[(concat >> 6) & 63];
		if ((i+2) < length)
			out[k++] = kBase64Alphabet[concat & 63];
	}

	return k;
}


ssize_t
decode_base64(char *out, const char *in, off_t length)
{
	uint32 concat, value;
	int lastOutLine = 0;
	int i, j;
	int outIndex = 0;

	for (i = 0; i < length; i += 4) {
		concat = 0;

		for (j = 0; j < 4 && (i + j) < length; j++) {
			value = in[i + j];

			if (value == '\n' || value == '\r') {
				// jump over line breaks
				lastOutLine = outIndex;
				i++;
				j--;
				continue;
			}

			if ((value >= 'A') && (value <= 'Z'))
				value -= 'A';
			else if ((value >= 'a') && (value <= 'z'))
				value = value - 'a' + 26;
			else if ((value >= '0') && (value <= '9'))
				value = value - '0' + 52;
			else if (value == '+')
				value = 62;
			else if (value == '/')
				value = 63;
			else if (value == '=')
				break;
			else {
				// there is an invalid character in this line - we will
				// ignore the whole line and go to the next
				outIndex = lastOutLine;
				while (i < length && in[i] != '\n' && in[i] != '\r')
					i++;
				concat = 0;
			}

			value = value << ((3-j)*6);

			concat |= value;
		}

		if (j > 1)
			out[outIndex++] = (concat & 0x00ff0000) >> 16;
		if (j > 2)
			out[outIndex++] = (concat & 0x0000ff00) >> 8;
		if (j > 3)
			out[outIndex++] = (concat & 0x000000ff);
	}

	return outIndex;
}


