%{
/*
 *  FILE: parser.y
 *  AUTH: Michael John Radwin <mjr@acm.org>
 *
 *  DESC: stubgen grammar description.  Portions borrowed from
 *  Newcastle University's Arjuna project (http://arjuna.ncl.ac.uk/),
 *  and Jeff Lee's ANSI Grammar
 *  (ftp://ftp.uu.net/usenet/net.sources/ansi.c.grammar.Z)
 *  This grammar is only a subset of the real C++ language.
 *
 *  DATE: Thu Aug 15 13:10:06 EDT 1996
 *   $Id: parser.y 10 2002-07-09 12:24:59Z ejakowatz $
 *
 *  Copyright (c) 1996-1998  Michael John Radwin
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  --------------------------------------------------------------------
 * 
 *  $Log: parser.y,v $
 *  Revision 1.1  2002/07/09 12:24:59  ejakowatz
 *  It is accomplished ...
 *
 *  Revision 1.1  2001/11/07 10:06:07  ithamar
 *  Added stubgen to CVS
 *
 *  Revision 1.72  1998/07/07 00:14:06  mradwin
 *  removed extra space from throw_decl, cleaned up memory leak
 *  in ctor skeleton
 *
 *  Revision 1.71  1998/06/11 14:52:09  mradwin
 *  allow for empty class declarations, such as
 *  class Element {};
 *  also, differentiate structs from classes with
 *  new STRUCT_KIND tag.
 *  New version: 2.04
 *
 *  Revision 1.70  1998/05/11 19:49:11  mradwin
 *  Version 2.03 (updated copyright information).
 *
 *  Revision 1.69  1998/04/07 23:43:34  mradwin
 *  changed error-handling code significantly.
 *  several functions now return a value, and instead of
 *  calling fatal(), we do a YYABORT or YYERROR to get out
 *  of the parsing state.
 *  New version: 2.02.
 *
 *  Revision 1.68  1998/03/28 02:59:41  mradwin
 *  working on slightly better error recovery; not done yet.
 *
 *  Revision 1.67  1998/03/28 02:34:56  mradwin
 *  added multi-line function parameters
 *  also changed pointer and reference (* and &) types so there
 *  is no trailing space before the parameter name.
 *
 *  Revision 1.66  1998/01/12 19:39:11  mradwin
 *  modified rcsid
 *
 *  Revision 1.65  1997/11/13 22:50:55  mradwin
 *  moved copyright from parser.y to main.c
 *
 *  Revision 1.64  1997/11/13 22:40:15  mradwin
 *  fixed a silly comment bug
 *
 *  Revision 1.63  1997/11/13 22:37:31  mradwin
 *  changed char[] to char * to make non-gcc compilers
 *  a little happier.  We need to #define const to nothing
 *  for other compilers as well.
 *
 *  Revision 1.62  97/11/13  21:29:30  21:29:30  mradwin (Michael Radwin)
 *  moved code from parser.y to main.c
 * 
 *  Revision 1.61  1997/11/13 21:10:17  mradwin
 *  renamed stubgen.[ly] to parser.y lexer.l
 *
 *  Revision 1.60  1997/11/11 04:11:29  mradwin
 *  fixed command-line flags: invalid options now force usgage.
 *
 *  Revision 1.59  1997/11/11 04:03:56  mradwin
 *  changed version info
 *
 *  Revision 1.58  1997/11/11 03:54:05  mradwin
 *  fixed a long-standing bug with -b option.  a typo was causing
 *  the -b flag to be ignored.
 *
 *  Revision 1.57  1997/11/11 03:52:06  mradwin
 *  changed fatal()
 *
 *  Revision 1.56  1997/11/05 03:02:02  mradwin
 *  Modified logging routines.
 *
 *  Revision 1.55  1997/11/05 02:14:38  mradwin
 *  Made some compiler warnings disappear.
 *
 *  Revision 1.54  1997/11/01 23:26:13  mradwin
 *  new Revision string and usage info
 *
 *  Revision 1.53  1997/11/01 23:12:43  mradwin
 *  greatly improved error-recovery.  errors no longer spill over
 *  into other files because the yyerror state is properly reset.
 *
 *  Revision 1.52  1997/10/27 01:14:23  mradwin
 *  fixed constant_value so it supports simple arithmetic.  it's
 *  not as robust as full expressions, but this will handle the
 *  char buffer[BUFSIZE + 1] problem.
 *
 *  Also removed expansion rules that simply did { $$ = $1; } because
 *  that action is implicit anyway.
 *
 *  Revision 1.51  1997/10/26 23:16:32  mradwin
 *  changed inform_user and fatal functions to use varargs
 *
 *  Revision 1.50  1997/10/26 22:27:07  mradwin
 *  Fixed this bug:
 *  stubgen dies on the following because the protected section is empty:
 *
 *  class WidgetCsg : public WidgetLens {
 *   protected:
 *
 *   public:
 *       virtual ~WidgetCsg() {}
 *                WidgetCsg();
 *  };
 *
 *  Error:
 *  stubgen version 2.0-beta $Revision: 1.1 $.
 *  parse error at line 4, file test.H:
 *   public:
 *        ^
 *
 *  Revision 1.49  1997/10/16 19:42:48  mradwin
 *  added support for elipses, static member/array initializers,
 *  and bitfields.
 *
 *  Revision 1.48  1997/10/16 17:35:39  mradwin
 *  cleaned up usage info
 *
 *  Revision 1.47  1997/10/16 17:12:59  mradwin
 *  handle extern "C" blocks better now, and support multi-line
 *  macros.  still need error-checking.
 *
 *  Revision 1.46  1997/10/15 22:09:06  mradwin
 *  changed tons of names.  stubelem -> sytaxelem,
 *  stubin -> infile, stubout -> outfile, stublog -> logfile.
 *
 *  Revision 1.45  1997/10/15 21:33:36  mradwin
 *  fixed up function_hdr
 *
 *  Revision 1.44  1997/10/15 21:33:02  mradwin
 *  *** empty log message ***
 *
 *  Revision 1.43  1997/10/15 17:42:37  mradwin
 *  added support for 'extern "C" { ... }' blocks.
 *
 *  Revision 1.42  1997/09/26 20:59:18  mradwin
 *  now allow "struct foobar *f" to appear in a parameter
 *  list or as a variable decl.  Had to remove the
 *  class_or_struct rule and blow up the class_specifier
 *  description.
 *
 *  Revision 1.41  1997/09/26 19:02:18  mradwin
 *  fixed memory leak involving template decls in skeleton code.
 *  Leads me to believe that skel_elemcmp() is flawed, because
 *  it may rely in parent->templ info.
 *
 *  Revision 1.40  1997/09/26 18:44:22  mradwin
 *  changed parameter handing from char *'s to an argument type
 *  to facilitate comparisons between skeleton code
 *  and header code.  Now we can correctly recognize different
 *  parameter names while still maintaining the same signature.
 *
 *  Revision 1.39  1997/09/26 00:47:29  mradwin
 *  added better base type support -- recognize things like
 *  "long long" and "short int" now.
 *
 *  Revision 1.38  1997/09/19 18:16:37  mradwin
 *  allowed an instance name to come after a class, struct,
 *  union, or enum.  This improves parseability of typedefs
 *  commonly found in c header files, although true typedefs are
 *  not understood.
 *
 *  Revision 1.37  1997/09/15 22:38:28  mradwin
 *  did more revision on the SGDEBUG stuff
 *
 *  Revision 1.36  1997/09/15 19:05:26  mradwin
 *  allow logging to be compiled out by turning off SGDEBUG
 *
 *  Revision 1.35  1997/09/12 00:58:43  mradwin
 *  duh, silly me.  messed up compilation.
 *
 *  Revision 1.34  1997/09/12 00:57:49  mradwin
 *  Revision string inserted in usage
 *
 *  Revision 1.33  1997/09/12 00:51:19  mradwin
 *  string copyright added to code for binary copyright.
 *
 *  Revision 1.32  1997/09/12 00:47:21  mradwin
 *  some more compactness of grammar with parameter_list_opt
 *  and also ampersand_opt
 *
 *  Revision 1.31  1997/09/12 00:26:19  mradwin
 *  better template support, but still poor
 *
 *  Revision 1.30  1997/09/08 23:24:51  mradwin
 *  changes to error-handling code.
 *  also got rid of the %type <flag> for the top-level rules
 *
 *  Revision 1.30  1997/09/08 23:20:02  mradwin
 *  some error reporting changes and default values for top-level
 *  grammar stuff.
 *
 *  Revision 1.29  1997/09/08 17:54:24  mradwin
 *  cleaned up options and usage info.
 *
 *  Revision 1.28  1997/09/05 19:38:04  mradwin
 *  changed options for .ext instead of -l or -x
 *
 *  Revision 1.27  1997/09/05 19:17:06  mradwin
 *  works for scanning old versions, except for parameter
 *  names that differ between .H and .C files.
 *
 *  Revision 1.26  1997/09/05 16:34:36  mradwin
 *  GPL-ized code.
 *
 *  Revision 1.25  1997/09/05 16:11:44  mradwin
 *  some simple cleanup before GPL-izing the code
 *
 *  Revision 1.24  1997/09/04 19:50:34  mradwin
 *  whoo-hoo!  by blowing up the description
 *  exponentially, it works!
 *
 *  Revision 1.23  1997/03/20 16:05:41  mjr
 *  renamed syntaxelem to syntaxelem_t, cleaned up throw_decl
 *
 *  Revision 1.22  1996/10/02 15:16:57  mjr
 *  using pathname.h instead of libgen.h
 *
 *  Revision 1.21  1996/09/12 14:44:49  mjr
 *  Added throw decl recognition (great, another 4 bytes in syntaxelem)
 *  and cleaned up the grammar so that const_opt appears in far fewer
 *  places.  const_opt is by default 0 as well, so we don't need to
 *  pass it as an arg to new_elem().
 *
 *  I also added a fix to a potential bug with the MINIT and INLIN
 *  exclusive start states.  I think they could have been confused
 *  by braces within comments, so now I'm grabbing comments in those
 *  states as well.
 *
 *  Revision 1.20  1996/09/12 04:55:22  mjr
 *  changed expand strategy.  Don't expand while parsing now;
 *  enqueue as we go along and expand at the end.  Eventually
 *  we'll need to provide similar behavior for when we parse
 *  .C files
 *
 *  Revision 1.19  1996/09/12 03:46:10  mjr
 *  No concrete changes in code.  Just added some sanity by
 *  factoring out code into util.[ch] and putting some prototypes
 *  that were in table.h into stubgen.y where they belong.
 *
 *  Revision 1.18  1996/09/06 14:32:48  mjr
 *  defined the some_KIND constants for clarity, and made
 *  expandClass return immediately if it was give something other
 *  than a CLASS_KIND element.
 *
 *  Revision 1.17  1996/09/06 14:05:44  mjr
 *  Almost there with expanded operator goodies.  Still need
 *  to get OPERATOR type_name to work.
 *
 *  Revision 1.16  1996/09/04 22:28:09  mjr
 *  nested classes work and default arguments are now removed
 *  from the parameter lists.
 *
 *  Revision 1.15  1996/09/04 20:01:57  mjr
 *  non-functional expanded code.  needs work.
 *
 *  Revision 1.14  1996/09/01 21:29:34  mjr
 *  put the expanded_operator code back in as a useless rule.
 *  oughta think about fixing it up if possible
 *
 *  Revision 1.13  1996/09/01 20:59:48  mjr
 *  Added collectMemberInitList() function, which is similar
 *  to collectInlineDef() and also the exclusive state MINIT
 *
 *  Revision 1.12  1996/08/23 05:09:19  mjr
 *  fixed up some more portability things
 *
 *  Revision 1.11  1996/08/22 02:43:47  mjr
 *  added parse error message (using O'Reilly p. 274)
 *
 *  Revision 1.10  1996/08/21 18:33:50  mjr
 *  added support for template instantiation in the type_name
 *  rule.  surprisingly it didn't cause any shift/reduce conflicts.
 *
 *  Revision 1.9  1996/08/21 17:40:56  mjr
 *  added some cpp directives for porting to WIN32
 *
 *  Revision 1.8  1996/08/21 00:00:19  mjr
 *  approaching stability and usability.  command line arguments
 *  are handled now and the fopens and fcloses appear to work.
 *
 *  Revision 1.7  1996/08/20 20:44:23  mjr
 *  added initial support for optind but it is incomplete.
 *
 *  Revision 1.6  1996/08/19 17:14:59  mjr
 *  misordered args, fixed bug
 *
 *  Revision 1.5  1996/08/19 17:11:41  mjr
 *  RCS got confused with the RCS-style header goodies.
 *  got it cleaned up now.
 *
 *  Revision 1.4  1996/08/19 17:01:33  mjr
 *  Removed the expanded code checking and added
 *  lots of code that duplicates what stubgen.pl did.
 *  still need options pretty badly
 *
 *  Revision 1.3  1996/08/17 23:21:10  mjr
 *  added the expanded operator code, cleaned up tabs.
 *  consider putting all of the expanded code in another
 *  grammar - this one is getting cluttered.
 *
 */
%}

