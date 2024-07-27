/*
 * Copyright (c) 2007-2012, Novell Inc.
 *
 * This program is licensed under the BSD license, read LICENSE.BSD
 * for further information
 */

#ifndef LIBSOLV_CHKSUM_H
#define LIBSOLV_CHKSUM_H

#include "pool.h"

#ifdef __cplusplus
extern "C" {
#endif

void *solv_chksum_create(Id type);
void *solv_chksum_create_from_bin(Id type, const unsigned char *buf);
void solv_chksum_add(void *handle, const void *data, int len);
Id solv_chksum_get_type(void *handle);
int solv_chksum_isfinished(void *handle);
const unsigned char *solv_chksum_get(void *handle, int *lenp);
void *solv_chksum_free(void *handle, unsigned char *cp);
const char *solv_chksum_type2str(Id type);
Id solv_chksum_str2type(const char *str);
int solv_chksum_len(Id type);

#ifdef __cplusplus
}
#endif

#endif /* LIBSOLV_CHKSUM_H */
