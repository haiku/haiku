#include <stdio.h>

#include <Application.h>
#include <Directory.h>
#include <FindDirectory.h>
#include <Path.h>
#include <image.h>
#include <DataIO.h>
#include <Message.h>

int main (int argc, char *argv[])
{
	image_id addon;
	char *transport;
	
	if (argc < 2) {
		printf("Usage: %s <transport add-on name>\n", argv[0]);
		return B_ERROR;
	}

	new BApplication("application/x-vnd-OBOS-print_transport_tester");

	transport = argv[1];

	printf("Looking for %s transport addon:", transport);

	// try first in user add-ons directory
	printf("In user add-ons directory...\n");

	BPath path;
	find_directory(B_USER_ADDONS_DIRECTORY, &path);
	path.Append("Print/transport");
	path.Append(transport);
	
	addon = load_add_on(path.Path());
	if (addon < 0) {
		// on failure try in system add-ons directory

		printf("In system add-ons directory...\n");
		find_directory(B_BEOS_ADDONS_DIRECTORY, &path);
		path.Append("Print/transport");
		path.Append(transport);

		addon = load_add_on(path.Path());
	}

	if (addon < 0) {
		// failed to load transport add-on
		printf("Failed to load %s print transport add-on!\n", transport);
		return B_ERROR;
	}
	
	printf("Loaded from %s\n", path.Path());

	// get init & exit proc
	BDataIO* (*init_proc)(BMessage*);
	void (*exit_proc)(void);
	
	get_image_symbol(addon, "init_transport", B_SYMBOL_TYPE_TEXT, (void **) &init_proc);
	get_image_symbol(addon, "exit_transport", B_SYMBOL_TYPE_TEXT, (void **) &exit_proc);

	if (init_proc == NULL || exit_proc == NULL) {
		// transport add-on has not the proper interface
		printf("Invalid print transport add-on API!\n");
		unload_add_on(addon);
		return B_ERROR;
	}

	printf("Initing %s: ", transport);

	// now, initialize the transport add-on
	// request BDataIO object from transport add-on
	BMessage msg('TRIN');
	msg.AddString("printer_file", "/boot/home/config/settings/printers/test");
	BDataIO *io = (*init_proc)(&msg);
	
	if (io) {
		printf("done.\nTransport parameters msg =>\n");
		msg.PrintToStream();
	} else
		printf("failed!\n");
	
	if (exit_proc) {
		printf("Exiting %s...\n", transport);
		(*exit_proc)();
	}
	
	unload_add_on(addon);
	printf("%s unloaded.\n", transport);

	return B_OK;
}