/*
 * Copyright 2023 Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#include <crypt.h>


/* move this to libroot_private.h ??? */
extern "C" char *_crypt_rn(const char* key, const char* setting, struct crypt_data* data, size_t size);


char *
crypt_rn(const char* key, const char* setting, struct crypt_data* data, size_t size)
{
	return _crypt_rn(key, setting, data, size);
}
