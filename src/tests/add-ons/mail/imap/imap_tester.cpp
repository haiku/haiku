#include "IMAPFolders.h"
#include "IMAPMailbox.h"
#include "IMAPStorage.h"

#include "argv.h"


struct cmd_entry {
	char*	name;
	void	(*func)(int argc, char **argv);
	char*	help;
};


static void do_help(int argc, char** argv);


extern const char* __progname;
static const char* kProgramName = __progname;

static IMAPStorage sStorage;
static IMAPMailbox sMailbox(sStorage);


static void
error(const char* context, status_t status)
{
	fprintf(stderr, "Error during %s: %s\n", context, strerror(status));
}


static void
usage()
{
	printf("Usage: %s <server> <username> <password>\n", kProgramName);
	exit(1);
}


// #pragma mark -


static void
do_select(int argc, char** argv)
{
	const char* folder = "INBOX";
	if (argc > 1)
		folder = argv[1];

	status_t status = sMailbox.SelectMailbox(folder);
	if (status != B_OK)
		error("select", status);
}


static void
do_folders(int argc, char** argv)
{
	IMAPFolders folder(sMailbox);
	FolderList folders;
	status_t status = folder.GetFolders(folders);
	if (status != B_OK)
		error("folders", status);

	for (size_t i = 0; i < folders.size(); i++) {
		printf(" %s %s\n", folders[i].subscribed ? "*" : " ",
			folders[i].folder.String());
	}
}


static void
do_raw(int argc, char** argv)
{
	// build command back again
	char command[4096];
	command[0] = '\0';

	for (int i = 1; i < argc; i++) {
		if (i > 1)
			strlcat(command, " ", sizeof(command));
		strlcat(command, argv[i], sizeof(command));
	}

	class RawCommand : public IMAPCommand {
	public:
		RawCommand(const char* command)
			:
			fCommand(command)
		{
		}

		BString Command()
		{
			return fCommand;
		}

		bool Handle(const BString& response)
		{
			return false;
		}

	private:
		const char* fCommand;
	};
	RawCommand rawCommand(command);
	status_t status = sMailbox.ProcessCommand(&rawCommand, 60 * 1000);
	if (status != B_OK)
		error("raw", status);
}


static cmd_entry sBuiltinCommands[] = {
	{"select", do_select, "Selects a mailbox, defaults to INBOX"},
	{"folders", do_folders, "List of existing folders"},
	{"raw", do_raw, "Issue a raw command to the server"},
	{"help", do_help, "prints this help text"},
	{"quit", NULL, "exits the application"},
	{NULL, NULL, NULL},
};


static void
do_help(int argc, char** argv)
{
	printf("Available commands:\n");

	for (cmd_entry* command = sBuiltinCommands; command->name != NULL;
			command++) {
		printf("%8s - %s\n", command->name, command->help);
	}
}


// #pragma mark -


int
main(int argc, char** argv)
{
	if (argc < 4)
		usage();

	const char* server = argv[1];
	const char* user = argv[2];
	const char* password = argv[3];
	bool useSSL = argc > 4;
	uint16 port = useSSL ? 995 : 143;

	printf("Connecting to \"%s\" as %s\n", server, user);

	status_t status = sMailbox.Connect(server, user, password, useSSL, port);
	if (status != B_OK) {
		error("connect", status);
		return 1;
	}

	while (true) {
		printf("> ");
		fflush(stdout);

		char line[1024];
		if (fgets(line, sizeof(line), stdin) == NULL)
			break;

        argc = 0;
        argv = build_argv(line, &argc);
        if (argv == NULL || argc == 0)
            continue;

		if (!strcmp(argv[0], "quit")
			|| !strcmp(argv[0], "exit")
			|| !strcmp(argv[0], "q"))
			break;

        int length = strlen(argv[0]);
		bool found = false;

		for (cmd_entry* command = sBuiltinCommands; command->name != NULL;
				command++) {
			if (!strncmp(command->name, argv[0], length)) {
				command->func(argc, argv);
				found = true;
				break;
			}
		}

		if (!found) {
			fprintf(stderr, "Unknown command \"%s\". Type \"help\" for a "
				"list of commands.\n", argv[0]);
		}

		free(argv);
	}


	return 0;
}
