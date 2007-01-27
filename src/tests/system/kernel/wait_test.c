/*
*  Copyright 2007, Jérôme Duval. All rights reserved.
*  Distributed under the terms of the MIT License.
*  
*/
#include <sys/wait.h>
#include <stdio.h>

/**
 * wait() doesn't return on Haiku, whereas it should return an error when the
 * process has no children.
 */

int main() {
	int child_status;
	pid_t pid = wait(&child_status);
	printf("wait returned %ld\n", pid);
}

