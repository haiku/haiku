#include <string.h>
#include <ctype.h>
#include <syscalls.h>
#include <stdio.h>
#include <stdlib.h>

#include "parse.h"
#include "commands.h"
#include "shell_defs.h"
#include "shell_vars.h"
#include "script.h"



struct token_info{
	char *token;
	int  code;
};

struct token_info tokens[]={
{"echo",SVO_ECHO},
{"exec",SVO_EXEC},
{"exit",SVO_EXIT},
{"if",SVO_IF},
{"goto",SVO_GOTO},
{"number",SVO_NUMBER_CAST},
{"string",SVO_STRING_CAST},
{"+",SVO_ADD},
{"-",SVO_SUB},
{"*",SVO_MUL},
{"/",SVO_DIV},
{"=",SVO_LOAD},
{"==",SVO_EQUAL},
{"!=",SVO_NOT_EQUAL},
{">",SVO_BIGGER},
{">=",SVO_BIGGER_E},
{"<",SVO_LESS},
{"<=",SVO_LESS_E},
{"$",SVO_DOLLAR},
{",",SVO_COMMA},
{"(",SVO_PARENL},
{")",SVO_PARENR},
{":",SVO_DOTDOT},
{NULL,SVO_NONE}
};


static int token_to_id(const char *token)
{
	int cnt = 0;

	while((tokens[cnt].token != NULL) && (strcmp(tokens[cnt].token,token) != 0))  cnt++;

	return tokens[cnt].code;
}

char *id_to_token(int id)
{
	int cnt = 0;

	switch(id){

		case SVO_IDENT			:return "ident";
		case SVO_SQ_STRING	:
		case SVO_DQ_STRING	:return "string";
		case SVO_NUMBER		:return "number";
		case SVO_END         :return "end";
	}

	while((tokens[cnt].token != NULL) && (tokens[cnt].code != id))  cnt++;

	if(tokens[cnt].token != NULL){

		return tokens[cnt].token;

	} else {

		return "unkown";

	}
}

static bool  scan_string(scan_info *info,int max_len)
{
	const char **in=&(info->scanner);
	char ch=**in;
	char *scan=info->token;
	int  len_cnt = max_len;
	bool err = false;

	(*in)++;

	while((**in != 0) && (**in != ch)){

		if(len_cnt > 0){

			*scan = **in;
			scan++;
			len_cnt--;

		}

		(*in)++;

	}

	if(**in != 0) {
		(*in)++;
	} else {
		info->scan_error = SHE_MISSING_QOUTE;
		err = true;
	}

	*scan = 0;
	return err;
}



int parse_line(const char *buf, char *argv[], int max_args, char *redirect_in, char *redirect_out)
{
	const char *scan=buf;
	const char *type_char;
	const char *start;
	char out[SCAN_SIZE+1];
	char tmp[SCAN_SIZE+1];
	char *arg_tmp;
	int arg_cnt = 0;
	int len = 0;
	bool replace;
	bool param;
	redirect_in[0] = 0;
	redirect_out[0] = 0;

	while(true){

		while((*scan != 0) && (isspace(*scan))) scan++;

		if(*scan == 0) break;

		replace = true;
		type_char = scan;
		start = scan;
		param = true;

		switch(*type_char){
		case '\'':
			replace  = false;

      case  '"':
			scan++;
			start++;
			while((*scan != 0) && (*scan != *start)) scan++;
			break;

		case '>':
		case '<':
         start++;
			while((*start != 0) && (isspace(*start))) start++;
			scan = start;

		default:

			while((*scan != 0) && (!isspace(*scan))) scan++;

		}

		len = scan - start;
		if(*scan != 0) scan++;
        
		if(replace){
        
				memcpy(tmp,start,len);
				tmp[len]=0;
				parse_vars_in_string(tmp,out,SCAN_SIZE);

		} else {

				memcpy(out,start,len);
				out[len]=0;
                
		}

		switch(*type_char){

		case '>':
			strcpy(redirect_out,out);
			break;

		case '<':
			strcpy(redirect_in,out);
			break;

		default:
			if(arg_cnt < max_args){

				arg_tmp = shell_strdup(out);
				if(arg_tmp != NULL) argv[arg_cnt++] = arg_tmp;

			}

		}
	}

	return arg_cnt;
}



