//#include <types.h>
#include <string.h>
#include <ctype.h>
#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>
#include <Errors.h>
#include <fcntl.h>

#include "file_utils.h"
#include "statements.h"
#include "shell_defs.h"

bool combine_path(const char *path1,const char *path2,char *out,unsigned int max_len)
{
	unsigned int cur_len = 0;

	cur_len = strlen(path1);
	if(cur_len > max_len) return false;
	strcpy(out,path1);

	if(*path1 != 0){
		if(out[cur_len-1] != '/'){
			if(cur_len >= max_len) return false;
			out[cur_len] = '/';
			out[cur_len+1] = 0;
			cur_len++;
		}
	}
	if(strlen(path2)+cur_len > max_len) return false;
	strcat(out,path2);
	return true;
}

bool exists_file(const char *file_name)
{
	int handle = sys_open(file_name,STREAM_TYPE_FILE,0);
	bool exists;
	exists =( handle >= 0);
	if(exists) sys_close(handle);
	return exists;
}

bool find_file_in_path(const char *file_name,char *found_name,unsigned int max_size)
{
	char path[SCAN_SIZE+1];
	int  cnt=0;

	if(strchr(file_name,'/') != NULL){
		strncpy(found_name,file_name,max_size);
		found_name[max_size] = 0;
		if(exists_file(found_name)) return true;
		found_name[0] = 0;
		return false;
	}
	while(get_path(cnt,path,SCAN_SIZE)){
		if(combine_path(path,file_name,found_name,max_size)){
			if(exists_file(found_name)) return true;
		}
		cnt++;
	}
	found_name[0] =0;
	return(false);
}


int exec_file(int argc,char *argv[],int *retcode)
{
	char filename[255];
	int pid;

	if( !find_file_in_path(argv[0],filename,SCAN_SIZE)) return SHE_FILE_NOT_FOUND;

	pid = sys_proc_create_proc(filename,filename, argv, argc, 5);

    if(pid < 0) return SHE_CANT_EXECUTE;

	sys_proc_wait_on_proc(pid, retcode);

	return SHE_NO_ERROR;
}


int read_file_in_buffer(const char *filename,char **buffer)
{
	struct stat stat;
	int file_no;
	int err;
	int size;

	*buffer = NULL;

	file_no = open(filename, O_RDONLY);
	if (file_no < 0){
		return file_no;
	}

	err = fstat(file_no,&stat);
	if (err < 0)
		return err;

	*buffer = malloc(stat.st_size + 1);
	if(*buffer == NULL) return ERR_NO_MEMORY;

	size = sys_read(file_no,*buffer,0,stat.st_size);

	sys_close(file_no);

	if (size < 0)
		free(*buffer);

	return size;
}
