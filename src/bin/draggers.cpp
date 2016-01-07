/*
 * draggers - show/hide draggers from CLI
 * (c) 2004, François Revol - revol@free.fr
 */

#include <stdio.h>
#include <string.h>
#include <Application.h>
#include <Dragger.h>


int usage(int ret)
{
	fprintf(stderr, "draggers [show|hide]\n");
	fprintf(stderr, "Shows/sets draggers state\n");
	return ret;
}


int main(int argc, char **argv)
{
	int i;
	BApplication app("application/x-vnd.Haiku-draggers");
	if (argc < 2) {
		printf("%s\n", BDragger::AreDraggersDrawn()?"shown":"hidden");
		return 0;
	}
	for (i = 1; i < argc; i++) {
		if (!strncmp(argv[i], "-h", 2)) {
			return usage(0);
		}
		if (!strcmp(argv[i], "1")
		 || !strncmp(argv[i], "en", 2)
		 || !strncmp(argv[i], "sh", 2)
		 || !strncmp(argv[i], "on", 2))
			BDragger::ShowAllDraggers();
		else if (!strcmp(argv[i], "0")
		 || !strncmp(argv[i], "di", 2)
		 || !strncmp(argv[i], "hi", 2)
		 || !strncmp(argv[i], "of", 2))
			BDragger::HideAllDraggers();
		else
			return usage(1);
	}
	return 0;
}
