/*
 * Copyright 2011, Joseph "looncraz" Groover, looncraz@satx.rr.com
 * Copyright 2007, François Revol, revol@free.fr.
 * Distributed under the terms of the MIT license.
 */


#include <stdio.h>

#include <Application.h>
#include <Bitmap.h>
#include <InterfaceDefs.h>
#include <String.h>
#include <View.h>
#include <Window.h>

#include <DecorInfo.h>


void
print_decor_info_header()
{
	printf("    Name      License\t    Description\n");
	printf("----------------------------------------------------\n");
}


void
print_decor_summary(DecorInfo* decor, bool isCurrent)
{
	if (isCurrent)
		printf("*");

	printf("%-12s\t%-8s  %-30s\n", decor->Name().String(),
		decor->LicenseName().String(), decor->ShortDescription().String());
}


void
print_decor_shortcut(DecorInfo* decor, bool isCurrent)
{
	if (isCurrent)
		printf("*");

	printf("%-12s\t%-12s\n", decor->ShortcutName().String(),
		decor->Name().String());
}


void
print_decor_info_verbose(DecorInfo* decor, bool isCurrent)
{
	printf("Name:\t\t%s\n", decor->Name().String());
	printf("Version:\t%f\n", decor->Version());
	printf("Author(s):\t%s\n", decor->Authors().String());
	printf("Description:\t%s\n", decor->ShortDescription().String());
	printf("License:\t%s (%s)\n", decor->LicenseName().String(),
		decor->LicenseURL().String());
	printf("Support URL:\t%s\n", decor->SupportURL().String());
	printf("%s\n", isCurrent ? "Currently in use." : "Currently not in use.");
}


int
main(int argc, char** argv)
{
	if (argc < 2) {
		printf("usage: %s [-l|-c|decorname]\n", argv[0]);
		printf("\t-l: list available decors\n");
		printf("\t-s: list shortcut names for available decors\n");
		printf("\t-c: give current decor name\n");
		printf("\t-i: detailed information about decor\n");
		printf("\t-p: see preview window\n");
		return 1;
	}

	// combine remaining args into one string:
	BString decoratorName;
	for (int i = 2; i < argc; ++i)
		decoratorName << argv[i] << " ";
	decoratorName.RemoveLast(" ");

	BApplication app("application/x-vnd.Haiku-setdecor");

	DecorInfoUtility* util = new DecorInfoUtility();
	DecorInfo* decor = NULL;

	if (util == NULL) {
		fprintf(stderr, "error instantiating DecoratorInfoUtility (out of"
			" memory?)\n");
		return 1;
	}

	// we want the list
	if (!strcmp(argv[1], "-l")) {
		// Print default decorator:
		print_decor_info_header();
		int32 count = util->CountDecorators();
		for (int32 i = 0; i < count; ++i) {
			decor = util->DecoratorAt(i);
			if (decor == NULL) {
				fprintf(stderr,
					"error NULL entry @ %" B_PRIi32 " / %" B_PRIi32
					" - BUG BUG BUG\n",
					i, count);
				// return 2 to track DecorInfoUtility errors
				return 2;
			}
			print_decor_summary(decor, util->IsCurrentDecorator(decor));
		}

		return 0;
	}

	// we want the current decorator
	if (!strcmp(argv[1], "-c")) {
		decor = util->CurrentDecorator();

		if (decor == NULL) {
			fprintf(stderr, "Unable to determine current decorator, sorry! - "
				"BUG BUG BUG\n");
			return 2;
		}

		print_decor_info_header();
		print_decor_summary(decor, true);
		return 0;
	}


	if (!strcmp(argv[1], "-s")) {

		printf("  Shortcut        Name\n");
		printf("------------------------------------\n");

		int32 count = util->CountDecorators();
		for (int32 i = 0; i < count; ++i) {
			decor = util->DecoratorAt(i);
			if (decor == NULL) {
				fprintf(stderr,
					"error NULL entry @ %" B_PRIi32 " / %" B_PRIi32
					" - BUG BUG BUG\n",
					i, count);
				// return 2 to track DecorInfoUtility errors
				return 2;
			}
			print_decor_shortcut(decor, util->IsCurrentDecorator(decor));
		}

		return 0;
	}

	// we want detailed information for a specific decorator ( by name or path )
	if (!strcmp(argv[1], "-i")) {
		if (argc < 3) {
			fprintf(stderr, "not enough arguments\n");
			return 1;
		}

		decor = util->FindDecorator(decoratorName.String());

		if (decor == NULL) {
			fprintf(stderr, "Can't find decor named \"%s\", try again\n",
				decoratorName.String());
			return 1;
		}

		print_decor_info_verbose(decor, util->IsCurrentDecorator(decor));
		return 0;
	}


	if (!strcmp(argv[1], "-p")) {
		if (argc < 3) {
			fprintf(stderr, "not enough arguments\n");
			return 1;
		}

		decor = util->FindDecorator(decoratorName.String());

		if (decor == NULL) {
			fprintf(stderr, "Can't find decor named \"%s\", try again\n",
				decoratorName.String());
			return 1;
		}

		printf("Preparing preview...\n");

		BWindow* previewWindow = new BWindow(BRect(150, 150, 390, 490),
			decor->Name().String(), B_TITLED_WINDOW, B_NOT_ZOOMABLE
				| B_QUIT_ON_WINDOW_CLOSE | B_NOT_RESIZABLE );

		previewWindow->AddChild(new BView(previewWindow->Bounds(), "",
			B_FOLLOW_ALL, 0));

		if (util->Preview(decor, previewWindow) != B_OK) {
			fprintf(stderr, "Unable to preview decorator, sorry!\n");
			// TODO: more detailed error...
			return 1;
		}

		previewWindow->Show();

		app.Run();
		return 0;
	}

	// we want to change it
	decoratorName = "";
	for (int i = 1; i < argc; ++i)
		decoratorName << argv[i] << " ";
	decoratorName.RemoveLast(" ");

	decor = util->FindDecorator(decoratorName.String());

	if (decor == NULL) {
		fprintf(stderr, "no such decorator \"%s\"\n", decoratorName.String());
		return 1;
	}

	if (util->IsCurrentDecorator(decor)) {
		printf("\"%s\" is already the current decorator\n",
			decor->Name().String());
		return 0;
	}

	printf("Setting %s as the current decorator...\n", decor->Name().String());
	if (util->SetDecorator(decor) != B_OK ) {
		fprintf(stderr, "Unable to set decorator, sorry\n\n");
		return 1;	// todo more detailed error...
	}

	return 0;
}

