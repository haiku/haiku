#ifndef _shell_var_h_
#define _shell_vars_h_

//#include <types.h>

struct _shell_value{
	bool isnumber;
   union{
		char *val_text;
		long int val_number;
	} value;
};

typedef struct _shell_value shell_value;

char *shell_strdup(const char *some_str);
shell_value *get_value_by_name(const char *name);

int shell_var_set_number(const char *var_name,long int number);
int shell_var_set_text(const char *var_name,const char *text);

int  set_shell_var_with_value(const char *var_name,shell_value *value);

void init_vars(void);


void shell_value_free(shell_value *value);
int         shell_value_do_operation(shell_value *out,const shell_value *other,int oper_type);
int         shell_value_neg(shell_value *out);
char *shell_value_to_char(shell_value *value);
shell_value *shell_value_init_number(long int value);
shell_value *shell_value_init_text(const char *value);
int shell_value_to_number(shell_value *value,long int *number);
int shell_value_set_text(shell_value *value,const char *new_value);
int shell_value_set_number(shell_value *value,long int new_value);
int shell_value_get_number(shell_value *value,long int *number);
int shell_value_get_text(shell_value * value,char **text);
shell_value *shell_value_clone(shell_value *var);


bool 		string_to_long_int(const char *str,long int *out);


#endif

