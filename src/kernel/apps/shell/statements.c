#include <string.h>
#include <ctype.h>
#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>

#include "parse.h"
#include "statements.h"
#include "shell_vars.h"
#include "shell_defs.h"
#include "file_utils.h"

static bool parse_value(scan_info *info,shell_value **out);
static int handle_exec(scan_info *info,shell_value **out);
static int parse_rvl_expr(scan_info *info,shell_value **out);

struct _oper_level{
	int sym_code;
	int level;

};

typedef struct _oper_level oper_level;

#define MAX_LEVEL 3

oper_level oper_levels[]=
{{SVO_EQUAL,3}
,{SVO_LESS,3}
,{SVO_LESS_E,3}
,{SVO_BIGGER,3}
,{SVO_BIGGER_E,3}
,{SVO_NOT_EQUAL,3}
,{SVO_ADD,2}
,{SVO_SUB,2}
,{SVO_MUL,1}
,{SVO_DIV,1}
,{0,-1}
};


static int get_oper_level(int oper)
{
	int cnt = 0;
	while((oper_levels[cnt].level != 0) && (oper_levels[cnt].sym_code != oper)) cnt++;
	return oper_levels[cnt].level;
}


static int parse_neg(scan_info *info,shell_value **out)
{
	int err;
	bool do_neg = false;

	if(info->sym_code == SVO_SUB){

		do_neg = true;
		if(scan(info)) return SHE_SCAN_ERROR;

	}

	err = parse_value(info,out);
	if(err != SHE_NO_ERROR) return err;

	if(do_neg){

		err = shell_value_neg(*out);
		if(err != SHE_NO_ERROR) {

			shell_value_free(*out);
			*out = NULL;

		}

	}

	return err;
}


static int parse_oper(scan_info *info,shell_value **out,int level)
{
	int err;
	int oper_code;
	shell_value *other;


	if(level > 1){

		err = parse_oper(info,out,level - 1);

	} else {

		err = parse_neg(info,out);

	}


	if(err != SHE_NO_ERROR) return err;


	oper_code = info->sym_code;

	while(get_oper_level(info->sym_code) == level){

		if(scan(info)){

			err = SHE_SCAN_ERROR;
			goto err;

		}

		if(level > 1){

			err = parse_oper(info,&other,level - 1);

		} else {

			err = parse_neg(info,&other);

		}

      if(err != SHE_NO_ERROR) goto err;

      err = shell_value_do_operation(*out,other,oper_code);
		if(err != SHE_NO_ERROR) goto err_2;

		shell_value_free(other);

	}

	return SHE_NO_ERROR;

err_2:
	shell_value_free(other);

err:
	shell_value_free(*out);
	*out = NULL;
	return err;

}


static int parse_expression(scan_info *info,shell_value **out)
{
	(*out) = NULL;
	return parse_oper(info,out,MAX_LEVEL);
}


static int parse_cast(scan_info * info,shell_value **out)
{
	int err;
	long int dummy;
	char *text;
	bool to_number = info->sym_code == SVO_NUMBER_CAST;

	if(scan(info)) return SHE_SCAN_ERROR;


	err = parse_rvl_expr(info,out);
	if(err != SHE_NO_ERROR) return err;

//place cast in shell_vars=>change type direct

	if(to_number){

		err = shell_value_to_number(*out,&dummy);
		if(err != SHE_NO_ERROR) goto err;

		err = shell_value_set_number(*out,dummy);
		if(err != SHE_NO_ERROR) goto err;

	} else {

		text = shell_value_to_char(*out);

		err = shell_value_set_text(*out,text);

		free(text);

	}
   return SHE_NO_ERROR;

err:

	shell_value_free(*out);
	*out = NULL;
	
	return err;
}

static int parse_value(scan_info *info,shell_value **out)
{
	char buf2[SCAN_SIZE+1];
	int err = SHE_NO_ERROR;
	long int val;
	shell_value *value;

	*out = NULL;

	switch(info->sym_code){

		case SVO_DQ_STRING:

			parse_vars_in_string(info->token,buf2,SCAN_SIZE);
			*out = shell_value_init_text(buf2);
			break;

		case  SVO_NUMBER:
			err = string_to_long_int(info->token,&val);
			if(err == SHE_NO_ERROR) *out = shell_value_init_number(val);
			break;

		case  SVO_SQ_STRING:

			*out = shell_value_init_text(info->token);
			break;

		case SVO_EXEC :
			err = handle_exec(info,out);
			break;

		case SVO_NUMBER_CAST:
		case SVO_STRING_CAST:
			err = parse_cast(info,out);
			break;

		case SVO_PARENL:
			err = parse_rvl_expr(info,out);
			if(err) return err;

			break;


		case SVO_DOLLAR:
			if(scan(info)) return SHE_SCAN_ERROR;

			if(info->sym_code == SVO_IDENT){

			value =get_value_by_name(info->token);

				if(value != NULL){

					 *out = shell_value_clone(value);

				} else {

					*out = shell_value_init_text("");

				}

			}  else {

				err = SVO_IDENT;

			}
			break;

		default:
			return SHE_PARSE_ERROR;
	}



	if(err == SHE_NO_ERROR){

		if(scan(info)){
			if(*out != NULL) shell_value_free(*out);
			 return SHE_SCAN_ERROR;
		}

		if(*out == NULL) err = SHE_NO_MEMORY;

	}

	return err;
}

