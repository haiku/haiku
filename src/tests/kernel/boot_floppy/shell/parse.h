#ifndef _parse_h_
#define _parse_h_
#include "shell_defs.h"
#define IS_IDENT_CHAR(c) isalnum(c) || (c == '_')




int  shell_parse(const char *buf, int len);
bool scan(scan_info *info);
int init_scan_info_by_file(const char *file_name,scan_info *info);
void init_scan_info(const char*in,scan_info *info);
bool scan_info_home(scan_info *info);
bool scan_info_next_line(scan_info *info);

bool expect(scan_info *info,int check);
void parse_vars_in_string(const char *string,char *out,int max_len);
int  parse_line(const char *buf, char *argv[], int max_args, char *redirect_in, char *redirect_out);
bool set_scan_info_line(scan_info *info);
char *id_to_token(int id);

#endif
