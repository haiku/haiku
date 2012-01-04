/*
 * Copyright 2012, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */


#include <KeyStore.h>

#include <stdio.h>


int
main(int argc, char* argv[])
{
	BKeyStore keyStore;
	BPasswordKey password;
	uint32 cookie = 0;

	while (true) {
		printf("trying to get next password with cookie: %" B_PRIu32 "\n",
			cookie);

		status_t result = keyStore.GetNextKey(B_KEY_TYPE_PASSWORD,
			B_KEY_PURPOSE_ANY, cookie, password);
		if (result != B_OK) {
			printf("failed with: %s\n", strerror(result));
			break;
		}

		password.PrintToStream();
	}

	return 0;
}