%{
#include "table.h"
#include "util.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef WIN32
#include <malloc.h> /* defintion of alloca */
#include "getopt.h" /* use GNU getopt      */
#endif /* WIN32 */

#ifndef WIN32
#include <pwd.h>
#endif /* WIN32 */

/* defined in lexer.l */
extern int collectInlineDef();
extern int collectMemberInitList();

/* defined here in parser.y */
static int error_recovery();
static int yyerror(char *);
static const char rcsid[] = "$Id: parser.y 10 2002-07-09 12:24:59Z ejakowatz $";

/* defined in main.c */
extern FILE *outfile;
extern char *currentFile;
extern int lineno;

%}

%union {
  char *string;
  syntaxelem_t *elt;
  arg_t *arg;
  int flag;
}

%token <string> IDENTIFIER CONSTANT STRING_LITERAL
%token <string> CHAR SHORT INT LONG SIGNED UNSIGNED FLOAT DOUBLE VOID
%token NEW DELETE TEMPLATE THROW

%token PTR_OP INC_OP DEC_OP LEFT_OP RIGHT_OP LE_OP GE_OP EQ_OP NE_OP
%token AND_OP OR_OP MUL_ASSIGN DIV_ASSIGN MOD_ASSIGN ADD_ASSIGN
%token SUB_ASSIGN LEFT_ASSIGN RIGHT_ASSIGN AND_ASSIGN
%token XOR_ASSIGN OR_ASSIGN CLCL MEM_PTR_OP

