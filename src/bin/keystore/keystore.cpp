/*
 * Copyright 2012, Haiku Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 *
 * Authors:
 *		Michael Lotz, mmlr@mlotz.ch
 */


#include <Application.h>
#include <KeyStore.h>

#include <stdio.h>


int
add_password(const char* keyring, const char* identifier,
	const char* secondaryIdentifier, const char* passwordString)
{
	BKeyStore keyStore;
	BPasswordKey password(passwordString, B_KEY_PURPOSE_GENERIC, identifier,
		secondaryIdentifier);

	status_t result = keyStore.AddKey(keyring, password);
	if (result != B_OK) {
		printf("failed to add password: %s\n", strerror(result));
		return 2;
	}

	return 0;
}


int
remove_password(const char* keyring, const char* identifier,
	const char* secondaryIdentifier)
{
	BKeyStore keyStore;
	BPasswordKey password;

	status_t result = keyStore.GetKey(keyring, B_KEY_TYPE_PASSWORD, identifier,
		secondaryIdentifier, false, password);
	if (result != B_OK) {
		printf("failed to get password \"%s\": %s\n", identifier,
			strerror(result));
		return 2;
	}

	result = keyStore.RemoveKey(keyring, password);
	if (result != B_OK) {
		printf("failed to remove password: %s\n", strerror(result));
		return 3;
	}

	return 0;
}


int
add_keyring(const char* keyring)
{
	BKeyStore keyStore;

	status_t result = keyStore.AddKeyring(keyring);
	if (result != B_OK) {
		printf("failed to add keyring: %s\n", strerror(result));
		return 2;
	}

	return 0;
}


int
remove_keyring(const char* keyring)
{
	BKeyStore keyStore;

	status_t result = keyStore.RemoveKeyring(keyring);
	if (result != B_OK) {
		printf("failed to remove keyring: %s\n", strerror(result));
		return 2;
	}

	return 0;
}


int
list_passwords(const char* keyring)
{
	BKeyStore keyStore;
	uint32 cookie = 0;

	while (true) {
		BPasswordKey password;
		status_t result = keyStore.GetNextKey(keyring, B_KEY_TYPE_PASSWORD,
			B_KEY_PURPOSE_ANY, cookie, password);
		if (result == B_ENTRY_NOT_FOUND)
			break;

		if (result != B_OK) {
			printf("failed to get next key with: %s\n", strerror(result));
			return 2;
		}

		password.PrintToStream();
	}

	return 0;
}


int
list_keyrings()
{
	BKeyStore keyStore;
	uint32 cookie = 0;

	while (true) {
		BString keyring;
		status_t result = keyStore.GetNextKeyring(cookie, keyring);
		if (result == B_ENTRY_NOT_FOUND)
			break;

		if (result != B_OK) {
			printf("failed to get next key with: %s\n", strerror(result));
			return 2;
		}

		printf("keyring: \"%s\"\n", keyring.String());
	}

	return 0;
}


int
show_status(const char* keyring)
{
	BKeyStore keyStore;
	printf("keyring \"%s\" is %slocked\n", keyring,
		keyStore.IsKeyringUnlocked(keyring) ? "un" : "");
	return 0;
}


int
lock_keyring(const char* keyring)
{
	BKeyStore keyStore;
	status_t result = keyStore.LockKeyring(keyring);
	if (result != B_OK) {
		printf("failed to lock keyring \"%s\": %s\n", keyring,
			strerror(result));
		return 2;
	}

	return 0;
}


int
add_keyring_to_master(const char* keyring)
{
	BKeyStore keyStore;
	status_t result= keyStore.AddKeyringToMaster(keyring);
	if (result != B_OK) {
		printf("failed to add keyring \"%s\" to master: %s\n", keyring,
			strerror(result));
		return 2;
	}

	return 0;
}


int
remove_keyring_from_master(const char* keyring)
{
	BKeyStore keyStore;
	status_t result= keyStore.RemoveKeyringFromMaster(keyring);
	if (result != B_OK) {
		printf("failed to remove keyring \"%s\" from master: %s\n", keyring,
			strerror(result));
		return 2;
	}

	return 0;
}


