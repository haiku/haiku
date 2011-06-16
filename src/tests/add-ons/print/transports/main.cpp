#include <getopt.h>
#include <stdio.h>

#include <Application.h>
#include <DataIO.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <image.h>
#include <Message.h>
#include <Path.h>
#include <PrintTransportAddOn.h>

static struct option longopts[] = {
    { "help",		no_argument,		NULL, 'h' },
    { "verbose",	no_argument,		NULL, 'v' },
    { "list-ports",	no_argument,		NULL, 'p' },
    { NULL,		0,			NULL, 0   }
};

static int verbose = 0;


static void
usage(int argc, char** argv)
{
    fprintf(stderr,
		"Usage: %s [OPTIONS...] transport-addon-name [file to send]\n"
		"\n"
		"  -p, --list-ports   print ports detected by the transport add-on\n"
		"  -v, --verbose      tell me more. Use multiple times for more details\n"
		"  -h, --help         display this help and exit\n"
		"\n", argv[0]
);
}


int main (int argc, char *argv[])
{
	int c;
	bool list_ports = false;

	while ((c = getopt_long(argc, argv, "hvp", longopts, 0)) > 0) {
		switch (c) {
		case 'p':
	    	list_ports = true;
		    break;
		case 'v':
		    verbose++;
		    break;
		default:
		    usage(argc, argv);
		    return 0;
		}
	}
    argc -= optind;
    argv += optind;

    if (argc < 1) {
		usage(argc, argv);
		return -1;
    }

	new BApplication("application/x-vnd-Haiku-print_transport_tester");

	image_id addon = -1;
	char *transport = argv[0];

	printf("Looking for %s transport addon:\n", transport);

	directory_which which[] = {
		B_USER_NONPACKAGED_ADDONS_DIRECTORY,
		B_USER_ADDONS_DIRECTORY,
		B_COMMON_NONPACKAGED_ADDONS_DIRECTORY,
		B_COMMON_ADDONS_DIRECTORY,
		B_SYSTEM_ADDONS_DIRECTORY
	};
	BPath path;
	for (uint32 i = 0; i <sizeof(which) / sizeof(which[0]); i++) {
		if (find_directory(which[i], &path, false) != B_OK)
			continue;

		path.Append("Print/transport");
		path.Append(transport);

		printf("\t%s ?\n", path.Path());
		addon = load_add_on(path.Path());
		if (addon >= B_OK)
			break;
	}

	if (addon == B_ERROR) {
		// failed to load transport add-on
		printf("Failed to load \"%s\" print transport add-on!\n", transport);
		return -1;
	}

	printf("Add-on %d = \"%s\" loaded from %s.\n", (int) addon,
		transport, path.Path());

	// get init & exit proc
	BDataIO* (*transport_init_proc)(BMessage*) = NULL;
	void (*transport_exit_proc)(void) = NULL;
	status_t (*list_transport_ports)(BMessage*) = NULL;
	int* transport_features_ptr = NULL;

	get_image_symbol(addon, "init_transport", B_SYMBOL_TYPE_TEXT, (void **) &transport_init_proc);
	get_image_symbol(addon, "exit_transport", B_SYMBOL_TYPE_TEXT, (void **) &transport_exit_proc);

	get_image_symbol(addon, B_TRANSPORT_LIST_PORTS_SYMBOL, B_SYMBOL_TYPE_TEXT,
		(void **) &list_transport_ports);
	get_image_symbol(addon, B_TRANSPORT_FEATURES_SYMBOL, B_SYMBOL_TYPE_DATA,
		(void **) &transport_features_ptr);

	if (transport_init_proc == NULL || transport_exit_proc == NULL) {
		// transport add-on has not the proper interface
		printf("Invalid print transport add-on API!\n");
		unload_add_on(addon);
		return B_ERROR;
	}

	if (list_ports) {
		printf("Ports list:\n");

		if (list_transport_ports == NULL)
			printf("Transport \"%s\" don't support this feature!\n", transport);
		else {
			BMessage ports;
			snooze(1000000);	// give some time for ports discovery
			status_t status = (*list_transport_ports)(&ports);
			if (status == B_OK)
				ports.PrintToStream();
			else
				printf("failed!\n");
		}
	}

	printf("Initing %s: ", transport);

	// now, initialize the transport add-on
	// request BDataIO object from transport add-on
	BMessage msg('TRIN');
	// TODO: create on the fly a temporary printer folder for testing purpose only
	msg.AddString("printer_file", "/boot/home/config/settings/printers/test");
	BDataIO *io = (*transport_init_proc)(&msg);

	if (io) {
		printf("done.\nTransport parameters msg =>\n");
		msg.PrintToStream();
	} else
		printf("failed!\n");


	if (argc > 1) {
		BFile data(argv[1], B_READ_ONLY);
		if (data.InitCheck() == B_OK) {
			uint8 buffer[B_PAGE_SIZE];
			ssize_t total = 0;
			ssize_t sz;

			printf("Sending data read from %s file...\n", argv[2]);
			while((sz = data.Read(buffer, sizeof(buffer))) > 0) {
				if (io->Write(buffer, sz) < 0) {
					printf("Error writting on the print transport stream!\n");
					break;
				}
				total += sz;
			}	// while
			printf("%ld data bytes sent.\n", total);
		}	// data valid file
	}	// optional data file

	if (transport_exit_proc) {
		printf("Exiting %s...\n", transport);
		(*transport_exit_proc)();
	}

	unload_add_on(addon);
	printf("%s unloaded.\n", transport);

	return B_OK;
}