%token FRIEND OPERATOR CONST CLASS STRUCT UNION ENUM 
%token PROTECTED PRIVATE PUBLIC EXTERN ELIPSIS

%type <string> simple_type_name simple_signed_type non_reference_type
%type <string> scoped_identifier pointer asterisk
%type <string> type_name binary_operator
%type <string> assignment_operator unary_operator any_operator
%type <string> variable_specifier_list union_specifier
%type <string> constant_value multiple_variable_specifier
%type <string> enum_specifier enumerator_list enumerator 
%type <string> template_specifier template_arg template_arg_list
%type <string> template_instance_arg template_instance_arg_list
%type <string> throw_decl throw_list variable_name bitfield_savvy_identifier
%type <string> primary_expression expression
%type <string> multiplicative_expression additive_expression
%type <flag> const_opt ampersand_opt
%type <elt> class_specifier
%type <elt> member_func_specifier function_specifier constructor
%type <elt> destructor member_specifier member member_with_access
%type <elt> mem_type_specifier member_or_error
%type <elt> member_list member_list_opt
%type <elt> member_func_inlined type_specifier overloaded_op_specifier
%type <elt> member_func_skel member_func_skel_spec
%type <elt> constructor_skeleton destructor_skeleton
%type <elt> overloaded_op_skeleton function_skeleton
%type <arg> variable_or_parameter variable_specifier
%type <arg> parameter_list parameter_list_opt unnamed_parameter


%start translation_unit
%%

translation_unit
	: declaration
	| translation_unit declaration
	;

declaration
	: declaration_specifiers ';'
	| EXTERN STRING_LITERAL compound_statement
{ 
  log_printf("IGNORING extern \"C\" { ... } block.\n");
  free($2);
}
	| member_func_inlined
{ 
    $1->kind = INLINED_KIND; 
    log_printf("\nBEGIN matched dec : m_f_i rule --");
    print_se($1); 
    log_printf("END   matched dec : m_f_i rule\n");
}
	| function_skeleton
{ 
    $1->kind = SKEL_KIND; 
    log_printf("\nBEGIN matched dec : function_skeleton rule --");
    print_se($1); 
    enqueue_skeleton($1);
    log_printf("END   matched dec : function_skeleton rule\n");
}
	| ';'
	| error
{
  log_printf("declaration : error.  Attempting to recover...\n");
  yyerrok;
  yyclearin;
  if (error_recovery() != 0) {
      log_printf("ERROR recovery could not complete -- YYABORT.\n");
      YYABORT;
  }
  log_printf("ERROR recovery complete.\n");
}
	;

declaration_specifiers
	: member_specifier
{
    /* the name of the rule "member_specifier" might be misleading, but
     * this is either a class, struct, union, enum, global var, global
     * prototype, etc..  */
    if ($1->kind == CLASS_KIND || $1->kind == STRUCT_KIND) {
	enqueue_class($1);
    } else {
	log_printf("\nIGNORING      dec_spec : mem_spec (%s) --",
		  string_kind($1->kind));
	print_se($1);
	log_printf("END IGNORING  dec_spec : mem_spec (%s)\n",
		  string_kind($1->kind));
    }
}
	| forward_decl
	;

type_specifier
	: union_specifier
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), $1, NULL, IGNORE_KIND);
/*   print_se(elem); */
  $$ = elem;
}
	| enum_specifier
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), $1, NULL, IGNORE_KIND);
/*   print_se(elem); */
  $$ = elem;
}
	| class_specifier
	;

ampersand_opt
	: /* nothing */				{ $$ = 0; }
	| '&'					{ $$ = 1; }
	;

type_name
	: non_reference_type ampersand_opt
{ 
  char *tmp_str = (char *) malloc(strlen($1) + ($2 ? 2 : 0) + 1);
  strcpy(tmp_str, $1);
  if ($2)
    strcat(tmp_str, " &");
  free($1);
  $$ = tmp_str;
}
	| CONST non_reference_type ampersand_opt
{ 
  char *tmp_str = (char *) malloc(strlen($2) + ($3 ? 2 : 0) + 7);
  sprintf(tmp_str, "const %s%s", $2, ($3 ? " &" : ""));
  free($2);
  $$ = tmp_str;
}
	;

forward_decl
	: CLASS IDENTIFIER			{ free($2); }
	| CLASS scoped_identifier		{ free($2); }
	| STRUCT IDENTIFIER			{ free($2); }
	| STRUCT scoped_identifier		{ free($2); }
	;

