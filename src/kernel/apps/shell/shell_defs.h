#ifndef _shell_defs_h_
#define _shell_defs_h_

#include "script.h"

#define NAME_VAR_PATH  "path"
#define NAME_VAR_ARGC  "argc"
#define NAME_PAR_PRFX  "p%d"

#define SVO_ADD			0
#define SVO_SUB    		1
#define SVO_MUL			2
#define SVO_DIV			3
#define SVO_BIGGER		4
#define SVO_EQUAL			5
#define SVO_LESS			6
#define SVO_LESS_E  		7
#define SVO_BIGGER_E  	8
#define SVO_NOT_EQUAL 	9
#define SVO_PARENL		10
#define SVO_PARENR		11
#define SVO_COMMA			12
#define SVO_LOAD			13
#define SVO_EXEC			14
#define SVO_ECHO			15
#define SVO_DOLLAR		16
#define SVO_IDENT			17
#define SVO_SQ_STRING   18
#define SVO_DQ_STRING   19
#define SVO_NUMBER		20
#define SVO_CHAR			21
#define SVO_END			22
#define SVO_NONE			23
#define SVO_EXIT			24
#define SVO_IF				25
#define SVO_DOTDOT		26
#define SVO_GOTO			27
#define SVO_NUMBER_CAST	28
#define SVO_STRING_CAST	29
#define SVO_MAX			30

#define SHE_NO_ERROR    		0   		  // no error
#define SHE_NO_MEMORY			SVO_MAX+2  // not enough memory
#define SHE_PARSE_ERROR			SVO_MAX+3  // parse error
#define SHE_FILE_NOT_FOUND		SVO_MAX+4
#define SHE_CANT_EXECUTE		SVO_MAX+5
#define SHE_INVALID_TYPE		SVO_MAX+6
#define SHE_INVALID_NUMBER		SVO_MAX+7
#define SHE_INVALID_OPERATION	SVO_MAX+8
#define SHE_SCAN_ERROR			SVO_MAX+9
#define SHE_MISSING_QOUTE		SVO_MAX+10
#define SHE_LABEL_NOT_FOUND	SVO_MAX+11


#define SCAN_SIZE  255		// size of output strings in parser

struct _scan_info{
	char input_line[SCAN_SIZE+1];
	char token[SCAN_SIZE+1];
	const char *scanner;
	int sym_code;
	int scan_error;
	int line_no;
	text_item *current;
	text_file data;
};

typedef struct _scan_info scan_info;

#endif
