/*
 * Copyright 2011-2013, Axel Dörfler, axeld@pinc-software.de.
 * Distributed under the terms of the MIT License.
 */


#include <stdlib.h>

#include "Protocol.h"
#include "Response.h"

#include "argv.h"


struct cmd_entry {
	const char*	name;
	void		(*func)(int argc, char **argv);
	const char*	help;
};


static void do_help(int argc, char** argv);


extern const char* __progname;
static const char* kProgramName = __progname;

static IMAP::Protocol sProtocol;


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

	IMAP::SelectCommand command(folder);
	status_t status = sProtocol.ProcessCommand(command);
	if (status == B_OK) {
		printf("Next UID: %" B_PRIu32 ", UID validity: %" B_PRIu32 "\n",
			command.NextUID(), command.UIDValidity());
	} else
		error("select", status);
}


static void
do_folders(int argc, char** argv)
{
	IMAP::FolderList folders;
	BString separator;

	status_t status = sProtocol.GetFolders(folders, separator);
	if (status != B_OK) {
		error("folders", status);
		return;
	}

	for (size_t i = 0; i < folders.size(); i++) {
		printf(" %s %s\n", folders[i].subscribed ? "*" : " ",
			folders[i].folder.String());
	}
}


static void
do_fetch(int argc, char** argv)
{
	uint32 from = 1;
	uint32 to;
	uint32 flags = IMAP::kFetchAll;
	if (argc < 2) {
		printf("usage: %s [<from>] [<to>] [header|body]\n", argv[0]);
		return;
	}
	if (argc > 2) {
		if (!strcasecmp(argv[argc - 1], "header")) {
			flags = IMAP::kFetchHeader;
			argc--;
		} else if (!strcasecmp(argv[argc - 1], "body")) {
			flags = IMAP::kFetchBody;
			argc--;
		}
	}
	if (argc > 2) {
		from = atoul(argv[1]);
		to = atoul(argv[2]);
	} else
		from = to = atoul(argv[1]);

	IMAP::FetchCommand command(from, to, flags | IMAP::kFetchFlags);

	// A fetch listener that dumps everything to stdout
	class Listener : public IMAP::FetchListener {
	public:
		virtual ~Listener()
		{
		}

		virtual	bool FetchData(uint32 fetchFlags, BDataIO& stream,
			size_t& length)
		{
			fBuffer.SetSize(0);

			char buffer[65535];
			while (length > 0) {
				ssize_t bytesRead = stream.Read(buffer,
					min_c(sizeof(buffer), length));
				if (bytesRead <= 0)
					break;

				fBuffer.Write(buffer, bytesRead);
				length -= bytesRead;
			}

			// Null terminate the buffer
			char null = '\0';
			fBuffer.Write(&null, 1);

			return true;
		}

		virtual	void FetchedData(uint32 fetchFlags, uint32 uid, uint32 flags)
		{
			printf("================= UID %ld, flags %lx =================\n",
				uid, flags);
			puts((const char*)fBuffer.Buffer());
		}

	private:
		BMallocIO	fBuffer;
	} listener;

	command.SetListener(&listener);

	status_t status = sProtocol.ProcessCommand(command);
	if (status != B_OK) {
		error("fetch", status);
		return;
	}
}


static void
do_flags(int argc, char** argv)
{
	uint32 from = 1;
	uint32 to;
	if (argc < 2) {
		printf("usage: %s [<from>] [<to>]\n", argv[0]);
		return;
	}
	if (argc > 2) {
		from = atoul(argv[1]);
		to = atoul(argv[2]);
	} else
		to = atoul(argv[1]);

	IMAP::MessageEntryList entries;
	IMAP::FetchMessageEntriesCommand command(entries, from, to, true);
	status_t status = sProtocol.ProcessCommand(command);
	if (status != B_OK) {
		error("flags", status);
		return;
	}

	for (size_t i = 0; i < entries.size(); i++) {
		printf("%10lu %8lu bytes, flags: %#lx\n", entries[i].uid,
			entries[i].size, entries[i].flags);
	}
}


static void
do_noop(int argc, char** argv)
{
	IMAP::RawCommand command("NOOP");
	status_t status = sProtocol.ProcessCommand(command);
	if (status != B_OK)
		error("noop", status);
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

	class RawCommand : public IMAP::Command, public IMAP::Handler {
	public:
		RawCommand(const char* command)
			:
			fCommand(command)
		{
		}

		BString CommandString()
		{
			return fCommand;
		}

		bool HandleUntagged(IMAP::Response& response)
		{
			if (response.IsCommand(fCommand)) {
				printf("-> %s\n", response.ToString().String());
				return true;
			}

			return false;
		}

	private:
		const char* fCommand;
	};
	RawCommand rawCommand(command);
	status_t status = sProtocol.ProcessCommand(rawCommand);
	if (status != B_OK)
		error("raw", status);
}


static cmd_entry sBuiltinCommands[] = {
	{"select", do_select, "Selects a mailbox, defaults to INBOX"},
	{"folders", do_folders, "List of existing folders"},
	{"flags", do_flags,
		"List of all mail UIDs in the mailbox with their flags"},
	{"fetch", do_fetch,
		"Fetch mails via UIDs"},
	{"noop", do_noop, "Issue a NOOP command (will report new messages)"},
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
	uint16 port = useSSL ? 993 : 143;

	BNetworkAddress address(AF_INET, server, port);
	printf("Connecting to \"%s\" as %s%s, port %u\n", server, user,
		useSSL ? " with SSL" : "", address.Port());

	status_t status = sProtocol.Connect(address, user, password, useSSL);
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