non_reference_type
	: scoped_identifier
	| IDENTIFIER
	| STRUCT IDENTIFIER	
{
  char *tmp_str = (char *) malloc(strlen($2) + 8);
  strcpy(tmp_str, "struct ");
  strcat(tmp_str, $2);
  free($2);
  $$ = tmp_str; 
}
	| scoped_identifier '<' template_instance_arg_list '>'
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 3);
  sprintf(tmp_str, "%s<%s>", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	| IDENTIFIER '<' template_instance_arg_list '>'
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 3);
  sprintf(tmp_str, "%s<%s>", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	| scoped_identifier '<' template_instance_arg_list '>' pointer
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + strlen($5) + 4);
  sprintf(tmp_str, "%s<%s> %s", $1, $3, $5);
  free($1);
  free($3);
  free($5);
  $$ = tmp_str;
}
	| IDENTIFIER '<' template_instance_arg_list '>' pointer
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + strlen($5) + 4);
  sprintf(tmp_str, "%s<%s> %s", $1, $3, $5);
  free($1);
  free($3);
  free($5);
  $$ = tmp_str;
}
	| scoped_identifier pointer	
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($2) + 2);
  sprintf(tmp_str, "%s %s", $1, $2);
  free($1);
  free($2);
  $$ = tmp_str;
}
	| IDENTIFIER pointer	
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($2) + 2);
  sprintf(tmp_str, "%s %s", $1, $2);
  free($1);
  free($2);
  $$ = tmp_str;
}
	| STRUCT IDENTIFIER pointer	
{ 
  char *tmp_str = (char *) malloc(strlen($2) + strlen($3) + 9);
  sprintf(tmp_str, "struct %s %s", $2, $3);
  free($2);
  free($3);
  $$ = tmp_str;
}
	| simple_signed_type
	| simple_signed_type pointer
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($2) + 2);
  sprintf(tmp_str, "%s %s", $1, $2);
  free($1);
  free($2);
  $$ = tmp_str;
}
	;

simple_signed_type
	: simple_type_name
	| SIGNED simple_type_name	
{ 
  char *tmp_str = (char *) malloc(strlen($2) + 8);
  strcpy(tmp_str,"signed ");
  strcat(tmp_str, $2);
  free($1);
  free($2);
  $$ = tmp_str;
}
	| UNSIGNED simple_type_name
{ 
  char *tmp_str = (char *) malloc(strlen($2) + 10);
  strcpy(tmp_str,"unsigned ");
  strcat(tmp_str, $2);
  free($1);
  free($2);
  $$ = tmp_str;
}
	;

simple_type_name
	: CHAR
	| SHORT
	| SHORT INT
{ 
  $$ = strdup("short int");
  free($1);
  free($2);
}
	| INT
	| LONG
	| LONG INT
{ 
  $$ = strdup("long int");
  free($1);
  free($2);
}
	| LONG LONG
{ 
  $$ = strdup("long long");
  free($1);
  free($2);
}
	| LONG LONG INT
{ 
  $$ = strdup("long long int");
  free($1);
  free($2);
  free($3);
}
	| UNSIGNED
	| FLOAT
	| DOUBLE
	| VOID
	;

scoped_identifier
	: IDENTIFIER CLCL IDENTIFIER
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 3);
  sprintf(tmp_str, "%s::%s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	| scoped_identifier CLCL IDENTIFIER
{ 
  /* control-Y programming! */
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 3);
  sprintf(tmp_str, "%s::%s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	;

pointer
	: asterisk
	| pointer asterisk
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($2) + 1);
  strcpy(tmp_str,$1);
  strcat(tmp_str,$2);
  free($1);
  free($2);
  $$ = tmp_str;
}
	;

asterisk
	: '*'					{ $$ = strdup("*"); }
	| '*' CONST				{ $$ = strdup("*const "); }
	;

variable_or_parameter
	: type_name bitfield_savvy_identifier
{ 
  arg_t *new_arg = (arg_t *) malloc(sizeof(arg_t));
  new_arg->type = $1;
  new_arg->name = $2;
  new_arg->array = NULL;
  new_arg->next = NULL;
  $$ = new_arg;
}
	| type_name scoped_identifier
{ 
  arg_t *new_arg = (arg_t *) malloc(sizeof(arg_t));
  new_arg->type = $1;
  new_arg->name = $2;
  new_arg->array = NULL;
  new_arg->next = NULL;
  $$ = new_arg;
}
	| variable_or_parameter '[' constant_value ']'
{
  char *old_array = $1->array;
  int old_len = old_array ? strlen(old_array) : 0;
  $1->array = (char *) malloc(strlen($3) + old_len + 3);
  sprintf($1->array, "%s[%s]", old_array ? old_array : "", $3);
  free($3);
  if (old_array)
    free(old_array);
  $$ = $1;
}
	| variable_or_parameter '[' ']'
{ 
  char *old_array = $1->array;
  int old_len = old_array ? strlen(old_array) : 0;
  $1->array = (char *) malloc(old_len + 3);
  sprintf($1->array, "%s[]", old_array ? old_array : "");
  if (old_array)
    free(old_array);
  $$ = $1;
}
	;

bitfield_savvy_identifier
	: IDENTIFIER
	| IDENTIFIER ':' CONSTANT		{ free($3); $$ = $1;}
	;

variable_name
	: bitfield_savvy_identifier
	| pointer IDENTIFIER
{
  char *tmp_str = (char *) malloc(strlen($1) + strlen($2) + 2);
  sprintf(tmp_str, "%s %s", $1, $2);
  free($1);
  free($2);
  $$ = tmp_str;
}
	| variable_name '[' constant_value ']'
{
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 3);
  sprintf(tmp_str, "%s[%s]", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	| variable_name '[' ']'
{ 
  char *tmp_str = (char *) malloc(strlen($1) + 3);
  strcpy(tmp_str, $1);
  strcat(tmp_str, "[]");
  free($1);
  $$ = tmp_str;
}
	;

variable_specifier
	: variable_or_parameter
	| EXTERN variable_or_parameter		{ $$ = $2; }
	| variable_or_parameter '=' constant_value
{
  free($3);
  $$ = $1;
}
	;

multiple_variable_specifier
	: variable_specifier
{
  $$ = args_to_string($1, 0);
  free_args($1);
}
	| multiple_variable_specifier ',' variable_name
{
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 3);
  sprintf(tmp_str, "%s, %s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	;

variable_specifier_list
	: multiple_variable_specifier ';'
{ 
  char *tmp_str = (char *) malloc(strlen($1) + 2);
  sprintf(tmp_str, "%s;", $1);
  free($1);
  $$ = tmp_str;
}
	| variable_specifier_list multiple_variable_specifier ';'
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($2) + 3);
  sprintf(tmp_str, "%s\n%s;", $1, $2);
  free($1);
  free($2);
  $$ = tmp_str;
}
	;

