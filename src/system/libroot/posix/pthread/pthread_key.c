/* 
** Copyright 2006, Jérôme Duval. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <TLS.h>
#include <pthread.h>
#include <stdlib.h>
#include "pthread_private.h"

struct key_link {
	struct key_link *next;
	pthread_key_t key;
	void (*destructor)(void*);
};

static pthread_mutex_t sKeyLock = PTHREAD_MUTEX_INITIALIZER;
static struct key_link *sKeyList = NULL;


void
_pthread_key_call_destructors(void *param)
{
	struct key_link *link = NULL;

	pthread_mutex_lock(&sKeyLock);

	for (link = sKeyList; link; link = link->next) {
		void *data = pthread_getspecific(link->key);
		if (link->destructor && data)
			(*link->destructor)(data);
	}

	pthread_mutex_unlock(&sKeyLock);
}


int 
pthread_key_create(pthread_key_t *key, void (*destructor)(void*))
{
	struct key_link *link;

	if (key == NULL)
		return B_BAD_VALUE;
	pthread_mutex_lock(&sKeyLock);
	*key = tls_allocate();
	if (*key < 0) {
		pthread_mutex_unlock(&sKeyLock);
		return *key;
	}

	// add key/destructor to the list
	link = malloc(sizeof(struct key_link));
	if (!link)
		return B_WOULD_BLOCK;
	link->next = sKeyList;
	link->key = *key;
	link->destructor= destructor;
	sKeyList = link;
	pthread_mutex_unlock(&sKeyLock);
	return B_OK;
}


int 
pthread_key_delete(pthread_key_t key)
{
	struct key_link *link, *last = NULL;
	pthread_mutex_lock(&sKeyLock);
	
	//remove key/destructor from the list
	for (link = sKeyList; link; link = link->next) {
		if (link->key == key) {
			if (last)
				last->next = link->next;
			else 
				sKeyList = link->next;
			free(link);
			pthread_mutex_unlock(&sKeyLock);
			return B_OK;
		}
		last = link;
	}

	pthread_mutex_unlock(&sKeyLock);
	return B_BAD_VALUE;
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

