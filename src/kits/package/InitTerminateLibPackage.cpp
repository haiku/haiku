/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rene Gollent <rene@gollent.com>
 */


#ifndef HAIKU_BOOTSTRAP_BUILD
#include <curl/curl.h>
#endif


extern "C" void
initialize_after()
{
	#ifndef HAIKU_BOOTSTRAP_BUILD
	curl_global_init(CURL_GLOBAL_SSL);
	#endif
}


extern "C" void
terminate_after()
{
	#ifndef HAIKU_BOOTSTRAP_BUILD
	curl_global_cleanup();
	#endif
}