parameter_list_opt
	: /* nothing */
{ 
  $$ = NULL; 
}
	| parameter_list
{
  $$ = reverse_arg_list($1);
}
	| parameter_list ',' ELIPSIS
{
  arg_t *new_arg = (arg_t *) malloc(sizeof(arg_t));
  new_arg->type = strdup("...");
  new_arg->name = NULL;
  new_arg->array = NULL;
  new_arg->next = $1;
  $$ = reverse_arg_list(new_arg);
}
	| ELIPSIS
{
  arg_t *new_arg = (arg_t *) malloc(sizeof(arg_t));
  new_arg->type = strdup("...");
  new_arg->name = NULL;
  new_arg->array = NULL;
  new_arg->next = NULL;
  $$ = new_arg;
}
	;

parameter_list
	: variable_specifier			
	| unnamed_parameter			
	| parameter_list ',' variable_specifier
{
  $3->next = $1;
  $$ = $3;
}
	| parameter_list ',' unnamed_parameter
{ 
  $3->next = $1;
  $$ = $3;
}
	;

unnamed_parameter
	: type_name				
{
  arg_t *new_arg = (arg_t *) malloc(sizeof(arg_t));
  new_arg->type = $1;
  new_arg->name = NULL;
  new_arg->array = NULL;
  new_arg->next = NULL;
  $$ = new_arg;
}
	| type_name '=' constant_value
{
  arg_t *new_arg = (arg_t *) malloc(sizeof(arg_t));
  new_arg->type = $1;
  new_arg->name = NULL;
  new_arg->array = NULL;
  new_arg->next = NULL;
  free($3);
  $$ = new_arg;
}
	| unnamed_parameter '[' constant_value ']'
{
  char *old_array = $1->array;
  int old_len = old_array ? strlen(old_array) : 0;
  $1->array = (char *) malloc(strlen($3) + old_len + 3);
  sprintf($1->array, "%s[%s]", old_array ? old_array : "", $3);
  free($3);
  if (old_array)
    free(old_array);
  $$ = $1;
}
	| unnamed_parameter '[' ']'
{ 
  char *old_array = $1->array;
  int old_len = old_array ? strlen(old_array) : 0;
  $1->array = (char *) malloc(old_len + 3);
  sprintf($1->array, "%s[]", old_array ? old_array : "");
  if (old_array)
    free(old_array);
  $$ = $1;
}
	;

function_skeleton
	: member_func_skel compound_statement
	| constructor_skeleton ':'
	  { if (collectMemberInitList() != 0) YYERROR; } compound_statement
	| template_specifier member_func_skel compound_statement 
{
  /* I think this is the correct behavior, but skel_elemcmp is wrong */
  /* $2->templ = $1; */
  free($1);
  $$ = $2;
}
	| template_specifier constructor_skeleton ':'
	  { if (collectMemberInitList() != 0) YYERROR; } compound_statement
{ 
  /* I think this is the correct behavior, but skel_elemcmp is wrong */
  /* $2->templ = $1; */
  free($1);
  $$ = $2;
}
	;

member_func_inlined
	: member_func_specifier compound_statement
	| constructor ':'
	  { if (collectMemberInitList() != 0) YYERROR; } compound_statement
	;

member_func_skel
	: member_func_skel_spec const_opt throw_decl 
{ 
    $1->const_flag = $2; 
    $1->throw_decl = $3;
    $$ = $1; 
}
	| constructor_skeleton
	| destructor_skeleton
	;

member_func_specifier
	: function_specifier const_opt throw_decl 
{ 
    $1->const_flag = $2; 
    $1->throw_decl = $3;
    $$ = $1; 
}
	| constructor
	| destructor
	;


member_func_skel_spec
	: type_name scoped_identifier '(' parameter_list_opt ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem($1, $2, $4, FUNC_KIND);
/*  print_se(elem); */
  $$ = elem;
}
	| type_name IDENTIFIER '<' template_instance_arg_list '>' CLCL IDENTIFIER '(' parameter_list_opt ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem($1,
			      (char *) malloc(strlen($2) + strlen($4) + strlen($7) + 5),
			      $9, FUNC_KIND);
  sprintf(elem->name,"%s<%s>::%s", $2, $4, $7);
  free($2);
  free($4);
  free($7);
  $$ = elem;
}
	| overloaded_op_skeleton
	;



function_specifier
	: type_name IDENTIFIER '(' parameter_list_opt ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem($1, $2, $4, FUNC_KIND);
  print_se(elem);
  $$ = elem;
}
	| overloaded_op_specifier
	;

overloaded_op_skeleton
	: type_name scoped_identifier CLCL OPERATOR any_operator '(' parameter_list_opt ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem($1, (char *)malloc(strlen($2) + strlen($5) + 12),
			      $7, FUNC_KIND);
  sprintf(elem->name, "%s::operator%s", $2, $5);
  free($2);
  free($5);
/*  print_se(elem); */
  $$ = elem;
}
	| scoped_identifier CLCL OPERATOR type_name '(' ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), 
			      (char *)malloc(strlen($1) + strlen($4) + 13),
			      NULL, FUNC_KIND);
  sprintf(elem->name, "%s::operator %s", $1, $4);
  free($1);
  free($4);
/*  print_se(elem); */
  $$ = elem;
}
	| type_name IDENTIFIER CLCL OPERATOR any_operator '(' parameter_list_opt ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem($1, (char *)malloc(strlen($2) + strlen($5) + 12),
			      $7, FUNC_KIND);
  sprintf(elem->name, "%s::operator%s", $2, $5);
  free($2);
  free($5);
/*  print_se(elem); */
  $$ = elem;
}
	| IDENTIFIER CLCL OPERATOR type_name '(' ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""),
			      (char *)malloc(strlen($1) + strlen($4) + 13),
			      NULL, FUNC_KIND);
  sprintf(elem->name, "%s::operator %s", $1, $4);
  free($1);
  free($4);
/*  print_se(elem); */
  $$ = elem;
}
	;

overloaded_op_specifier
	: type_name OPERATOR any_operator '(' parameter_list_opt ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem($1, (char *)malloc(strlen($3) + 9), 
			      $5, FUNC_KIND);
  sprintf(elem->name, "operator%s", $3);
  free($3);
  print_se(elem);
  $$ = elem;
}
	| OPERATOR type_name '(' ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), (char *)malloc(strlen($2) + 10),
			      NULL, FUNC_KIND);
  sprintf(elem->name, "operator %s", $2);
  free($2);
  print_se(elem);
  $$ = elem;
}
	;

const_opt
	: /* nothing */				{ $$ = 0; }
	| CONST					{ $$ = 1; }
	;

