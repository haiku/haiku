#include <pthread_emu.h>


pthread_mutex_t
pthread_mutex_static_initializer(void)
{
	return _pthread_mutex_static_initializer();
}

/*
Does not work because the destructor function must be called for _every_ thread.
Solution: maybe run a thread in the background that watches for dying threads
and calls the key's destructor function.

Does tls_allocate() set the key's value to NULL?

int
pthread_key_create(pthread_key_t *key, void (*destructor)(void *))
{
	if(!key)
		return B_ERROR;
	
	*key = tls_allocate();
	if(destructor)
		return on_exit_thread(destructor, tls_address(*key));
	
	return B_OK;
}


int
pthread_setspecific(pthread_key_t key, void *value)
{
	tls_set(key, value);
	return B_OK;
}


void*
pthread_getspecific(pthread_key_t key)
{
	return tls_get(key);
}
*/
