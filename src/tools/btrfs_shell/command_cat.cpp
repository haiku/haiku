#include <stdio.h>
#include <stdlib.h>

#include "syscalls.h"
#include "system_dependencies.h"


namespace FSShell {


fssh_status_t
command_cat(int argc, const char* const* argv)
{
	long numBytes = 10;
	int fileStart = 1;
	if (argc < 2 || strcmp(argv[1], "--help") == 0) {
		printf(
			"Usage: %s [ -n ] [FILE]...\n"
			"\t -n\tNumber of bytes to read\n",
			argv[0]
		);
		return FSSH_B_OK;
	}

	if (argc > 3 && strcmp(argv[1], "-n") == 0) {
		fileStart += 2;
		numBytes = strtol(argv[2], NULL, 10);
	}

	const char* const* files = argv + fileStart;
	for (; *files; files++) {
		const char* file = *files;
		int fd = _kern_open(-1, file, O_RDONLY, O_RDONLY);
		if (fd < 0) {
			fssh_dprintf("Error: %s\n", fssh_strerror(fd));
			return FSSH_B_BAD_VALUE;
		}

		char buffer[numBytes + 1];
		if (buffer == NULL) {
			fssh_dprintf("Error: No memory\n");
			_kern_close(fd);
			return FSSH_B_NO_MEMORY;
		}

		if (_kern_read(fd, 0, buffer, numBytes) != numBytes) {
			fssh_dprintf("Error: fail to read, length: %i\n", numBytes);
			_kern_close(fd);
			return FSSH_B_BAD_VALUE;
		}

		_kern_close(fd);
		buffer[numBytes] = '\0';
		printf("%s\n", buffer);
	}

	return FSSH_B_OK;
}


}	// namespace FSShell
