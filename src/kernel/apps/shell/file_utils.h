#ifndef _file_utils_h_
#define _file_utils_h_

bool combine_path(const char *path1,const char *path2,char *out,unsigned int max_len);
bool exists_file(const char *file_name);
bool find_file_in_path(const char *file_name,char *found_name,unsigned int max_size);
int exec_file(int argc,char *argv[],int *retcode);
int read_file_in_buffer(const char *filename,char **buffer);

#endif