static int handle_exit(scan_info *info)
{
	long int exit_value = 0;
	int err = SHE_NO_ERROR;
	shell_value *expr;

	if(info->sym_code == SVO_PARENL){

		if(scan(info)) return SHE_SCAN_ERROR;

		err = parse_expression(info,&expr);
		if(err != SHE_NO_ERROR) return err;

		if(!(expr->isnumber)){
			err = SHE_INVALID_TYPE;
			goto err;
		}

		err = shell_value_get_number(expr,&exit_value);
		if(err != SHE_NO_ERROR) goto err;

		if(expect(info,SVO_PARENR)){
			err = SVO_PARENR;
			goto err;
		}

	}

	sys_exit(exit_value);


err:
	shell_value_free(expr);
	return err;
}

static int parse_rvl_expr(scan_info *info,shell_value **out)
{
	int err = SHE_NO_ERROR;

	*out = NULL;

	if(expect(info,SVO_PARENL)) return SVO_PARENL;

	err = parse_expression(info,out);

	if(err != SHE_NO_ERROR) return err;

	if(expect(info,SVO_PARENR)) {

		err = SVO_PARENR;
		goto err;

	}

	return SHE_NO_ERROR;
err:
	shell_value_free(*out);

	return err;
}

static int handle_if(scan_info *info)
{
	int err = SHE_NO_ERROR;
	long int value;

	shell_value *expr;

	err = parse_rvl_expr(info,&expr);
	if(err != SHE_NO_ERROR) return err;

	if(!expr->isnumber){

		err = SHE_INVALID_TYPE;
		goto err;

	}

	err = shell_value_get_number(expr,&value);
	if(err != SHE_NO_ERROR) goto err;

 	if(value != 0) err = parse_info(info);

err:
	shell_value_free(expr);
	return err;
}


//
// Parse exec function
//

static int handle_exec(scan_info *info,shell_value **out)
{
	shell_value *state;
	int argc;
	char *argv[64];
	char redirect_in[SCAN_SIZE+1];
	char redirect_out[SCAN_SIZE+1];
	int err = SHE_NO_ERROR;
	int retcode;
	char *statement;

	*out = NULL;

	if(scan(info)) return SHE_SCAN_ERROR;

	err = parse_rvl_expr(info,&state);
	if(err != SHE_NO_ERROR) return err;

	err = shell_value_get_text(state,&statement);
	if(err != SHE_NO_ERROR) return err;

	argc = parse_line(statement,argv,64,redirect_in,redirect_out);

	err = exec_file(argc,argv,&retcode);

	if(err != SHE_NO_ERROR) goto err;

	*out = shell_value_init_number(retcode);

err:
	shell_value_free(state);
	return err;
}

//
// Parse load statement
//

static int handle_load(scan_info *info)
{
	char var_name[SCAN_SIZE+1];
	shell_value *value;
	int  err = SHE_NO_ERROR;

	if(info->sym_code != SVO_IDENT) return SVO_IDENT;

	strcpy(var_name,info->token);
	if(scan(info)) return SHE_SCAN_ERROR;

	if(expect(info,SVO_LOAD)) return SVO_LOAD;

	err = parse_expression(info,&value);
	if(err != SHE_NO_ERROR) return err;

	set_shell_var_with_value(var_name,value);

	shell_value_free(value);

	return SHE_NO_ERROR;
}


//
// Parse echo statement
//

static int handle_echo(scan_info *info)
{
	shell_value *value;
	int err = SHE_NO_ERROR;
	char *text;

	while(info->sym_code != SVO_END){

		err = parse_expression(info,&value);

		if(err != SHE_NO_ERROR) return err;

		text = shell_value_to_char(value);
		printf("%s",text);
		shell_value_free(value);
		free(text);

		if(info->sym_code == SVO_END) break;

		if(expect(info,SVO_COMMA)) {
			err = SHE_SCAN_ERROR;
			break;
		}

	}

	printf("\n");
	return err;
}


//
// Convert error code too string.
//

void ste_error_to_str(int error,char *descr)
{

	char *err_txt;
	if(error < 0){
		strcpy(descr,strerror(error));
		return;
	}
	if(error <= SVO_MAX){
        sprintf(descr,"'%s' expected",id_to_token(error));
        return;
	}
	switch(error){
		case SHE_NO_ERROR:
				err_txt = "No error";
				break;

		case SHE_NO_MEMORY:
				err_txt = "Not enough memory";
				break;

		case SHE_PARSE_ERROR:
				err_txt = "Parse error";
				break;

		case SHE_FILE_NOT_FOUND:
				err_txt = "File not found";
				break;

		case SHE_CANT_EXECUTE:
				err_txt = "Can't execute";
				break;

		case SHE_INVALID_TYPE:
				err_txt = "Invalid type";
				break;

		case SHE_INVALID_NUMBER:
				err_txt = "Invalid number";
				break;

		case SHE_INVALID_OPERATION:
				err_txt = "Invalid operation";
				break;

		case SHE_MISSING_QOUTE:
				err_txt = "Missing Qoute";
				break;

		case SHE_LABEL_NOT_FOUND:
				err_txt = "Label not found";
				break;

		default:
				err_txt = "Unkown error";
	}
	strcpy(descr,err_txt);
}