throw_decl
	: /* nothing */				{ $$ = NULL; }
	| THROW '(' throw_list ')'
{ 
  char *tmp_str = (char *) malloc(strlen($3) + 8);
  sprintf(tmp_str, "throw(%s)", $3);
  free($3);
  $$ = tmp_str;
}
	;

throw_list
	: type_name
	| throw_list ',' type_name
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 3);
  sprintf(tmp_str, "%s, %s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	;

destructor
	: '~' IDENTIFIER '(' ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), (char *) malloc(strlen($2) + 2),
			      NULL, FUNC_KIND);
  sprintf(elem->name,"~%s", $2);
  free($2);
/*   print_se(elem); */
  $$ = elem;
}
	;

destructor_skeleton
	: scoped_identifier CLCL '~' IDENTIFIER '(' ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), 
			      (char *) malloc(strlen($1) + strlen($4) + 4),
			      NULL, FUNC_KIND);
  sprintf(elem->name,"%s::~%s", $1, $4);
  free($1);
  free($4);
/*   print_se(elem); */
  $$ = elem;
}
	| IDENTIFIER CLCL '~' IDENTIFIER '(' ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), 
			      (char *) malloc(strlen($1) + strlen($4) + 4),
			      NULL, FUNC_KIND);
  sprintf(elem->name,"%s::~%s", $1, $4);
  free($1);
  free($4);
/*   print_se(elem); */
  $$ = elem;
}
	| IDENTIFIER '<' template_instance_arg_list '>' CLCL '~' IDENTIFIER '(' ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), 
			      (char *) malloc(strlen($1) + strlen($3) + strlen($7) + 6),
			      NULL, FUNC_KIND);
  sprintf(elem->name,"%s<%s>::~%s", $1, $3, $7);
  free($1);
  free($3);
  free($7);
  $$ = elem;
}
	| scoped_identifier '<' template_instance_arg_list '>' CLCL '~' IDENTIFIER '(' ')'
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), 
			      (char *) malloc(strlen($1) + strlen($3) + strlen($7) + 6),
			      NULL, FUNC_KIND);
  sprintf(elem->name,"%s<%s>::~%s", $1, $3, $7);
  free($1);
  free($3);
  free($7);
  $$ = elem;
}
	;

constructor
	: IDENTIFIER '(' parameter_list_opt ')' throw_decl
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), $1, $3, FUNC_KIND);
  elem->throw_decl = $5;
/*   print_se(elem); */
  $$ = elem;
}
	;

constructor_skeleton
	: scoped_identifier '(' parameter_list_opt ')' throw_decl
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), $1, $3, FUNC_KIND);
  elem->throw_decl = $5;
/*  print_se(elem); */
  $$ = elem;
}
	| IDENTIFIER '<' template_instance_arg_list '>' CLCL IDENTIFIER '(' parameter_list_opt ')' throw_decl
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), 
			      (char *) malloc(strlen($1) + strlen($3) + strlen($6) + 5),
			      $8, FUNC_KIND);
  sprintf(elem->name,"%s<%s>::%s", $1, $3, $6);
  free($1);
  free($3);
  free($6);
  elem->throw_decl = $10;
  $$ = elem;
}
	| scoped_identifier '<' template_instance_arg_list '>' CLCL IDENTIFIER '(' parameter_list_opt ')' throw_decl
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), 
			      (char *) malloc(strlen($1) + strlen($3) + strlen($6) + 5),
			      $8, FUNC_KIND);
  sprintf(elem->name,"%s<%s>::%s", $1, $3, $6);
  free($1);
  free($3);
  free($6);
  elem->throw_decl = $10;
  $$ = elem;
}
	;

compound_statement
	: '{' { if (collectInlineDef() != 0) YYERROR; } '}'
	;

enum_specifier
	: ENUM '{' enumerator_list '}'		
{ 
  char *tmp_str = (char *) malloc(strlen($3) + 10);
  sprintf(tmp_str, "enum { %s }", $3);
  free($3);
  $$ = tmp_str;
}
	| ENUM IDENTIFIER '{' enumerator_list '}'
{ 
  char *tmp_str = (char *) malloc(strlen($2) + strlen($4) + 11);
  sprintf(tmp_str, "enum %s { %s }", $2, $4);
  free($2);
  free($4);
  $$ = tmp_str;
}
	| ENUM IDENTIFIER
{ 
  char *tmp_str = (char *) malloc(strlen($2) + 6);
  sprintf(tmp_str, "enum %s", $2);
  free($2);
  $$ = tmp_str;
}
	;

enumerator_list
	: enumerator
	| enumerator_list ',' enumerator
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 3);
  sprintf(tmp_str, "%s, %s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	;

enumerator
	: IDENTIFIER
	| IDENTIFIER '=' constant_value
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 2);
  sprintf(tmp_str, "%s=%s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	;

access_specifier_opt
	: /* nothing */
	| access_specifier
	;

access_specifier_list
	: access_specifier ':'
	| access_specifier_list access_specifier ':'
	;

access_specifier
	: PUBLIC
	| PRIVATE
	| PROTECTED
	;

unary_operator
	: '&'					{ $$ = strdup("&"); }
	| '*'					{ $$ = strdup("*"); }
	| '+'					{ $$ = strdup("+"); }
	| '-'					{ $$ = strdup("-"); }
	| '~'					{ $$ = strdup("~"); }
	| '!' 					{ $$ = strdup("!"); }
	;

binary_operator
	: '/'					{ $$ = strdup("/"); }
	| '%'					{ $$ = strdup("%"); }
	| '^'					{ $$ = strdup("^"); }
	| '|'					{ $$ = strdup("|"); }
	| '<'					{ $$ = strdup("<"); }
	| '>'					{ $$ = strdup(">"); }
	| ','					{ $$ = strdup(","); }
	;

assignment_operator
	: '='					{ $$ = strdup("="); }
	| MUL_ASSIGN				{ $$ = strdup("*="); }
	| DIV_ASSIGN				{ $$ = strdup("/="); }
	| MOD_ASSIGN				{ $$ = strdup("%="); }
	| ADD_ASSIGN				{ $$ = strdup("+="); }
	| SUB_ASSIGN				{ $$ = strdup("-="); }
	| LEFT_ASSIGN				{ $$ = strdup("<<="); }
	| RIGHT_ASSIGN				{ $$ = strdup(">>="); }
	| AND_ASSIGN				{ $$ = strdup("&="); }
	| XOR_ASSIGN				{ $$ = strdup("^="); }
	| OR_ASSIGN				{ $$ = strdup("|="); }
	;

any_operator
	: assignment_operator
	| unary_operator
	| binary_operator
	| NEW					{ $$ = strdup(" new"); }
	| DELETE				{ $$ = strdup(" delete"); }
	| PTR_OP				{ $$ = strdup("->"); }
	| MEM_PTR_OP				{ $$ = strdup("->*"); }
	| INC_OP				{ $$ = strdup("++"); }
	| DEC_OP				{ $$ = strdup("--"); }
	| LEFT_OP				{ $$ = strdup("<<"); }
	| RIGHT_OP				{ $$ = strdup(">>"); }
	| LE_OP					{ $$ = strdup("<="); }
	| GE_OP					{ $$ = strdup(">="); }
	| EQ_OP					{ $$ = strdup("=="); }
	| NE_OP					{ $$ = strdup("!="); }
	| AND_OP				{ $$ = strdup("&&"); }
	| OR_OP					{ $$ = strdup("||"); }
	| '[' ']'				{ $$ = strdup("[]"); }
	| '(' ')'				{ $$ = strdup("()"); }
	;

constant_value
	: expression
	| compound_statement			{ $$ = strdup("{ ... }"); }
	;

expression
	: additive_expression
	;

primary_expression
	: CONSTANT
	| '-' CONSTANT
{ 
  char *tmp_str = (char *) malloc(strlen($2) + 2);
  sprintf(tmp_str, "-%s", $2);
  free($2);
  $$ = tmp_str; 
}
	| STRING_LITERAL
	| IDENTIFIER
	| scoped_identifier
	| '(' expression ')'
{ 
  char *tmp_str = (char *) malloc(strlen($2) + 3);
  sprintf(tmp_str, "(%s)", $2);
  free($2);
  $$ = tmp_str; 
}
	;

multiplicative_expression
	: primary_expression
	| multiplicative_expression '*' primary_expression
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 4);
  sprintf(tmp_str, "%s * %s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str; 
}
	| multiplicative_expression '/' primary_expression
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 4);
  sprintf(tmp_str, "%s / %s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str; 
}
	| multiplicative_expression '%' primary_expression
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 4);
  sprintf(tmp_str, "%s %% %s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str; 
}
	;

