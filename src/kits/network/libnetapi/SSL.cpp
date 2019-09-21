/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Copyright 2014 Haiku, inc.
 *
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>
#include <pthread.h>


namespace BPrivate {


class SSL {
public:
	SSL()
	{
		SSL_library_init();

		int64 seed = find_thread(NULL) ^ system_time();
		RAND_seed(&seed, sizeof(seed));

		// Set callbacks required for thread-safe operation of OpenSSL.
		sMutexBuf = new pthread_mutex_t[CRYPTO_num_locks()];
		for (int i = 0; i < CRYPTO_num_locks(); i++)
			pthread_mutex_init(&sMutexBuf[i], NULL);
		CRYPTO_set_id_callback(_GetThreadId);
		CRYPTO_set_locking_callback(_LockingFunction);
	}

	~SSL()
	{
		CRYPTO_set_id_callback(NULL);
		CRYPTO_set_locking_callback(NULL);

		for (int i = 0; i < CRYPTO_num_locks(); i++)
			pthread_mutex_destroy(&sMutexBuf[i]);
		delete[] sMutexBuf;
	}

private:
	static void _LockingFunction(int mode, int n, const char * file, int line)
	{
		if (mode & CRYPTO_LOCK)
			pthread_mutex_lock(&sMutexBuf[n]);
		else
			pthread_mutex_unlock(&sMutexBuf[n]);
	}

	static unsigned long _GetThreadId()
	{
		return find_thread(NULL);
	}

private:
	static pthread_mutex_t* sMutexBuf;
};


static SSL sSSL;
pthread_mutex_t* SSL::sMutexBuf;


}	// namespace BPrivate