// get an directory from path shell variable
// no	   : number of path
// name    : return buffer
// max_len : max size of text in return buffer

bool get_path(int no,char *name,int max_len)
{
	int cnt=no;
	int len_cnt = max_len;
	shell_value *var_value=get_value_by_name(NAME_VAR_PATH);
	char *scan=name;
	char *value;
	int err;
	if(var_value == NULL) return false;

	err = shell_value_get_text(var_value,&value);
	if(err != SHE_NO_ERROR) return false;

	while((cnt > 0) && (*value != 0)){

		while (*value != ':'){

			if(*value == 0){
				*name = 0;
				 return false;
			}
			value++;

		}
		value++;
		cnt --;
	}

	if(*value == 0) return false;

	while((*value != 0) && (*value != ':') && (len_cnt > 0)){

		*scan = *value;
		scan ++;
		value++;
		len_cnt--;

	}

	*scan = 0;
	return true;

}


static int handle_ident(scan_info *info,bool *is_label)
{
	char ch = *(info->scanner);
	*is_label = false;

	if(ch == ':'){
		*is_label = true;
		if(scan(info)) return SHE_SCAN_ERROR;
		if(scan(info)) return SHE_SCAN_ERROR;
		if(expect(info,SVO_END)) return SVO_END;
	}

	return SHE_NO_ERROR;

}


static int is_label_with_name(scan_info *info,char *check_label ,bool *is_label)
{
	char *begin = info->input_line;
	char *scanner = begin;
	*is_label = false;

	if(*scanner == 0) return SHE_NO_ERROR;

	scanner +=  strlen(scanner) -1;

	while((scanner != begin)  && (isspace(*scanner))) scanner--;

	if(*scanner == ':'){

		if(info->sym_code != SVO_IDENT) return SVO_IDENT;

		if(strcmp(check_label , info->token) == 0) *is_label = true;

	}

	return SHE_NO_ERROR;

}


static int scan_until_label(scan_info *info,char *label_name)
{
	int err;
	bool is_label;

	if(scan_info_home(info)) return SHE_SCAN_ERROR;
	while(info->current != NULL){

			err = is_label_with_name(info,label_name,&is_label);

			if(is_label) return SHE_NO_ERROR;

			if(scan_info_next_line(info)) return SHE_SCAN_ERROR;

	}

	return SHE_LABEL_NOT_FOUND;
}

static int handle_goto(scan_info *info)
{
	int err;

	char label_name[SCAN_SIZE + 1];
	if(scan(info)) return SHE_SCAN_ERROR;

	if(info->sym_code != SVO_IDENT) return SVO_IDENT;

	strcpy(label_name,info->token);

	err = scan_until_label(info,label_name);

	return err;
}



int parse_string(char *in)
{
	scan_info info;
	init_scan_info(in,&info);
	return parse_info(&info);
}

int parse_info(scan_info *info)
{
	char err_txt[255];
	int err = SHE_NO_ERROR;
	bool is_label;

	switch(info->sym_code){

		case SVO_DOLLAR :

			if(scan(info)){

				err = SHE_SCAN_ERROR;
				break;

			}
			err = handle_load(info);
			break;

		case SVO_GOTO:

			err = handle_goto(info);
			break;

		case SVO_IDENT:

			err = handle_ident(info,&is_label);

			if(!is_label){
				err =  shell_parse(info->input_line,strlen(info->input_line));
			}
         break;
         
		case SVO_ECHO :
			if(scan(info)){

				err = SHE_SCAN_ERROR;
				break;
			}
			err = handle_echo(info);
			 	break;

		case SVO_IF :

			if(scan(info)){

				err = SHE_SCAN_ERROR;

			} else {

				err = handle_if(info);

			}
         
			break;

		case SVO_EXIT :
			if(scan(info)){

				err = SHE_SCAN_ERROR;

			} else {

				err = handle_exit(info);

			}
         
			break;

		default:
			return shell_parse(info->input_line,strlen(info->input_line));

	}

	if(err != SHE_NO_ERROR){

		if(err == SHE_SCAN_ERROR) err = info->scan_error;
		ste_error_to_str(err,err_txt);
		printf("error :%d:%ld,  %d-%s \n",info->line_no,
		                                 info->scanner - info->input_line - strlen(info->token),
		                                 err,
		                                 err_txt);

	}
	return err;
}



void init_statements(void){

	shell_var_set_text(NAME_VAR_PATH,".:/:/boot:/boot/bin");

}



