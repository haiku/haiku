#include <Application.h>
#include <Screen.h>

#include <stdio.h>
#include <string.h>

// TODO: very basic, will need to be extended
int main()
{
	BApplication app("application/x-vnd.BScreenTest");
	BScreen screen;
	display_mode mode;
	
	if (!screen.IsValid()) {
		printf("Invalid BScreen object\n");
		exit(-1);
	}
	
	status_t status = screen.GetMode(&mode);
	if (status < B_OK)
		printf("%s\n", strerror(status));
	else
		printf("width: %d, height: %d\n", mode.virtual_width, mode.virtual_height);
	
	printf("Screen frame: \n");
	screen.Frame().PrintToStream();
}