#ifndef _script_h_

#define _script_h_

#include <ctype.h>

struct _text_item{
        struct _text_item *next;
		char *text;
};

typedef struct _text_item text_item;


struct _text_file{
	char *buffer;
	text_item *list;
	text_item *top;
};

typedef struct _text_file text_file;


int  read_text_file(const char *filename,text_file *data);
void  free_text_file(text_file *data);
int run_script(const char *file_name);
#endif
