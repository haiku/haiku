//#include <types.h>
#include <string.h>
#include <ctype.h>
#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>

#include "shell_vars.h"
#include "statements.h"
#include "shell_defs.h"


struct _shell_var{
	struct _shell_var *next;
	char *name;
	shell_value *value;
}
;

typedef struct _shell_var shell_var;

static shell_var *var_list;

char *shell_strdup(const char *some_str)
{
	char *result;

	result = malloc(strlen(some_str) + 1);

	if(result != NULL){
		strcpy(result,some_str);
	}

	return result;
}

bool string_to_long_int(const char *str,long int *out)
{
	const char *scanner = str;
	bool do_neg = false;

	*out = 0;

	if(*scanner == '-'){
		do_neg = true;
		scanner++;
	}

	while ((*scanner != 0) && (isdigit(*scanner))){

		(*out) = (*out)*10 + (*scanner)-48;
		scanner++;

	}

   if(do_neg) *out = -(*out);

	return (*scanner != 0);
}

void shell_value_free(shell_value *value)
{
	if(!value->isnumber) free(value->value.val_text);
	free(value);
}


char *shell_value_to_char(shell_value *value)
{
	if(value->isnumber){
		char tmp[255];
		sprintf(tmp,"%ld",value->value.val_number);
      return shell_strdup(tmp);
	} else {
		return shell_strdup(value->value.val_text);
	}
}

int shell_value_to_number(shell_value *value,long int *number)
{
	if(value->isnumber){
		*number = value->value.val_number;
		return SHE_NO_ERROR;
	} else {
		return string_to_long_int(value->value.val_text,number);
	}
}

int shell_value_get_text(shell_value * value,char **text){
	if(value->isnumber) return SHE_INVALID_TYPE;
	*text = value->value.val_text;
	return SHE_NO_ERROR;
}

int shell_value_get_number(shell_value *value,long int *number)
{
	if(!value->isnumber) return SHE_INVALID_TYPE;
	*number = value->value.val_number;
	return SHE_NO_ERROR;
}

/* XXX - unused and static 
static void shell_var_free(shell_var *var)
{
	shell_value_free(var->value);
	free(var->name);
	free(var);
}
*/

shell_value *shell_value_init_number(long int value)
{
	shell_value *current;

	current = malloc(sizeof(shell_value));
	if(current == NULL) return NULL;

	current->isnumber = true;
	current->value.val_number = value;

	return  current;
}


shell_value *shell_value_init_text(const char *value)
{
	shell_value *current;

	current = malloc(sizeof(shell_value));
	if(current == NULL) goto err;

	current->value.val_text = shell_strdup(value);
	if(current->value.val_text == NULL) goto err1;

	current->isnumber = false;

	return current;
err1:
	free(current);
err:
	return NULL;
}


int shell_value_set_text(shell_value *value,const char *new_value)
{
	char *val = shell_strdup(new_value);
	if(val == NULL) return SHE_NO_MEMORY;

	if(!value->isnumber) free(value->value.val_text);

	value->value.val_text  = val;
	value->isnumber = false;

	return SHE_NO_ERROR;
}

int shell_value_set_number(shell_value *value,long int new_value)
{
	if(!value->isnumber) free(value->value.val_text);

	value->value.val_number = new_value;
	value->isnumber = true;

	return SHE_NO_ERROR;
}



shell_value *shell_value_clone(shell_value *var)
{
	if(var->isnumber){
		return shell_value_init_number(var->value.val_number);
	} else {
		return shell_value_init_text(var->value.val_text);
	}
}






int shell_value_neg(shell_value *out)
{
	long int value;
	int err;

	err = shell_value_get_number(out,&value);
	if(err != SHE_NO_ERROR) return err;

	err = shell_value_set_number(out,-value);
	return err;
}


// out = out .op. out

