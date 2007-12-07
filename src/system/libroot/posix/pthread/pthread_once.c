/* 
** Copyright 2007, Jérôme Duval. All rights reserved.
** Distributed under the terms of the MIT License.
*/


#include <pthread.h>
#include "pthread_private.h"


int 
pthread_once(pthread_once_t *once_control, void (*init_routine)(void))
{
	if (once_control->state == PTHREAD_NEEDS_INIT) {
		// TODO race condition ?
		if (once_control->mutex == NULL) {
			if (pthread_mutex_init(&once_control->mutex, NULL) != 0)
				return -1;
		}
		pthread_mutex_lock(&once_control->mutex);

		if (once_control->state == PTHREAD_NEEDS_INIT) {
			init_routine();
			once_control->state = PTHREAD_DONE_INIT;
		}

		pthread_mutex_unlock(&once_control->mutex);
	}
	return 0;
}

