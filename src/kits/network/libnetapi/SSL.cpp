/*
 * Copyright 2011, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <OS.h>

#include <openssl/ssl.h>
#include <openssl/rand.h>


namespace BPrivate {


class SSL {
public:
	SSL()
	{
		SSL_library_init();

		int64 seed = find_thread(NULL) ^ system_time();
		RAND_seed(&seed, sizeof(seed));
	}
};


static SSL sSSL;


}	// namespace BPrivate
