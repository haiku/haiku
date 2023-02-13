/*
 * Copyright 2017, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 * 		Andrew Aldridge, i80and@foxquill.com
 */


#include <assert.h>
#include <crypt.h>
#include <errno.h>
#include <fcntl.h>
#include <inttypes.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include <SupportDefs.h>

#include "../musl/crypt/crypt_des.h"
#include "crypto_scrypt.h"

#define SALT_BYTES 32
#define SALT_STR_BYTES (SALT_BYTES * 2 + 1)
#define DEFAULT_N_LOG2 14

#define CRYPT_OUTPUT_BYTES (6 + 64 + 1 + 64 + 1)
#define SALT_OUTPUT_BYTES (6 + 64 + 1 + 1)

static const char* kHexAlphabet = "0123456789abcdef";
static const int8 kHexLookup[] = {
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 2, 3,
	4, 5, 6, 7, 8, 9, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,
	-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 10, 11, 12, 13, 14, 15};


static int
toHex(const uint8* buffer, size_t bufferLength, char* outBuffer,
	size_t outBufferLength)
{
	size_t i;
	size_t outIndex = 0;

	if (outBufferLength <= bufferLength * 2) {
		outBuffer[0] = '\0';
		return 1;
	}

	for (i = 0; i < bufferLength; i += 1) {
		const uint8 n = buffer[i];
		const uint8 upper = n >> 4;
		const uint8 lower = n & 0x0f;

		assert(lower < 16 && upper < 16);
		outBuffer[outIndex++] = kHexAlphabet[upper];
		outBuffer[outIndex++] = kHexAlphabet[lower];
		outBuffer[outIndex] = '\0';
	}

	outBuffer[outIndex] = '\0';

	return 0;
}


static size_t
fromHex(const char* hex, uint8* outBuffer, size_t outBufferLength)
{
	size_t i = 0;
	size_t outIndex = 0;

	if (hex[0] == '\0' || outBufferLength == 0)
		return 0;

	while (hex[i] != '\0' && hex[i + 1] != '\0') {
		const uint8 char1 = hex[i];
		const uint8 char2 = hex[i + 1];

		if (char1 >= sizeof(kHexLookup) || char2 >= sizeof(kHexLookup))
			return outIndex;

		const char index1 = kHexLookup[char1];
		const char index2 = kHexLookup[char2];

		if (outIndex >= outBufferLength)
			return 0;

		outBuffer[outIndex++] = (index1 << 4) | index2;
		i += 2;
	}

	return outIndex;
}


//! Generate a new salt appropriate for crypt().
static int
crypt_gensalt_rn(char *outbuf, size_t bufsize)
{
	uint8 salt[SALT_BYTES];
	char saltString[SALT_STR_BYTES];
	size_t totalBytesRead = 0;

	int fd = open("/dev/random", O_RDONLY, 0);
	if (fd < 0)
		return -1;

	while (totalBytesRead < sizeof(salt)) {
		const ssize_t bytesRead = read(fd,
			static_cast<void*>(salt + totalBytesRead),
			sizeof(salt) - totalBytesRead);
		if (bytesRead <= 0) {
			close(fd);
			return -1;
		}

		totalBytesRead += bytesRead;
	}
	close(fd);

	assert(toHex(salt, sizeof(salt), saltString, sizeof(saltString)) == 0);
	snprintf(outbuf, bufsize, "$s$%d$%s$", DEFAULT_N_LOG2, saltString);
	return 0;
}


extern "C" char *
_crypt_rn(const char* key, const char* setting, struct crypt_data* data, size_t size)
{
	uint8 saltBinary[SALT_BYTES];
	char saltString[SALT_STR_BYTES];
	char gensaltResult[SALT_OUTPUT_BYTES];
	uint8 resultBuffer[32];
	char hexResultBuffer[64 + 1];
	int nLog2 = DEFAULT_N_LOG2;

	if (setting == NULL) {
		int res = crypt_gensalt_rn(gensaltResult, sizeof(gensaltResult));

		// crypt_gensalt_r should set errno itself.
		if (res < 0)
			return NULL;

		setting = gensaltResult;
	}

	// Some idioms existed where the password was also used as the salt.
	// As a crude heuristic, use the old crypt algorithm if the salt is
	// shortish.
	if (strlen(setting) < 16)
		return _crypt_des_r(key, setting, data->buf);

	// We don't want to fall into the old algorithm by accident somehow, so
	// if our salt is kind of like our salt, but not exactly, return an
	// error.
	if (sscanf(setting, "$s$%2d$%64s$", &nLog2, saltString) != 2) {
		errno = EINVAL;
		return NULL;
	}

	// Set a lower bound on N_log2: below 12 scrypt is weaker than bcrypt.
	if (nLog2 < 12) {
		errno = EINVAL;
		return NULL;
	}

	size_t saltBinaryLength = fromHex(saltString, saltBinary,
		sizeof(saltBinary));
	if (saltBinaryLength != sizeof(saltBinary)) {
		errno = EINVAL;
		return NULL;
	}

	long n = static_cast<long>(pow(2, nLog2));
	if (crypto_scrypt(reinterpret_cast<const uint8*>(key), strlen(key),
		saltBinary, saltBinaryLength, n, 8, 1, resultBuffer,
		sizeof(resultBuffer)) != 0) {
		// crypto_scrypt sets errno itself
		return NULL;
	}

	assert(toHex(resultBuffer, sizeof(resultBuffer), hexResultBuffer,
		sizeof(hexResultBuffer)) == 0);
	snprintf(data->buf, size - sizeof(int), "$s$%d$%s$%s", nLog2, saltString,
		hexResultBuffer);

	return data->buf;
}


char *
crypt(const char* key, const char* salt)
{
	static struct crypt_data data;
	return _crypt_rn(key, salt, &data, sizeof(struct crypt_data));
}


//! To make fcrypt users happy. They don't need to call init_des.
char*
fcrypt(const char* key, const char* salt)
{
	return crypt(key, salt);
}