static void scan_ident(const char **in,char *out,int max_len)
{
	char *scan = out;
	int len_cnt = max_len;

	while ((**in != 0) && (IS_IDENT_CHAR(**in))){
		if(len_cnt > 0){
			*scan = **in;
			scan++;
			len_cnt--;
		}
		(*in)++;
	}
	*scan = 0;
}

static void scan_number(const char **in,char *out,int max_len)
{
	char *scan = out;
	int len_cnt = max_len;

	while ( (**in != 0) && (isdigit(**in)) ){

		if(len_cnt > 0){

			*scan = **in;
			scan++;
			len_cnt--;

		}

		(*in)++;

	}
	*scan = 0;
}


static void scan_comp(const char **in,char *out,int max_len)
{
	int num_cnt = max_len;
	char *out_scan = out;

	*out_scan  = **in;
	out_scan++;
	(*in)++;
	num_cnt--;

	if(num_cnt > 0) {

		if(**in == '='){

			*out_scan = **in;
			out_scan++;
			(*in)++;
			num_cnt--;

		}
	}
	*out_scan = 0;
}

bool expect(scan_info *info,int check)
{
	if(info->sym_code == check){

		scan(info);
		return false;

	} else {

		return true;

	}
}


bool scan_info_next_line(scan_info *info)
{
	if(info->current == NULL) return false;

	info->current = info->current->next;

	if(info->current == NULL) return false;

	info->line_no++;

	return set_scan_info_line(info);
}

bool scan_info_home(scan_info *info)
{
	info->current = info->data.list;
	info->line_no = 1;
	return set_scan_info_line(info);
}


bool init_scan_info_by_file(const char *file_name,scan_info *info)
{
	int err = SHE_NO_ERROR;

	err = read_text_file(file_name,&(info->data));

	if(err != SHE_NO_ERROR) return err;

	info->scanner    = NULL;
	info->scan_error = SHE_NO_ERROR;

	if(scan_info_home(info)) err  = SHE_SCAN_ERROR;

	return err;

}

bool set_scan_info_line(scan_info *info)
{

	if(info->current == NULL) return true;

	strcpy(info->input_line,info->current->text);
	info->scanner = (info->input_line);
	return scan(info);

}

// fetch next token
//      **in  scan string
//      *out  token
//      return token type (STY_<> vars)


void init_scan_info(const char*in,scan_info *info){

	strncpy(info->input_line,in,SCAN_SIZE);

	info->input_line[SCAN_SIZE] = 0;
	info->scanner=(const char*)info->input_line;
	info->scan_error = SHE_NO_ERROR;
	info->current = NULL;
	info->line_no = 1;

	scan(info);

}

bool scan(scan_info *info)
{
	bool err = false;
	int sym_code = SVO_NONE;
	const char **in = &(info->scanner);
	char tmp[SCAN_SIZE+1];

	while((**in != 0) && (isspace(**in))) (*in)++;

	if(isalpha(**in)){

		scan_ident(in,info->token,SCAN_SIZE);

		sym_code = token_to_id(info->token);
		if(sym_code == SVO_NONE) sym_code = SVO_IDENT;

	} else if(isdigit(**in)){

		scan_number(in,info->token,SCAN_SIZE);
		sym_code = SVO_NUMBER;
	} else{
		switch(**in){
			case '#':
				sym_code = SVO_END;
				break;
			case '>':
			case '<':
			case '=':
			case '!':

				scan_comp(in,info->token,SCAN_SIZE);
				break;

			case '"':

				err = scan_string(info,SCAN_SIZE);

				if(err != SHE_NO_ERROR){
					parse_vars_in_string(info->token,tmp,SCAN_SIZE);
					strncpy(info->token,tmp,SCAN_SIZE);
					info->token[SCAN_SIZE] = 0;
				}

				sym_code = SVO_DQ_STRING;
				break;
			case '\'':

				sym_code = SVO_SQ_STRING;
				err =scan_string(info,SCAN_SIZE);
				break;
			case 0:

				*(info->token) = 0;
				sym_code = SVO_END;
				break;
			default:
				info->token[0] = **in;
				info->token[1] = 0;
				(*in)++;
				break;

		}
		if(sym_code == SVO_NONE) sym_code = token_to_id(info->token);

	}

	info->sym_code = sym_code;

	return err;
}

