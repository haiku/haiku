//#include <types.h>
#include <string.h>
#include <ctype.h>
#include <syscalls.h>
#include <stdio.h>

#include "args.h"
#include "shell_defs.h"
#include "shell_vars.h"

bool af_exit_after_script;
char *af_script_file_name;


static void handle_option(char *option){

	switch(option[1]){
		case 's':// setup script
			if(option[2] == 0){
				af_exit_after_script = false;
			}
	}

	printf("wrong option : %s \n",option);
}

void init_arguments(int argc,char **argv){

	int cnt = 1;
	int shell_argc = 0;
	char name[255];

   af_exit_after_script = true;
	af_script_file_name  = NULL;

	while((cnt < argc) && (argv[cnt][0] == '-')){
		handle_option(argv[cnt]);
		cnt++;
	}

	if(cnt < argc){

		af_script_file_name = argv[cnt];
		cnt++;

		while(cnt < argc){

			sprintf(name,NAME_PAR_PRFX,cnt-1);
			shell_var_set_text(name,argv[cnt]);
			shell_argc++;
			cnt++;
		}

		shell_var_set_number(NAME_VAR_ARGC,shell_argc);

	}


}
