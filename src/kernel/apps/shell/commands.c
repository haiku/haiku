/*
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/

#include <syscalls.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "commands.h"
#include "file_utils.h"
#include "shell_defs.h"

struct command cmds[] = {
	{"exec", &cmd_exec},
	{"stat", &cmd_stat},
	{"mkdir", &cmd_mkdir},
	{"cat", &cmd_cat},
	{"cd", &cmd_cd},
	{"pwd", &cmd_pwd},
	{"kill", &cmd_kill},
	{"help", &cmd_help},
	{NULL, NULL}
};


int
cmd_exec(int argc, char *argv[])
{
	return cmd_create_proc(argc - 1,argv+1);
}


int
cmd_create_proc(int argc,char *argv[])
{
	bool must_wait=true;
	team_id pid;

	int  arg_len;
	char *tmp;
	char filename[SCAN_SIZE+1];

	if (argc < 1) {
		printf("not enough args to exec\n");
		return 0;
	}

	tmp =  argv[argc - 1];

	if( !find_file_in_path(argv[0],filename,SCAN_SIZE)){
		printf("can't find '%s' \n",argv[0]);
		return 0;
	}


	// a hack to support the unix '&'
	if(argc >= 1) {
		arg_len = strlen(tmp);
		if(arg_len > 0){
			tmp += arg_len -1;
			if(*tmp == '&'){
				if(arg_len == 1){
					argc --;
				} else {
					*tmp = 0;
				}
				must_wait = false;
			}
		}
	}

	pid = _kern_create_team(filename,filename, argv, argc, NULL, 0, 5);
	if (pid >= 0) {
		if (must_wait)
			_kern_wait_for_team(pid, NULL);
	} else {
		printf("Error: cannot execute '%s'\n", filename);
		return 0; // should be -1, but the shell would exit
	}

	return 0;
}


int
cmd_mkdir(int argc, char *argv[])
{
	int rc;

	if (argc < 2) {
		printf("not enough arguments to mkdir\n");
		return 0;
	}

	rc = _kern_create_dir(-1, argv[1], 0755);
	if (rc < 0) {
		printf("sys_mkdir() returned error: %s\n", strerror(rc));
	} else {
		printf("%s successfully created.\n", argv[1]);
	}

	return 0;
}


int
cmd_cat(int argc, char *argv[])
{
	int rc;
	int fd;
	char buf[257];

	if (argc < 2) {
		printf("not enough arguments to cat\n");
		return 0;
	}

	fd = open(argv[1], 0);
	if (fd < 0) {
		printf("cat: sys_open() returned error: %s!\n", strerror(fd));
		goto done_cat;
	}

	for(;;) {
		rc = read_pos(fd, -1, buf, sizeof(buf) - 1);
		if (rc <= 0)
			break;

		buf[rc] = '\0';
		printf("%s", buf);
	}
	close(fd);

done_cat:
	return 0;
}


int
cmd_cd(int argc, char *argv[])
{
	int rc;

	if (argc < 2) {
		printf("not enough arguments to cd\n");
		return 0;
	}

	rc = chdir(argv[1]);
	if (rc < 0)
		printf("cd: sys_setcwd() returned error: %s!\n", strerror(errno));

	return 0;
}


int
cmd_pwd(int argc, char *argv[])
{
	char buffer[B_PATH_NAME_LENGTH];

	if (getcwd(buffer, sizeof(buffer)) == NULL) {
		printf("cd: sys_getcwd() returned error: %s!\n", strerror(errno));
		return 0;
	}

	printf("%s\n", buffer);

	return 0;
}


int
cmd_stat(int argc, char *argv[])
{
	int rc;
	struct stat st;

	if (argc < 2) {
		printf("not enough arguments to stat\n");
		return 0;
	}

	rc = stat(argv[1], &st);
	if (rc >= 0) {
		printf("stat of file '%s': \n", argv[1]);
		printf("vnid 0x%x\n", (unsigned int)st.st_ino);
		printf("type %d\n", st.st_type);
		printf("size %d\n", (int)st.st_size);
	} else
		printf("stat failed for file '%s'\n", argv[1]);

	return 0;
}


int
cmd_kill(int argc, char *argv[])
{
	int rc;
	
	if (argc < 3) {
		printf("not enough arguments to kill\n");
		return 0;
	}
	rc = send_signal(atoi(argv[2]), atoi(argv[1]));
	if (rc)
		printf("kill failed\n");
	return 0;
}


int
cmd_help(int argc, char *argv[])
{
	printf("command list:\n\n");
	printf("exit : quits this copy of the shell\n");
	printf("exec <file> : load this file as a binary and run it\n");
	printf("mkdir <path> : makes a directory at <path>\n");
	printf("cd <path> : sets the current working directory at <path>\n");
	printf("ls <path> : directory list of <path>. If no path given it lists the current dir\n");
	printf("stat <file> : gives detailed file statistics of <file>\n");
	printf("help : this command\n");
	printf("cat <file> : dumps the file to stdout\n");
	printf("mount <path> <device> <fsname> : tries to mount <device> at <path>\n");
	printf("unmount <path> : tries to unmount at <path>\n");
	printf("kill <sig> <tid> : sends signal <sig> to thread <tid>\n");

	return 0;
}