int shell_value_do_operation(shell_value *out,const shell_value *other,int oper_type)
{
	long int i1;
	long int i2;
	long int i3;
	int err;

	if(out->isnumber != other->isnumber) return SHE_INVALID_TYPE;
	if(out->isnumber){

		i1 = out->value.val_number;
		i2 = other->value.val_number;
      printf("%ld %ld %d",i1,i2,oper_type);
		switch(oper_type){
			case SVO_ADD       : i3 = i1+i2    ;break;
			case SVO_SUB       : i3 = i1-i2    ;break;
			case SVO_MUL       : i3 = i1*i2    ;break;
			case SVO_DIV       : i3 = i1/i2    ;break;
			case SVO_BIGGER    : i3 = i1>i2    ;break;
			case SVO_LESS      : i3 = i1<i2    ;break;
			case SVO_EQUAL     : i3 = i1 == i2 ;break;
			case SVO_BIGGER_E  : i3 = i1 >=i2  ;break;
			case SVO_LESS_E    : i3 = i1 <= i2 ;break;
			case SVO_NOT_EQUAL : i3 = i1 != i2  ;break;
			default: return SHE_INVALID_OPERATION;
		}

	} else {

		i1 = strcmp(out->value.val_text,other->value.val_text);

		switch(oper_type){
			case SVO_EQUAL	   : i3 = i1 == 0;break;
			case SVO_BIGGER	: i3 = i1 > 0;break;
			case SVO_LESS		: i3 = i1 < 0;break;
			case SVO_BIGGER_E : i3 = i1 >= 0;break;
			case SVO_LESS_E	: i3 = i1 <= 0;break;
			case SVO_NOT_EQUAL: i3 = i1 != 0;break;
			default : return SHE_INVALID_OPERATION;
		}
	}

	err =  shell_value_set_number(out,i3);
	return err;
}


//
// init shell var
//

static shell_var *shell_var_init_value(const char *name,shell_value *value)
{
	shell_var *current;

	current = malloc(sizeof(shell_var));
	if(current == NULL) goto err;

	current->name = shell_strdup(name);
	if(current->name == NULL) goto err2;

	current->value = value;

	return current;

	free(current->name);
err2:
	free(current);
err:
	return NULL;
}




//
// Add shell variable
//

static int shell_var_add(const char *var_name,shell_value *value)
{
	shell_var *current;

	current = shell_var_init_value(var_name,value);

	current->next = var_list;
	var_list = current;

	return SHE_NO_ERROR;

	free(current->name);

	free(current);
	return SHE_NO_MEMORY;
}


static  shell_var * find_shell_var_by_name(const char *name)
{
	shell_var *current;

	current = var_list;

	while((current != NULL) && (strcmp(current->name,name) != 0)) current = current->next;

	return current;
}





static int shell_var_set(const char *var_name,shell_value *value)
{
	shell_var *current;

	current = find_shell_var_by_name(var_name);

	if(current == NULL){

        return shell_var_add(var_name,value);

	}

	shell_value_free(current->value);
	current->value = value;

	return SHE_NO_ERROR;
}

int shell_var_set_number(const char *var_name,long int number)
{
	shell_value *value;

	value = shell_value_init_number(number);
	if(value == NULL) return SHE_NO_MEMORY;

	return shell_var_set(var_name,value);
}


int shell_var_set_text(const char *var_name,const char *text)
{
	shell_value *value;

	value = shell_value_init_text(text);
	if(value == NULL) return SHE_NO_MEMORY;

	return shell_var_set(var_name,value);
}



int set_shell_var_with_value(const char *var_name,shell_value *value)
{
	return shell_var_set(var_name,shell_value_clone(value));
}

shell_value *get_value_by_name(const char *name)
{
	shell_var *current;
	shell_value *result = NULL;

	current  = find_shell_var_by_name(name);

	if(current != NULL) result = current->value;

	return result;
}


void init_vars(void){
	var_list = NULL;
}
