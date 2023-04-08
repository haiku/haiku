/*
 * Copyright 2023, Haiku, Inc. All rights reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef _GNU_CRYPT_H_
#define _GNU_CRYPT_H_


#include <features.h>


#ifdef _DEFAULT_SOURCE


#include <sys/types.h>


#ifdef __cplusplus
extern "C" {
#endif

struct crypt_data {
	int initialized;
	char buf[512];
};


char *crypt_rn(const char *key, const char *salt, struct crypt_data *data, size_t size);


static inline char *
crypt_r(const char *key, const char *salt, struct crypt_data *data)
{
	return crypt_rn(key, salt, data, sizeof(struct crypt_data));
}


#ifdef __cplusplus
}
#endif


#endif


#endif	/* _GNU_CRYPT_H_ */