additive_expression
	: multiplicative_expression
	| additive_expression '+' multiplicative_expression
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 4);
  sprintf(tmp_str, "%s + %s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str; 
}
	| additive_expression '-' multiplicative_expression
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 4);
  sprintf(tmp_str, "%s - %s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str; 
}
	;


union_specifier
	: UNION IDENTIFIER '{' variable_specifier_list '}'
{ 
  char *tmp_str = (char *) malloc(strlen($2) + strlen($4) + 12);
  sprintf(tmp_str, "union %s { %s }", $2, $4);
  free($2);
  free($4);
  $$ = tmp_str;
}
	| UNION '{' variable_specifier_list '}'
{ 
  char *tmp_str = (char *) malloc(strlen($3) + 11);
  sprintf(tmp_str, "union { %s }", $3);
  free($3);
  $$ = tmp_str;
}
	| UNION IDENTIFIER
{ 
  char *tmp_str = (char *) malloc(strlen($2) + 7);
  sprintf(tmp_str, "union %s", $2);
  free($2);
  $$ = tmp_str;
}
	;

class_specifier
	: CLASS IDENTIFIER '{' member_list_opt '}'
{
  syntaxelem_t *child;
  /* ret_type, name, args, kind */
  syntaxelem_t *tmp_elem = new_elem(strdup(""), $2, NULL, CLASS_KIND);
  tmp_elem->children = reverse_list($4);
  
  for (child = tmp_elem->children; child != NULL; child = child->next)
      child->parent = tmp_elem;

/*   print_se(tmp_elem); */
  $$ = tmp_elem;
}
	| CLASS IDENTIFIER ':' superclass_list '{' member_list_opt '}'
{
  syntaxelem_t *child;
  /* ret_type, name, args, kind */
  syntaxelem_t *tmp_elem = new_elem(strdup(""), $2, NULL, CLASS_KIND);
  tmp_elem->children = reverse_list($6);
  
  for (child = tmp_elem->children; child != NULL; child = child->next)
      child->parent = tmp_elem;

/*   print_se(tmp_elem); */
  $$ = tmp_elem;
}
	| template_specifier CLASS IDENTIFIER '{' member_list_opt '}'
{
  syntaxelem_t *child;
  /* ret_type, name, args, kind */
  syntaxelem_t *tmp_elem = new_elem(strdup(""), $3, NULL, CLASS_KIND);
  tmp_elem->children = reverse_list($5);
  tmp_elem->templ = $1;
  
  for (child = tmp_elem->children; child != NULL; child = child->next)
      child->parent = tmp_elem;

/*   print_se(tmp_elem); */
  $$ = tmp_elem;
}
	| template_specifier CLASS IDENTIFIER ':' superclass_list '{' member_list_opt '}'
{
  syntaxelem_t *child;
  /* ret_type, name, args, kind */
  syntaxelem_t *tmp_elem = new_elem(strdup(""), $3, NULL, CLASS_KIND);
  tmp_elem->children = reverse_list($7);
  tmp_elem->templ = $1;
  
  for (child = tmp_elem->children; child != NULL; child = child->next)
      child->parent = tmp_elem;

/*   print_se(tmp_elem); */
  $$ = tmp_elem;
}
	| STRUCT '{' member_list_opt '}'
{
  syntaxelem_t *child;
  /* ret_type, name, args, kind */
  syntaxelem_t *tmp_elem = new_elem(strdup(""), "unnamed_struct",
				    NULL, IGNORE_KIND);
  tmp_elem->children = reverse_list($3);
  
  for (child = tmp_elem->children; child != NULL; child = child->next)
      child->parent = tmp_elem;

/*   print_se(tmp_elem); */
  $$ = tmp_elem;
}
	| STRUCT IDENTIFIER '{' member_list_opt '}'
{
  syntaxelem_t *child;
  /* ret_type, name, args, kind */
  syntaxelem_t *tmp_elem = new_elem(strdup(""), $2, NULL, STRUCT_KIND);
  tmp_elem->children = reverse_list($4);
  
  for (child = tmp_elem->children; child != NULL; child = child->next)
      child->parent = tmp_elem;

/*   print_se(tmp_elem); */
  $$ = tmp_elem;
}
	| STRUCT IDENTIFIER ':' superclass_list '{' member_list_opt '}'
{
  syntaxelem_t *child;
  /* ret_type, name, args, kind */
  syntaxelem_t *tmp_elem = new_elem(strdup(""), $2, NULL, STRUCT_KIND);
  tmp_elem->children = reverse_list($6);
  
  for (child = tmp_elem->children; child != NULL; child = child->next)
      child->parent = tmp_elem;

/*   print_se(tmp_elem); */
  $$ = tmp_elem;
}
	| template_specifier STRUCT IDENTIFIER '{' member_list_opt '}'
{
  syntaxelem_t *child;
  /* ret_type, name, args, kind */
  syntaxelem_t *tmp_elem = new_elem(strdup(""), $3, NULL, STRUCT_KIND);
  tmp_elem->children = reverse_list($5);
  tmp_elem->templ = $1;
  
  for (child = tmp_elem->children; child != NULL; child = child->next)
      child->parent = tmp_elem;

/*   print_se(tmp_elem); */
  $$ = tmp_elem;
}
	| template_specifier STRUCT IDENTIFIER ':' superclass_list '{' member_list_opt '}'
{
  syntaxelem_t *child;
  /* ret_type, name, args, kind */
  syntaxelem_t *tmp_elem = new_elem(strdup(""), $3, NULL, STRUCT_KIND);
  tmp_elem->children = reverse_list($7);
  tmp_elem->templ = $1;
  
  for (child = tmp_elem->children; child != NULL; child = child->next)
      child->parent = tmp_elem;

/*   print_se(tmp_elem); */
  $$ = tmp_elem;
}
	;

