// NetFSServerPrefs.cpp

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <Application.h>
#include <Message.h>

#include "NetFSServerRoster.h"
#include "Permissions.h"

// simplified permissions
static const uint32 kMountPermission = MOUNT_SHARE_PERMISSION;
static const uint32 kQueryPermission = QUERY_SHARE_PERMISSION;
static const uint32 kReadPermission
	= READ_PERMISSION | READ_DIR_PERMISSION | RESOLVE_DIR_ENTRY_PERMISSION;
static const uint32 kWritePermission
	= WRITE_PERMISSION | WRITE_DIR_PERMISSION;

// usage
static const char* kUsage =
"Usage: netfs_server_prefs -h | --help\n"
"       netfs_server_prefs <command>\n"
"options:\n"
"  -h, --help        - print this text\n"
"\n"
"commands:\n"
"  launch\n"
"      launches the server\n"
"  terminate\n"
"      terminates the server\n"
"  save\n"
"      saves the server settings\n"
"  l, list\n"
"      list all users and all shares\n"
"  add share <name> <path>\n"
"      add a new share with the name <name> and path <path>\n"
"  remove share <name>\n"
"      remove the share named <name>\n"
"  add user <name> [ <password> ]\n"
"      add a new user with the name <name> and, if supplied, \n"
"      password <password>\n"
"  remove user <name>\n"
"      remove the user named <name>\n"
"  permissions <user> <share> [ m ] [ r ] [ w ] [ q ]\n"
"      set the permissions of user <user> for share <share> to m(ount),\n"
"      r(ead), w(rite), and/or q(uery).\n"
;

// print_usage
static
void
print_usage(bool error)
{
	fputs(kUsage, (error ? stderr : stdout));
}

// print_usage_and_exit
static
void
print_usage_and_exit(bool error)
{
	print_usage(error);
	exit(error ? 1 : 0);
}

// get_permissions_string
static
void
get_permissions_string(uint32 permissions, char* str)
{
	str[0] = (permissions & kMountPermission ? 'm' : '-');
	str[1] = (permissions & kReadPermission ? 'r' : '-');
	str[2] = (permissions & kWritePermission ? 'w' : '-');
	str[3] = (permissions & kQueryPermission ? 'q' : '-');
	str[4] = '\0';
}

// get_permissions
static
bool
get_permissions(const char* str, uint32* permissions)
{
	*permissions = 0;

	if (!str)
		return true;

	while (*str) {
		switch (*str) {
			case 'm':
				*permissions |= kMountPermission;
				break;
			case 'r':
				*permissions |= kReadPermission;
				break;
			case 'w':
				*permissions |= kWritePermission;
				break;
			case 'q':
				*permissions |= kQueryPermission;
				break;
			default:
				return false;
		}
		str++;
	}

	return true;
}

// assert_server_running
static
void
assert_server_running()
{
	// check, if the server is running
	NetFSServerRoster roster;
	if (!roster.IsServerRunning()) {
		fprintf(stderr, "Server is not running.\n");
		exit(1);
	}
}

