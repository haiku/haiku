#ifndef _COMMANDS_H
#define _COMMANDS_H

int cmd_exit(int argc, char *argv[]);
int cmd_exec(int argc, char *argv[]);
int cmd_stat(int argc, char *argv[]);
int cmd_mkdir(int argc, char *argv[]);
int cmd_cat(int argc, char *argv[]);
int cmd_cd(int argc, char *argv[]);
int cmd_pwd(int argc, char *argv[]);
int cmd_help(int argc, char *argv[]);
int cmd_create_proc(int argc,char *argv[]);
typedef int(cmd_handler_proc)(int argc,char *argv[]);

struct command {
	char *cmd_text;
	cmd_handler_proc *cmd_handler;
};

extern struct command cmds[];
#endif
