// dump_mime_types.cpp

#include <stdio.h>
#include <string.h>
#include <sys/utsname.h>

#include <Application.h>
#include <Message.h>
#include <Mime.h>

// main
int
main()
{
	// create a BApplication on BeOS
	struct utsname unameInfo;
	if (uname(&unameInfo) < 0 || strcmp(unameInfo.sysname, "Haiku") != 0)
		new BApplication("application/x-vnd.haiku.dump-mime-types");

	// get the list of installed types
	BMessage installedTypes;
	status_t error = BMimeType::GetInstalledTypes(&installedTypes);
	if (error != B_OK) {
		fprintf(stderr, "Failed to get installed types: %s\n", strerror(error));
		exit(1);
	}

	// print the types (including some additional info)
	const char *type;
	for (int i = 0; installedTypes.FindString("types", i, &type) == B_OK; i++) {
		printf("%s:\n", type);

		// get mime type
		BMimeType mimeType;
		error = mimeType.SetTo(type);
		if (error != B_OK) {
			printf("  failed to init type: %s\n", strerror(error));
			continue;
		}

		// preferred app
		char preferredApp[B_MIME_TYPE_LENGTH];
		if (mimeType.GetPreferredApp(preferredApp) == B_OK)
			printf("  preferred app:     %s\n", preferredApp);
		
		// short description
		char shortDescription[256];
		if (mimeType.GetShortDescription(shortDescription) == B_OK)
			printf("  short description: %s\n", shortDescription);

		// long description
		char longDescription[256];
		if (mimeType.GetLongDescription(longDescription) == B_OK)
			printf("  long description:  %s\n", longDescription);

		// extensions
		BMessage extensions;
		if (mimeType.GetFileExtensions(&extensions) == B_OK) {
			printf("  extensions:        ");
			const char *extension;
			for (int k = 0;
				 extensions.FindString("extensions", k, &extension) == B_OK;
				 k++) {
				if (k > 0)
					printf(" ");
				printf("%s", extension);
			}
			printf("\n");
		}

		// supporting apps
		BMessage supportingApps;
		if (mimeType.GetSupportingApps(&supportingApps) == B_OK) {
			const char *app;
			for (int k = 0;
				 supportingApps.FindString("applications", k, &app) == B_OK;
				 k++) {
				if (k == 0)
					printf("  supporting apps:   ");
				else
					printf("                     ");
				printf("%s\n", app);
			}
		}
		
		// attr info
		BMessage attrInfo;
		if (mimeType.GetAttrInfo(&attrInfo) == B_OK) {
			printf("  attributes:\n");
			const char *name;
			const char *publicName;
			type_code type;
			bool isViewable;
			bool isPublic;
			bool isEditable;
			for (int k = 0;
				 attrInfo.FindString("attr:name", k, &name) == B_OK
				 && (attrInfo.FindString("attr:public_name", k,
				 	&publicName) == B_OK || (publicName = name, true))
				 && (attrInfo.FindInt32("attr:type", k, (int32*)&type) == B_OK
				 	|| (type = '____', true))
				 && (attrInfo.FindBool("attr:viewable", k, &isViewable) == B_OK
				 	|| (isViewable = false, true))
				 && (attrInfo.FindBool("attr:public", k, &isPublic) == B_OK
				 	|| (isPublic = isViewable, true))
				 && (attrInfo.FindBool("attr:editable", k, &isEditable) == B_OK
				 	|| (isEditable = false, true));
				 k++) {
				printf("    `%s' (`%s')\n", name, publicName);
				printf("      type:     %c%c%c%c (0x%lx)\n", char(type >> 24),
					char(type >> 16), char(type >> 8), char(type), type);
				printf("      public:   %s\n", (isPublic ? "true" : "false"));
				printf("      editable: %s\n", (isEditable ? "true" : "false"));
			}
		}
	}

	return 0;
}