// list
static
void
list()
{
	assert_server_running();

	NetFSServerRoster roster;

	// get the users
	BMessage users;
	status_t error = roster.GetUsers(&users);
	if (error == B_OK) {
		// list the users
		printf("users\n");
		printf("-----\n");
		const char* user;
		for (int32 i = 0; users.FindString("users", i, &user) == B_OK; i++)
			printf("%s\n", user);
		printf("\n");
	} else
		fprintf(stderr, "Failed to get users: %s\n", strerror(error));

	// get the shares
	BMessage shares;
	error = roster.GetShares(&shares);
	if (error == B_OK) {
		// list the shares
		printf("shares\n");
		printf("------\n");
		const char* share;
		for (int32 i = 0; shares.FindString("shares", i, &share) == B_OK; i++) {
			// get path
			const char* path;
			if (shares.FindString("paths", i, &path) != B_OK)
				path = "<invalid path>\n";

			// get share users
			BMessage shareUsers;
			roster.GetShareUsers(share, &shareUsers);

			// get statistics
			BMessage statistics;
			roster.GetShareStatistics(share, &statistics);

			printf("%s:\n", share);
			printf("  path:         %s\n", path);

			// print permitted users
			printf("  mountable by: ");
			const char* user;
			for (int32 k = 0;
				 shareUsers.FindString("users", k, &user) == B_OK;
				 k++) {
				if (k > 0)
					printf(", ");
				printf("%s", user);

				// print permissions
				uint32 permissions = 0;
				roster.GetUserPermissions(share, user, &permissions);
				char permissionsString[8];
				get_permissions_string(permissions, permissionsString);
				printf(" (%s)", permissionsString);
			}
			printf("\n");

			// print current users
			printf("  mounted by:   ");
			for (int32 k = 0;
				 statistics.FindString("mounted by", k, &user) == B_OK;
				 k++) {
				if (k > 0)
					printf(", ");
				printf("%s", user);
			}
			printf("\n");

			printf("\n");
		}
	} else
		fprintf(stderr, "Failed to get users: %s\n", strerror(error));
}

// add_share
static
void
add_share(const char* name, const char* path)
{
	assert_server_running();

	NetFSServerRoster roster;

	// check whether a share with the given name already exists
	BMessage statistics;
	if (roster.GetShareStatistics(name, &statistics) == B_OK) {
		fprintf(stderr, "A share `%s' does already exist.\n", name);
		exit(1);
	}

	// add the share
	status_t error = roster.AddShare(name, path);
	if (error != B_OK) {
		fprintf(stderr, "Failed to add share: %s\n", strerror(error));
		exit(1);
	}
}

// remove_share
static
void
remove_share(const char* name)
{
	assert_server_running();

	NetFSServerRoster roster;

	// check whether a share with the given name exists
	BMessage statistics;
	if (roster.GetShareStatistics(name, &statistics) != B_OK) {
		fprintf(stderr, "A share `%s' does not exist.\n", name);
		exit(1);
	}

	// remove the share
	status_t error = roster.RemoveShare(name);
	if (error != B_OK) {
		fprintf(stderr, "Failed to remove share: %s\n", strerror(error));
		exit(1);
	}
}

// add_user
static
void
add_user(const char* name, const char* password)
{
	assert_server_running();

	NetFSServerRoster roster;

	// check whether a user with the given name already exists
	BMessage statistics;
	if (roster.GetUserStatistics(name, &statistics) == B_OK) {
		fprintf(stderr, "A user `%s' does already exist.\n", name);
		exit(1);
	}

	// add the user
	status_t error = roster.AddUser(name, password);
	if (error != B_OK) {
		fprintf(stderr, "Failed to add user: %s\n", strerror(error));
		exit(1);
	}
}

// remove_user
static
void
remove_user(const char* name)
{
	assert_server_running();

	NetFSServerRoster roster;

	// check whether a user with the given name exists
	BMessage statistics;
	if (roster.GetUserStatistics(name, &statistics) != B_OK) {
		fprintf(stderr, "A user `%s' does not exist.\n", name);
		exit(1);
	}

	// remove the user
	status_t error = roster.RemoveUser(name);
	if (error != B_OK) {
		fprintf(stderr, "Failed to remove user: %s\n", strerror(error));
		exit(1);
	}
}

// set_user_permissions
static
void
set_user_permissions(const char* user, const char* share, uint32 permissions)
{
	assert_server_running();

	NetFSServerRoster roster;

	// check whether a user with the given name exists
	BMessage statistics;
	if (roster.GetUserStatistics(user, &statistics) != B_OK) {
		fprintf(stderr, "A user `%s' does not exist.\n", user);
		exit(1);
	}

	// check whether a share with the given name exists
	if (roster.GetShareStatistics(share, &statistics) != B_OK) {
		fprintf(stderr, "A share `%s' does not exist.\n", share);
		exit(1);
	}

	// set the permissions
	status_t error = roster.SetUserPermissions(share, user, permissions);
	if (error != B_OK) {
		fprintf(stderr, "Failed to set permissions: %s\n", strerror(error));
		exit(1);
	}
}

