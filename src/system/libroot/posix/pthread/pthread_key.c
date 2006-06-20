/* 
** Copyright 2006, Jérôme Duval. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <TLS.h>
#include <pthread.h>
#include "pthread_private.h"

int 
pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
	if (key == NULL)
		return B_BAD_VALUE;
	*key = tls_allocate();
	if (*key > 0)
		return B_OK;
	return *key;
}


int 
pthread_key_delete(pthread_key_t key)
{
	// we don't check if the key is valid
	return B_OK;
}


void *
pthread_getspecific(pthread_key_t key)
{
	return tls_get(key);
}


int 
pthread_setspecific(pthread_key_t key, const void *value)
{
	// we don't check if the key is valid
	tls_set(key, (void *)value);
	return B_OK;
}

