#include <stdio.h>
#include <unistd.h>

#include "../FinePixUSBKit/FinePix.h"

int main ()
{
	FinePix* test = new FinePix();
	uint8 *frame = new uint8[100000]; // Max frame size 100k
	int total_size = 0;
	
	while(1) 
	{	// Wait around and let the (usb)roster do its thing
		snooze(1000000); 
		if (test->InitCheck() == B_OK) 
		{
			fprintf(stderr, "Camera ready\n");
			test->SetupCam();
			for (int i=0; i<1; i++) { // Number of frames to get	
				// Get a frame
				test->GetPic(frame, total_size);
				// Save the frame
				//fprintf(stderr,"This frame was %d bytes\n", total_size);
				char fname[100];
				sprintf(fname, "frame-%05d.jpg", i);
				int fd = open(fname, O_WRONLY | O_CREAT,0644);
				write(fd, frame, total_size);
				close(fd);
				fprintf(stderr,"Saved as file:%s \n", fname);
				
				/* Wait before requesting next frame, 
				 * for 30 fps wait less than 33333ms (1 sek / 30) */
				snooze(30000);
			}	
			break;
		} else {
			fprintf(stderr, "Camera not ready\n");
		}
	}
	
	delete test;
	return 0;
}
