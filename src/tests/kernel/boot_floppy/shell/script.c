#include <syscalls.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <Errors.h>
#include <stdlib.h>

#include "statements.h"
#include "file_utils.h"
#include "shell_defs.h"
#include "script.h"
#include "parse.h"
#include "statements.h"

static int add_string_to_list(text_file *data,char *text)
{
	text_item *item;

	item = malloc(sizeof(text_item));
	if(item == NULL) return SHE_NO_MEMORY;

	item->text = text;
	item->next = NULL;

	if(data->top != NULL) data->top->next = item;
	if(data->list == NULL) data->list = item;

	data->top = item;

	return SHE_NO_ERROR;
}

void  free_text_file(text_file *data)
{
	text_item *current;
	text_item *next;

	current = data->list;

	while(current != NULL){

		next = current->next;
		free(current);
		current = next;

	}

	free(data->buffer);
	data->buffer = NULL;
	data->list   = NULL;
	data->top    = NULL;
}


int  read_text_file(const char *filename,text_file *data)
{
	char *scan;
	char *old;
	int err;
	int size;
	char realfilename[256];

	if( !find_file_in_path(filename,realfilename,SCAN_SIZE)){

		printf("can't find '%s' \n",filename);
		return SHE_FILE_NOT_FOUND;

	}

	size = read_file_in_buffer(realfilename,&(data->buffer));

	if(size < 0) return size;

	scan  = data->buffer;
	data->top = NULL;
	data->list = NULL;

	while(size >0){

		err = add_string_to_list(data, scan);
		if(err <0) goto err;

		old = scan;

		while((size > 0) && (*scan != 10)) {
			scan++;
			size--;
		}

		*scan=0;
		scan++;
		size--;
	}
	return B_NO_ERROR;
err:
	printf("error\n");
	free_text_file(data);
	return err;
}


int run_script(const char *file_name)
{
	scan_info info;
	int err = init_scan_info_by_file(file_name,&info);
	if(err != SHE_NO_ERROR) return err;

	while(info.current != NULL){

		err = parse_info(&info);
		if(err != SHE_NO_ERROR) break;

		if(scan_info_next_line(&info)) {
			err = SHE_SCAN_ERROR;
			break;
		}

	}

//err:
	free_text_file(&(info.data));
	return err;
}

