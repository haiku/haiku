#ifndef __STATEMENTS_H_
#define __STATEMENTS_H_

//#include <types.h>
#include "shell_defs.h"




void init_statements(void);
bool get_path(int no,char *name,int max_len);

void ste_error_to_str(int error,char *descr);

int parse_string(char *in);
int parse_info(scan_info *info);

#endif
