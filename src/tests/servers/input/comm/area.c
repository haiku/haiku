#include <stdio.h>
#include <stdlib.h>
#include <OS.h>

int main(int count, char **args) {

	sem_id cursorSem = atoi(args[2]);
	area_id appArea = atoi(args[1]);
	void *appBuffer;
	area_id newArea;

	acquire_sem(cursorSem);

	newArea = clone_area("isClone", &appBuffer, B_ANY_ADDRESS, B_READ_AREA|B_WRITE_AREA, appArea); 
	if (newArea > 0) {
		int fd = open ("/tmp/input_area.bin", O_WRONLY | O_CREAT | O_TRUNC, 0644);
                if (fd >= 0) {
	                write (fd, appBuffer, 0x1000);
                        close (fd);
                }
                printf("success when writing area %ld\n", appArea);

		delete_area(newArea);
	}

	return 0;
}