// launch_server
static
void
launch_server()
{
	NetFSServerRoster roster;

	if (roster.IsServerRunning()) {
		fprintf(stderr, "Server is already running.\n");
		exit(1);
	}

	status_t error = roster.LaunchServer();
	if (error != B_OK) {
		fprintf(stderr, "Failed to launch server: %s\n", strerror(error));
		exit(1);
	}
}

// terminate_server
static
void
terminate_server()
{
	assert_server_running();

	NetFSServerRoster roster;

	status_t error = roster.TerminateServer();
	if (error != B_OK) {
		fprintf(stderr, "Failed to terminate server: %s\n", strerror(error));
		exit(1);
	}
}

// save_server_setttings
static
void
save_server_setttings()
{
	assert_server_running();

	NetFSServerRoster roster;

	status_t error = roster.SaveServerSettings();
	if (error != B_OK) {
		fprintf(stderr, "Failed to save settings: %s\n", strerror(error));
		exit(1);
	}
}

// next_arg
static
const char*
next_arg(int argc, char** argv, int& argi, bool dontFail = false)
{
	if (argi >= argc) {
		if (dontFail)
			return NULL;
		print_usage_and_exit(true);
	}

	return argv[argi++];
}

// no_more_args
static
void
no_more_args(int argc, int argi)
{
	if (argi < argc)
		print_usage_and_exit(true);
}

// main
int
main(int argc, char** argv)
{
	BApplication app("application/x-vnd.haiku-netfs_server_prefs");

	// parse first argument
	int argi = 1;
	const char* arg = next_arg(argc, argv, argi);
	if (strcmp(arg, "-h") == 0 || strcmp(arg, "--help") == 0)
		print_usage_and_exit(false);

	if (strcmp(arg, "launch") == 0) {
		// launch
		no_more_args(argc, argi);
		launch_server();
	} else if (strcmp(arg, "terminate") == 0) {
		// terminate
		no_more_args(argc, argi);
		terminate_server();
	} else if (strcmp(arg, "save") == 0) {
		// save
		no_more_args(argc, argi);
		save_server_setttings();
	} else if (strcmp(arg, "l") == 0 || strcmp(arg, "list") == 0) {
		// list
		no_more_args(argc, argi);
		list();
	} else if (strcmp(arg, "add") == 0) {
		// add
		arg = next_arg(argc, argv, argi);
		if (strcmp(arg, "share") == 0) {
			// share
			const char* name = next_arg(argc, argv, argi);
			const char* path = next_arg(argc, argv, argi);
			no_more_args(argc, argi);
			add_share(name, path);
		} else if (strcmp(arg, "user") == 0) {
			// user
			const char* name = next_arg(argc, argv, argi);
			const char* password = next_arg(argc, argv, argi, true);
			no_more_args(argc, argi);
			add_user(name, password);
		} else
			print_usage_and_exit(true);
	} else if (strcmp(arg, "remove") == 0) {
		// remove
		arg = next_arg(argc, argv, argi);
		if (strcmp(arg, "share") == 0) {
			// share
			const char* name = next_arg(argc, argv, argi);
			no_more_args(argc, argi);
			remove_share(name);
		} else if (strcmp(arg, "user") == 0) {
			// user
			const char* name = next_arg(argc, argv, argi);
			no_more_args(argc, argi);
			remove_user(name);
		} else
			print_usage_and_exit(true);
	} else if (strcmp(arg, "permissions") == 0) {
		// permissions
		const char* user = next_arg(argc, argv, argi);
		const char* share = next_arg(argc, argv, argi);
		uint32 permissions = 0;
		while (argi < argc) {
			uint32 perms = 0;
			arg = next_arg(argc, argv, argi);
			if (!get_permissions(arg, &perms))
				print_usage_and_exit(true);
			permissions |= perms;
		}
		set_user_permissions(user, share, permissions);
	} else {
		print_usage_and_exit(true);
	}

	return 0;
}