superclass_list
	: superclass
	| superclass_list ',' superclass
	;

superclass
	: access_specifier_opt type_name	{ free($2); }
	;

friend_specifier
	: FRIEND member_func_specifier		{ $2->kind = IGNORE_KIND; }
	| FRIEND forward_decl
	;

member_list_opt
	: /* nothing  */ { $$ = NULL; }
	| member_list
	;

member_list
	: member_with_access			
{
  if ($1 != NULL)
    $1->next = NULL;
  
  $$ = $1; 
}
	| member_list member_with_access	
{ 
  if ($2 != NULL) {
    $2->next = $1;
    $$ = $2; 
  } else {
    $$ = $1;
  }
}
	;

member_or_error
	: member
	| error
{
  log_printf("member_with_access : error.  Attempting to recover...\n");
  yyerrok;
  yyclearin;
  if (error_recovery() != 0) {
      log_printf("ERROR recovery could not complete -- YYABORT.\n");
      YYABORT;
  }
  log_printf("ERROR recovery complete.\n");
  $$ = NULL;
}
	;

member_with_access
	: member_or_error
	| access_specifier_list member_or_error	{ $$ = $2; }
	;

member
	: member_specifier ';'
	| friend_specifier ';'		{ $$ = NULL; }
	| member_func_inlined		{ $1->kind = INLINED_KIND; $$ = $1; }
	| ';'				{ $$ = NULL; }
	;

member_specifier
	: multiple_variable_specifier
{ 
  /* ret_type, name, args, kind */
  syntaxelem_t *elem = new_elem(strdup(""), $1, NULL, IGNORE_KIND);
/*   print_se(elem); */
  $$ = elem;
}
	| member_func_specifier
	| EXTERN member_func_specifier 		{ $$ = $2; }
	| member_func_specifier '=' CONSTANT	{ $1->kind = IGNORE_KIND; free($3); $$ = $1; }
	| mem_type_specifier
	;

mem_type_specifier
	: type_specifier
	| type_specifier IDENTIFIER		{ free($2); $$ = $1; }
	| mem_type_specifier '[' ']'
	| mem_type_specifier '[' constant_value ']'
{
  free($3);
  $$ = $1; 
}
	;

template_arg
	: CLASS IDENTIFIER
{
  char *tmp_str = (char *) malloc(strlen($2) + 7);
  sprintf(tmp_str, "class %s", $2);
  free($2);
  $$ = tmp_str;
}
	| type_name IDENTIFIER
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($2) + 2);
  sprintf(tmp_str, "%s %s", $1, $2);
  free($1);
  free($2);
  $$ = tmp_str;
}
	;

template_arg_list
	: template_arg
	| template_arg_list ',' template_arg
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 3);
  sprintf(tmp_str, "%s, %s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	;

template_instance_arg
	: CONSTANT
	| type_name
	;

template_instance_arg_list
	: template_instance_arg
	| template_instance_arg_list ',' template_instance_arg
{ 
  char *tmp_str = (char *) malloc(strlen($1) + strlen($3) + 3);
  sprintf(tmp_str, "%s, %s", $1, $3);
  free($1);
  free($3);
  $$ = tmp_str;
}
	;

template_specifier
	: TEMPLATE '<' template_arg_list '>'
{
  char *tmp_str = (char *) malloc(strlen($3) + 12);
  sprintf(tmp_str, "template <%s>", $3);
  free($3);
  $$ = tmp_str;
}
	;

%%

static int yyerror(char *s /*UNUSED*/)
{
  if (outfile != NULL)
    fflush(outfile);

  return 0;
}

static int error_recovery()
{
  extern char linebuf[];
  extern int lineno;
  extern int column;
  extern int tokens_seen;

#ifdef SGDEBUG
  log_printf("parse error at line %d, file %s:\n%s\n%*s\n",
	     lineno, currentFile, linebuf, column, "^");
  log_flush();
#endif /* SGDEBUG */

  if (tokens_seen == 0) {
    /*
     * if we've seen no tokens but we're in an error, we must have
     * hit an EOF, either by stdin, or on a file.  Just give up
     * now instead of complaining.
     */
    return -1;

  } else {
    fprintf(stderr, "parse error at line %d, file %s:\n%s\n%*s\n",
	    lineno, currentFile, linebuf, column, "^");
  }

  linebuf[0] = '\0';

  for (;;) {
    int result = yylex();

    if (result <= 0) {
      /* fatal error: Unexpected EOF during parse error recovery */

#ifdef SGDEBUG
      log_printf("EOF in error recovery, line %d, file %s\n",
		 lineno, currentFile);
      log_flush();
#endif /* SGDEBUG */

      fprintf(stderr, "EOF in error recovery, line %d, file %s\n",
	      lineno, currentFile);

      return -1;
    }

    switch(result) {
    case IDENTIFIER:
    case CONSTANT:
    case STRING_LITERAL:
    case CHAR:
    case SHORT:
    case INT:
    case LONG:
    case SIGNED:
    case UNSIGNED:
    case FLOAT:
    case DOUBLE:
    case VOID:
      free(yylval.string);
      break;
    case (int) '{':
      if (collectInlineDef() != 0)
	return -1;
      result = yylex();
      return 0;
    case (int) ';':
      return 0;
    }
  }
}