int
list_applications(const char* keyring)
{
	BKeyStore keyStore;
	uint32 cookie = 0;

	while (true) {
		BString signature;
		status_t result = keyStore.GetNextApplication(keyring,
			cookie, signature);
		if (result == B_ENTRY_NOT_FOUND)
			break;

		if (result != B_OK) {
			printf("failed to get next application: %s\n", strerror(result));
			return 2;
		}

		printf("application: \"%s\"\n", signature.String());
	}

	return 0;
}


int
remove_application(const char* keyring, const char* signature)
{
	BKeyStore keyStore;

	status_t result = keyStore.RemoveApplication(keyring, signature);
	if (result != B_OK) {
		printf("failed to remove application: %s\n", strerror(result));
		return 3;
	}

	return 0;
}


int
set_unlock_key(const char* keyring, const char* passwordString)
{
	BKeyStore keyStore;
	BPasswordKey password(passwordString, B_KEY_PURPOSE_KEYRING, NULL);

	status_t result = keyStore.SetUnlockKey(keyring, password);
	if (result != B_OK) {
		printf("failed to set unlock key: %s\n", strerror(result));
		return 3;
	}

	return 0;
}


int
remove_unlock_key(const char* keyring)
{
	BKeyStore keyStore;

	status_t result = keyStore.RemoveUnlockKey(keyring);
	if (result != B_OK) {
		printf("failed to remove unlock key: %s\n", strerror(result));
		return 3;
	}

	return 0;
}


int
print_usage(const char* name)
{
	printf("usage:\n");
	printf("\t%s list passwords [<fromKeyring>]\n", name);
	printf("\t\tLists all passwords of the specified keyring or from the"
		" master keyring if none is supplied.\n");
	printf("\t%s list keyrings\n", name);
	printf("\t\tLists all keyrings.\n");
	printf("\t%s list applications [<fromKeyring>]\n", name);
	printf("\t\tLists the applications that have been granted permanent access"
		" to a keyring once it is unlocked.\n\n");

	printf("\t%s add password <identifier> [<secondaryIdentifier>] <password>"
		"\n", name);
	printf("\t\tAdds the specified password to the master keyring.\n");
	printf("\t%s add password to <keyring> <identifier> [<secondaryIdentifier>]"
		" <password>\n", name);
	printf("\t\tAdds the specified password to the specified keyring.\n\n");

	printf("\t%s remove password <identifier> [<secondaryIdentifier>]\n", name);
	printf("\t\tRemoves the specified password from the master keyring.\n");
	printf("\t%s remove password from <keyring> <identifier>"
		" [<secondaryIdentifier>]\n", name);
	printf("\t\tRemoves the specified password from the specified keyring.\n\n");

	printf("\t%s add keyring <name>\n", name);
	printf("\t\tAdds a new keyring with the specified name.\n");
	printf("\t%s remove keyring <name>\n", name);
	printf("\t\tRemoves the specified keyring.\n\n");

	printf("\t%s status [<keyring>]\n", name);
	printf("\t\tShows the lock state of the specified keyring, or the"
		" master keyring if none is supplied.\n\n");

	printf("\t%s lock [<keyring>]\n", name);
	printf("\t\tLock the specified keyring, or the master keyring if none is"
		" supplied.\n\n");

	printf("\t%s master add <keyring>\n", name);
	printf("\t\tAdd the access key for the specified keyring to the master"
		" keyring.\n");

	printf("\t%s master remove <keyring>\n", name);
	printf("\t\tRemove the access key for the specified keyring from the"
		" master keyring.\n\n");

	printf("\t%s remove application <signature>\n", name);
	printf("\t\tRemove permanent access for the application with the given"
		" signature from the master keyring.\n");
	printf("\t%s remove application from <keyring> <signature>\n", name);
	printf("\t\tRemove permanent access for the application with the given"
		" signature from the specified keyring.\n\n");

	printf("\t%s key set <keyring> <password>\n", name);
	printf("\t\tSet the unlock key of the specified keyring to the given"
		" password.\n");
	printf("\t%s key remove <keyring>\n", name);
	printf("\t\tRemove the unlock key of the specified keyring.\n");
	return 1;
}


