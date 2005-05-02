#include <Application.h>
#include <Screen.h>

#include <stdio.h>
#include <string.h>

int main()
{
	BApplication app("application/x-vnd.BScreenTest");
	BScreen screen;
	
	if (!screen.IsValid()) {
		printf("Invalid BScreen object\n");
		exit(-1);
	}
	
	display_mode oldMode;
	status_t status = screen.GetMode(&oldMode);
	if (status < B_OK)
		printf("%s\n", strerror(status));
	else
		printf("width: %d, height: %d, space: %lu\n",
			 oldMode.virtual_width,
			 oldMode.virtual_height,
			 oldMode.space);
	
	printf("Screen frame: ");
	screen.Frame().PrintToStream();
	
	printf("\nTrying to set mode to 800x600: ");
	display_mode newMode = oldMode;
	newMode.virtual_width = 800;
	newMode.virtual_height = 600;
	
	status = screen.SetMode(&newMode);
	if (status < B_OK)
		printf("FAILED (%s)\n", strerror(status));
	else {
		printf("OK\n");
		status_t status = screen.GetMode(&newMode);
		if (status < B_OK)
			printf("%s\n", strerror(status));
		else {
			printf("width: %d, height: %d, space: %lu\n",
			 	newMode.virtual_width,
			 	newMode.virtual_height,
			 	newMode.space);
			printf("Screen frame: ");
			screen.Frame().PrintToStream();
			printf("\n");
		}
	}
	
	snooze(4000000);
	
	printf("resetting video mode: ");
	status = screen.SetMode(&oldMode);
	if (status < B_OK)
		printf("FAILED (%s)\n", strerror(status));
	else
		printf("OK\n");
}