void parse_vars_in_string(const char *string,char *out,int max_len)
{
	const char *scan= string;
	char buf[SCAN_SIZE + 1];
	char *dest;
	char *out_scan = out;
	shell_value *value;
	int out_len = 0;
	int len_cnt;
	int out_max_len= max_len;
	int can_move;
	char *text;

	while((*scan != 0) && (*scan != '\'') && (out_max_len > 0)){
		if(*scan == '$'){
			scan++;
			dest = buf;
			len_cnt = SCAN_SIZE;

			while ((*scan != 0) && (IS_IDENT_CHAR(*scan)) &&(len_cnt > 0)){

					*dest = *scan;
					dest++;
					scan++;
					len_cnt--;

			}

			*dest = 0;
			value = get_value_by_name(buf);

			if(value != NULL){

				text = shell_value_to_char(value);
				can_move = strlen(text);

				if(can_move > out_max_len) can_move = out_max_len;

				memcpy(out_scan,text,can_move);
				out_max_len -= can_move;
				out_scan += can_move;

				free(text);

			}

		} else{

			*out_scan = *scan;
			out_len++;
			out_scan++;
			scan ++;
			out_max_len--;

		}
	}
	*out_scan= 0;
}




static int launch(int (*cmd)(int, char **), int argc, char **argv, char *r_in, char *r_out)
{
	int saved_in;
	int saved_out;
	int new_in;
	int new_out;
	int retval= 0;
   int err;

	if(strcmp(r_in, "")!= 0) {
		new_in = sys_open(r_in, STREAM_TYPE_ANY, 0);
		if(new_in < 0) {
			new_in = sys_create(r_in,STREAM_TYPE_FILE);
		}
	} else {
		new_in = sys_dup(0);
	}
	if(new_in < 0) {
		err = new_in;
		goto err_1;
	}

	if(strcmp(r_out, "")!= 0) {
		new_out = sys_open(r_out, STREAM_TYPE_ANY, 0);
		if(new_out < 0){
			new_out = sys_create(r_out,STREAM_TYPE_FILE);
		}
	} else {
		new_out = sys_dup(1);
	}
	if(new_out < 0) {
		err = new_out;
		goto err_2;
	}


	saved_in = sys_dup(0);
	saved_out= sys_dup(1);

	sys_dup2(new_in, 0);
	sys_dup2(new_out, 1);
	sys_close(new_in);
	sys_close(new_out);

	retval= cmd(argc, argv);

	sys_dup2(saved_in, 0);
	sys_dup2(saved_out, 1);
	sys_close(saved_in);
	sys_close(saved_out);

	return 0;

err_2:
	sys_close(new_in);
err_1:
	return err;
}


int shell_parse(const char *buf, int len)
{
	int i;
//	bool found_command = false;
	int  argc;
	char *argv[64];
	char redirect_in[256];
	char redirect_out[256];
	cmd_handler_proc *handler = NULL;
	int cnt;
	int err;

	// search for the command
	argc = parse_line(buf, argv, 64, redirect_in, redirect_out);

	if(argc == 0) return 0;

	handler = &cmd_create_proc;

	for(i=0; cmds[i].cmd_handler != NULL; i++) {
		if(strcmp(cmds[i].cmd_text,argv[0]) == 0){
			handler = cmds[i].cmd_handler;
			break;
		}
	}

	err = launch(handler, argc, argv, redirect_in, redirect_out);

	for(cnt = 0;cnt <argc;cnt++) free(argv[cnt]);

	return err;
}