int
main(int argc, char* argv[])
{
	BApplication app("application/x-vnd.Haiku-keystore-cli");

	if (argc < 2)
		return print_usage(argv[0]);

	if (strcmp(argv[1], "list") == 0) {
		if (argc < 3)
			return print_usage(argv[0]);

		if (strcmp(argv[2], "passwords") == 0)
			return list_passwords(argc > 3 ? argv[3] : NULL);
		if (strcmp(argv[2], "keyrings") == 0)
			return list_keyrings();
		if (strcmp(argv[2], "applications") == 0)
			return list_applications(argc > 3 ? argv[3] : NULL);
	} else if (strcmp(argv[1], "add") == 0) {
		if (argc < 3)
			return print_usage(argv[0]);

		if (strcmp(argv[2], "password") == 0) {
			if (argc < 5)
				return print_usage(argv[0]);

			const char* keyring = NULL;
			const char* identifier = NULL;
			const char* secondaryIdentifier = NULL;
			const char* password = NULL;
			if (argc >= 7 && argc <= 8 && strcmp(argv[3], "to") == 0) {
				keyring = argv[4];
				identifier = argv[5];
				if (argc == 7)
					password = argv[6];
				else {
					secondaryIdentifier = argv[6];
					password = argv[7];
				}
			} else if (argc <= 6) {
				identifier = argv[3];
				if (argc == 5)
					password = argv[4];
				else {
					secondaryIdentifier = argv[4];
					password = argv[5];
				}
			}

			if (password != NULL) {
				return add_password(keyring, identifier, secondaryIdentifier,
					password);
			}
		} else if (strcmp(argv[2], "keyring") == 0) {
			if (argc < 4)
				return print_usage(argv[0]);

			return add_keyring(argv[3]);
		}
	} else if (strcmp(argv[1], "remove") == 0) {
		if (argc < 3)
			return print_usage(argv[0]);

		if (strcmp(argv[2], "password") == 0) {
			if (argc < 4)
				return print_usage(argv[0]);

			const char* keyring = NULL;
			const char* identifier = NULL;
			const char* secondaryIdentifier = NULL;
			if (argc >= 6 && argc <= 7 && strcmp(argv[3], "from") == 0) {
				keyring = argv[4];
				identifier = argv[5];
				if (argc == 7)
					secondaryIdentifier = argv[6];
			} else if (argc <= 5) {
				identifier = argv[3];
				if (argc == 5)
					secondaryIdentifier = argv[4];
			}

			if (identifier != NULL) {
				return remove_password(keyring, identifier,
					secondaryIdentifier);
			}
		} else if (strcmp(argv[2], "keyring") == 0) {
			if (argc == 4)
				return remove_keyring(argv[3]);
		} else if (strcmp(argv[2], "application") == 0) {
			const char* keyring = NULL;
			const char* signature = NULL;
			if (argc == 6 && strcmp(argv[3], "from") == 0) {
				keyring = argv[4];
				signature = argv[5];
			} else if (argc == 4)
				signature = argv[3];

			if (signature != NULL)
				return remove_application(keyring, signature);
		}
	} else if (strcmp(argv[1], "status") == 0) {
		if (argc != 2 && argc != 3)
			return print_usage(argv[0]);

		return show_status(argc == 3 ? argv[2] : "");
	} else if (strcmp(argv[1], "lock") == 0) {
		if (argc != 2 && argc != 3)
			return print_usage(argv[0]);

		return lock_keyring(argc == 3 ? argv[2] : "");
	} else if (strcmp(argv[1], "master") == 0) {
		if (argc != 4)
			return print_usage(argv[0]);

		if (strcmp(argv[2], "add") == 0)
			return add_keyring_to_master(argv[3]);
		if (strcmp(argv[2], "remove") == 0)
			return remove_keyring_from_master(argv[3]);
	} else if (strcmp(argv[1], "key") == 0) {
		if (argc < 4)
			return print_usage(argv[0]);

		if (strcmp(argv[2], "set") == 0) {
			if (argc == 5)
				return set_unlock_key(argv[3], argv[4]);
		} else if (strcmp(argv[2], "remove") == 0) {
			if (argc == 4)
				return remove_unlock_key(argv[3]);
		}
	}

	return print_usage(argv[0]);
}
