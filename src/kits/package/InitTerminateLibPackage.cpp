/*
 * Copyright 2013, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Rene Gollent <rene@gollent.com>
 */

#include <curl/curl.h>

extern "C" void
initialize_after()
{
	curl_global_init(CURL_GLOBAL_SSL);
}


extern "C" void
terminate_after()
{
	curl_global_cleanup();
}
