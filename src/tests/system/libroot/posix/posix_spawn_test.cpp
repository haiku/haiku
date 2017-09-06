#include <errno.h>
#include <spawn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>


int main()
{

	char* _args[4];
	char* _env[] = { "myenv=5", NULL };

	_args[0] = "bash";
	_args[1] = "-c";
	_args[2] = "exit $myenv";
	_args[3] = NULL;

	pid_t pid;
	int err = posix_spawnp(&pid, _args[0], NULL, NULL, _args, _env);
	printf("posix_spawnp: %d, %d\n", err, pid);

	int status;
	pid_t waitpid_res = waitpid(pid, &status, 0);
	if (waitpid_res != pid)
		printf("posix_spawnp: waitpid didn't return pid\n");

	printf("posix_spawnp: WIFEXITED(): %d, WEXITSTATUS() %d => 5\n",
		WIFEXITED(status), WEXITSTATUS(status));

	_args[0] = "/tmp/toto";
	_args[1] = NULL;

	err = posix_spawn(&pid, _args[0], NULL, NULL, _args, _env);
	printf("posix_spawn: %d, %d\n", err, pid);

	if (err == 0) {
		waitpid_res = waitpid(pid, &status, 0);
		if (waitpid_res != pid)
			printf("posix_spawn: waitpid didn't return pid\n");
		printf("posix_spawn: WIFEXITED(): %d, WEXITSTATUS() %d => 127\n",
			WIFEXITED(status), WEXITSTATUS(status));
	} else {
		waitpid_res = waitpid(-1, NULL, 0);
		printf("posix_spawn: waitpid %d, %d\n", waitpid_res, errno);
	}

	return 0;
}

