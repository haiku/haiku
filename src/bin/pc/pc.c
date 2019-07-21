/*
               PC -- programmer's calculator.

   This program implements a simple recursive descent parser that
understands pretty much all standard C language math and logic
expressions.  It handles the usual add, subtract, multiply, divide,
and mod sort of stuff.  It can also deal with logical/relational
operations and expressions.  The logic/relational operations AND, OR,
NOT, and EXCLUSIVE OR, &&, ||, ==, !=, <, >, <=, and >= are all
supported.  It also handles parens and nested expresions as well as
left and right shifts.  There are variables and assignments (as well
as assignment operators like "*=").

   The other useful feature is that you can use "." in an expression
to refer to the value from the previous expression (just like bc).

   Multiple statements can be separated by semi-colons (;) on a single
line (though a single statement can't span multiple lines).   

   This calculator is mainly a programmers calculator because it
doesn't work in floating point and only deals with integers.

   I wrote this because the standard unix calculator (bc) doesn't
offer a useful modulo, it doesn't have left and right shifts and
sometimes it is a pain in the ass to use (but I still use bc for
things that require any kind of floating point).  This program is
great when you have to do address calculations and bit-wise
masking/shifting as you do when working on kernel type code.  It's
also handy for doing quick conversions between decimal, hex and ascii
(and if you want to see octal for some reason, just put it in the
printf string).

   The parser is completely separable and could be spliced into other
code very easy.  The routine parse_expression() just expects a char
pointer and returns the value.  Implementing command line editing
would be easy using a readline() type of library.

   This isn't the world's best parser or anything, but it works and
suits my needs.  It faithfully implements C style precedence of
operators for:

        ++ -- ~ ! * / % + - << >> < > <= >= == != & ^ | && ||

(in that order, from greatest to least precedence).

   Note: The ! unary operator is a logical negation, not a bitwise
negation (if you want bitwise negation, use ~).

   I've been working on adding variables and assignments, and I've
just (10/26/94) got it working right and with code I'm not ashamed of.
Now you can have variables (no restrictions on length) and assign to
them and use them in expressions.  Variable names have the usual C
rules (i.e. alpha or underscore followed by alpha-numeric and
underscore).  Variables are initialized to zero and created as needed.
You can have any number of variables. Here are some examples:

      x = 5
      x = y = 10
      x = (y + 5) * 2
      (y * 2) + (x & 0xffeef)


   Assignment operators also work.  The allowable assignment operators
are (just as in C):

       +=, -=, *=, /=, %=, &=, ^=, |=, <<=, and >>=


   The basic ideas for this code came from the book "Compiler Design
in C", by Allen I. Holub, but I've extended them significantly.

   If you find bugs or parsing bogosites, I'd like to know about them
so I can fix them.  Other comments and criticism of the code are
welcome as well.
 
   Thanks go to Joel Tesler (joel@engr.sgi.com) for adding the ability
to pass command line arguments and have pc evaluate them instead of reading
from stdin.

      Dominic Giampaolo
      dbg@be.com (though this was written while I was at sgi)
 */
#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <ctype.h>
#include "lex.h"


/*
 * You should #define USE_LONG_LONG if your compiler supports the long long
 * data type, has strtoull(), and a %lld conversion specifier for printf.
 * Otherwise just comment out the #define and pc will use plain longs.
 */

#define  USE_LONG_LONG


#ifdef  USE_LONG_LONG
#define LONG     long long
#define ULONG    unsigned long long
#define STRTOUL  strtoull
#define STR1     "%20llu  0x%.16llx signed: %20lld" 
#define STR2     "%20llu  0x%.16llx"
#define STR3     " char: "
#define STR4     "%20lu  0x%-16.8lx signed: %20ld" 
#define STR5     "%20lu  0x%-16.8lx"
#else
#define LONG     long
#define ULONG    unsigned long
#define STRTOUL  strtoul
#define STR1     "%10lu\t 0x%.8lx\t signed: %10ld" 
#define STR2     "%10lu\t 0x%.8lx"
#define STR3     " char: "
#define STR4     STR1
#define STR5     STR2
#endif


ULONG parse_expression(char *str);   /* top-level interface to parser */
ULONG assignment_expr(char **str);   /* assignments =, +=, *=, etc */
ULONG do_assignment_operator(char **str, char *var_name);
ULONG logical_or_expr(char **str);   /* logical OR  `||' */
ULONG logical_and_expr(char **str);  /* logical AND  `&&' */
ULONG or_expr(char **str);           /* OR  `|' */
ULONG xor_expr(char **str);          /* XOR `^' */
ULONG and_expr(char **str);          /* AND `&' */
ULONG equality_expr(char **str);     /* equality ==, != */
ULONG relational_expr(char **str);   /* relational <, >, <=, >= */
ULONG shift_expr(char **str);        /* shifts <<, >> */
ULONG add_expression(char **str);    /* addition/subtraction +, - */
ULONG term(char **str);              /* multiplication/division *,%,/ */
ULONG factor(char **str);            /* negation, logical not ~, ! */
ULONG get_value(char **str);
int   get_var(char *name, ULONG *val); /* external interfaces to vars */
void  set_var(char *name, ULONG val);

void  do_input(void);                /* reads stdin and calls parser */
char *skipwhite(char *str);          /* skip over input white space */



/*
 * Variables are kept in a simple singly-linked list. Not high
 * performance, but it's also an extremely small implementation.
 *
 * New variables get added to the head of the list.  Variables are
 * never deleted, though it wouldn't be hard to do that.
 *
 */
typedef struct variable 
{
  char  *name;
  ULONG  value;
  struct variable *next;
}variable;

variable dummy = { NULL, 0L, NULL };
variable *vars=&dummy;

variable *lookup_var(char *name);
variable *add_var(char *name, ULONG value);
char     *get_var_name(char **input_str);
void      parse_args(int argc, char *argv[]);
int (*set_var_lookup_hook(int (*func)(char *name, ULONG *val)))
                         (char *name, ULONG *val);

/*
 * last_result is equal to the result of the last expression and 
 * expressions can refer to it as `.' (just like bc).
 */
ULONG last_result = 0;


static int
special_vars(char *name, ULONG *val)
{
  if (strcmp(name, "time") == 0)
    *val = (ULONG)time(NULL);
  else if (strcmp(name, "rand") == 0)
    *val = (ULONG)rand();
  else if (strcmp(name, "dbg") == 0)
    *val = 0x82969;
  else
    return 0;

  return 1;
}


int
main(int argc, char *argv[])
{

  set_var_lookup_hook(special_vars);

  if (argc > 1)
    parse_args(argc, argv);
  else
    do_input();
  
  return 0;
}

/*
   This function prints the result of the expression.
   It tries to be smart about printing numbers so it
   only uses the necessary number of digits.  If you
   have long long (i.e. 64 bit numbers) it's very
   annoying to have lots of leading zeros when they
   aren't necessary.  By doing the somewhat bizarre
   casting and comparisons we can determine if a value
   will fit in a 32 bit quantity and only print that.
*/
static void
print_result(ULONG value)
{
  int i;
  ULONG ch, shift;
  
  if ((signed LONG)value < 0)
   {
     if ((signed LONG)value < (signed LONG)(LONG_MIN))
       printf(STR1, value, value, value);
     else 
       printf(STR4, (long)value, (long)value, (long)value);
   }
  else if ((ULONG)value <= (ULONG)ULONG_MAX)
    printf(STR5, (long)value, (long)value);
  else
    printf(STR2, value, value);
     
  /*
     Print any printable character (and print dots for unprintable chars
  */  
  printf(STR3);
  for(i=sizeof(ULONG)-1; i >= 0; i--)
   {
     shift = i * 8;
     ch = ((ULONG)value & ((ULONG)0xff << shift)) >> shift;

     if (isprint((int)ch))
       printf("%c", (char)(ch));
     else
       printf(".");
   }

  printf("\n");
}

void
parse_args(int argc, char *argv[])
{
  int i, len;
  char *buff;
  ULONG value;

  for(i=1, len=0; i < argc; i++)
    len += strlen(argv[i]);
  len++;

  buff = malloc(len*sizeof(char));
  if (buff == NULL)
    return;

  buff[0] = '\0';
  while (--argc > 0)
  {
    strcat(buff, *++argv);
  }
  value = parse_expression(buff);

  print_result(value);

  free(buff);
}
  
void
do_input(void)
{
  ULONG value;
  char buff[256], *ptr;
  
  while(fgets(buff, 256, stdin) != NULL)
   {
     if (buff[0] != '\0' && buff[strlen(buff)-1] == '\n')
       buff[strlen(buff)-1] = '\0';     /* kill the newline character */

     for(ptr=buff; isspace(*ptr) && *ptr; ptr++)
        /* skip whitespace */;

     if (*ptr == '\0')    /* hmmm, an empty line, just skip it */
       continue;
     
     value = parse_expression(buff);
     
     print_result(value);
   }
}


ULONG
parse_expression(char *str)
{
  ULONG val;
  char *ptr = str;

  ptr = skipwhite(ptr);
  if (*ptr == '\0')
    return last_result;

  val = assignment_expr(&ptr);

  ptr = skipwhite(ptr);
  while (*ptr == SEMI_COLON && *ptr != '\0')
   {
     ptr++;
     if (*ptr == '\0')   /* reached the end of the string, stop parsing */
       continue;
     
     val = assignment_expr(&ptr);
   }

  last_result = val;
  return val;
}


ULONG
assignment_expr(char **str)
{
  ULONG val; 
  char *orig_str;
  char *var_name;
  variable *v;
  
  *str = skipwhite(*str);
  orig_str = *str;

  var_name = get_var_name(str);

  *str = skipwhite(*str);
  if (**str == EQUAL && *(*str+1) != EQUAL)
   {
     *str = skipwhite(*str + 1);     /* skip the equal sign */

     val = assignment_expr(str);     /* go recursive! */

     if ((v = lookup_var(var_name)) == NULL)
       add_var(var_name, val);
     else
       v->value = val;
   }
  else if (((**str == PLUS  || **str == MINUS    || **str == OR ||
	     **str == TIMES || **str == DIVISION || **str == MODULO ||
	     **str == AND   || **str == XOR) && *(*str+1) == EQUAL) ||
	   strncmp(*str, "<<=", 3) == 0 || strncmp(*str, ">>=", 3) == 0)
   {
     val = do_assignment_operator(str, var_name);
   }
  else
   {
     *str = orig_str;
     val = logical_or_expr(str);     /* no equal sign, just get var value */

     *str = skipwhite(*str);
     if (**str == EQUAL)
      {
	fprintf(stderr, "Left hand side of expression is not assignable.\n");
      }
   }

  if (var_name)
    free(var_name);

  return val;
}


ULONG
do_assignment_operator(char **str, char *var_name)
{
  ULONG val;
  variable *v;
  char operator;
  
  operator = **str;

  if (operator == SHIFT_L || operator == SHIFT_R)
    *str = skipwhite(*str + 3);
  else
    *str = skipwhite(*str + 2);     /* skip the assignment operator */

  val = assignment_expr(str);       /* go recursive! */

  v = lookup_var(var_name);
  if (v == NULL)
   {
     v = add_var(var_name, 0);
     if (v == NULL)
       return 0;
   }
  
  if (operator == PLUS)
    v->value += val;
  else if (operator == MINUS)
    v->value -= val;
  else if (operator == AND)
    v->value &= val;
  else if (operator == XOR)
    v->value ^= val;
  else if (operator == OR)
    v->value |= val;
  else if (operator == SHIFT_L)
    v->value <<= val;
  else if (operator == SHIFT_R)
    v->value >>= val;
  else if (operator == TIMES)
    v->value *= val;
  else if (operator == DIVISION)
   {
     if (val == 0)  /* check for it, but still get the result */
       fprintf(stderr, "Divide by zero!\n");

     v->value /= val;
   }
  else if (operator == MODULO)
   {
     if (val == 0)  /* check for it, but still get the result */
       fprintf(stderr, "Modulo by zero!\n");

     v->value %= val;
   }
  else
   {
     fprintf(stderr, "Unknown operator: %c\n", operator);
     v->value = 0;
   }

  return v->value;
}


ULONG
logical_or_expr(char **str)
{
  ULONG val, sum = 0; 
  
  *str = skipwhite(*str);

  sum = logical_and_expr(str);

  *str = skipwhite(*str);
  while(**str == OR && *(*str + 1) == OR)
   {
     *str = skipwhite(*str + 2);   /* advance over the operator */
     
     val = logical_and_expr(str);

     sum = (val || sum);
   }

  return sum;
}



ULONG
logical_and_expr(char **str)
{
  ULONG val, sum = 0; 
  
  *str = skipwhite(*str);

  sum = or_expr(str);

  *str = skipwhite(*str);
  while(**str == AND && *(*str + 1) == AND)
   {
     *str = skipwhite(*str + 2);   /* advance over the operator */
     
     val = or_expr(str);

     sum = (val && sum);
   }

  return sum;
}


ULONG
or_expr(char **str)
{
  ULONG val, sum = 0; 
  
  *str = skipwhite(*str);

  sum = xor_expr(str);

  *str = skipwhite(*str);
  while(**str == OR && *(*str+1) != OR)
   {
     *str = skipwhite(*str + 1);   /* advance over the operator */
     
     val = xor_expr(str);

     sum |= val;
   }

  return sum;
}



ULONG
xor_expr(char **str)
{
  ULONG val, sum = 0; 
  
  *str = skipwhite(*str);

  sum = and_expr(str);

  *str = skipwhite(*str);
  while(**str == XOR)
   {
     *str = skipwhite(*str + 1);   /* advance over the operator */
     
     val = and_expr(str);

     sum ^= val;
   }

  return sum;
}



ULONG
and_expr(char **str)
{
  ULONG val, sum = 0; 
  
  *str = skipwhite(*str);

  sum = equality_expr(str);

  *str = skipwhite(*str);
  while(**str == AND && *(*str+1) != AND)
   {
     *str = skipwhite(*str + 1);   /* advance over the operator */
     
     val = equality_expr(str);

     sum &= val;
   }

  return sum;
}


ULONG 
equality_expr(char **str)
{
  ULONG val, sum = 0; 
  char op;
  
  *str = skipwhite(*str);

  sum = relational_expr(str);

  *str = skipwhite(*str);
  while((**str == EQUAL && *(*str+1) == EQUAL) ||
	(**str == BANG  && *(*str+1) == EQUAL))
   {
     op = **str;
     
     *str = skipwhite(*str + 2);   /* advance over the operator */
     
     val = relational_expr(str);

     if (op == EQUAL)
       sum = (sum == val);
     else if (op == BANG)
       sum = (sum != val);
   }

  return sum;
}


ULONG 
relational_expr(char **str)
{
  ULONG val, sum = 0; 
  char op, equal_to=0;
  
  *str = skipwhite(*str);

  sum = shift_expr(str);

  *str = skipwhite(*str);
  while(**str == LESS_THAN || **str == GREATER_THAN)
   {
     equal_to = 0;
     op = **str;

     if (*(*str+1) == EQUAL)
      {
	equal_to = 1;
	*str = *str+1;             /* skip initial operator */
      }
       
     *str = skipwhite(*str + 1);   /* advance over the operator */
     
     val = shift_expr(str);

     /*
       Notice that we do the relational expressions as signed
       comparisons.  This is because of expressions like:
             0 > -1
       which would not return the expected value if we did the
       comparison as unsigned.  This may not always be the
       desired behavior, but aside from adding casting to epxressions,
       there isn't much of a way around it.
     */
     if (op == LESS_THAN && equal_to == 0)
       sum = ((LONG)sum < (LONG)val);
     else if (op == LESS_THAN && equal_to == 1)
       sum = ((LONG)sum <= (LONG)val);
     else if (op == GREATER_THAN && equal_to == 0)
       sum = ((LONG)sum > (LONG)val);
     else if (op == GREATER_THAN && equal_to == 1)
       sum = ((LONG)sum >= (LONG)val);
   }

  return sum;
}


ULONG
shift_expr(char **str)
{
  ULONG val, sum = 0; 
  char op;
  
  *str = skipwhite(*str);

  sum = add_expression(str);

  *str = skipwhite(*str);
  while((strncmp(*str, "<<", 2) == 0) || (strncmp(*str, ">>", 2) == 0))
   {
     op = **str;
     
     *str = skipwhite(*str + 2);   /* advance over the operator */
     
     val = add_expression(str);

     if (op == SHIFT_L)
       sum <<= val;
     else if (op == SHIFT_R)
       sum >>= val;
   }

  return sum;
}




ULONG
add_expression(char **str)
{
  ULONG val, sum = 0; 
  char op;
  
  *str = skipwhite(*str);

  sum = term(str);

  *str = skipwhite(*str);
  while(**str == PLUS || **str == MINUS)
   {
     op = **str;
     
     *str = skipwhite(*str + 1);   /* advance over the operator */
     
     val = term(str);

     if (op == PLUS)
       sum += val;
     else if (op == MINUS)
       sum -= val;
   }

  return sum;
}




ULONG
term(char **str)
{
  ULONG val, sum = 0;
  char op;


  sum = factor(str);
  *str = skipwhite(*str);

  /*
   * We're at the bottom of the parse.  At this point we either have
   * an operator or we're through with this string.  Otherwise it's
   * an error and we print a message.
   */
  if (**str != TIMES     && **str != DIVISION && **str != MODULO &&
      **str != PLUS      && **str != MINUS    && **str != OR     &&
      **str != AND       && **str != XOR      && **str != BANG   &&
      **str != NEGATIVE  && **str != TWIDDLE  && **str != RPAREN &&
      **str != LESS_THAN && **str != GREATER_THAN && **str != SEMI_COLON &&
      strncmp(*str, "<<", 2) != 0 && strncmp(*str, ">>", 2) &&      
      **str != EQUAL && **str != '\0')
   {
     fprintf(stderr, "Parsing stopped: unknown operator %s\n", *str);
     return sum;
   }

  while(**str == TIMES || **str == DIVISION || **str == MODULO)
   {
     op   = **str;
     *str = skipwhite(*str + 1);
     val = factor(str);
     
     if (op == TIMES)
       sum *= val;
     else if (op == DIVISION)
      {
	if (val == 0)
	  fprintf(stderr, "Divide by zero!\n");

	sum /= val;
      }
     else if (op == MODULO)
      {
	if (val == 0)
	  fprintf(stderr, "Modulo by zero!\n");

	sum %= val;
      }
   }

  return sum;
}


ULONG
factor(char **str)
{
  ULONG val=0;
  char op = NOTHING, have_special=0;
  char *var_name, *var_name_ptr;
  variable *v;

  if (**str == NEGATIVE || **str == PLUS || **str == TWIDDLE || **str == BANG)
   {
     op = **str;                     /* must be a unary op */

     if ((op == NEGATIVE && *(*str + 1) == NEGATIVE) ||  /* look for ++/-- */
	 (op == PLUS     && *(*str + 1) == PLUS))
      {
	*str = *str + 1;
	have_special = 1;
      }
     
     *str = skipwhite(*str + 1);
     var_name_ptr = *str;          /* save where the varname should be */
   }
    
  val = get_value(str);

  *str = skipwhite(*str);
  
  /*
   * Now is the time to actually do the unary operation if one
   * was present.
   */
  if (have_special)   /* we've got a ++ or -- */
   {
     var_name = get_var_name(&var_name_ptr);
     if (var_name == NULL)
      {
	fprintf(stderr, "Can only use ++/-- on variables.\n");
	return val;
      }
     if ((v = lookup_var(var_name)) == NULL)
      {
	v = add_var(var_name, 0);
	if (v == NULL)
	  return val;
      }
     free(var_name);

     if (op == PLUS)
	val = ++v->value;
     else
	val = --v->value;
   }
  else                   /* normal unary operator */
   {
     switch(op)
      {
	case NEGATIVE : val *= -1;
	                break;

	case BANG     : val = !val;
	                break;
	
	case TWIDDLE  : val = ~val;
                        break;
      }
   }

  return val;
}



ULONG
get_value(char **str)
{
  ULONG val;
  char *var_name;
  variable *v;
  
  if (**str == SINGLE_QUOTE)         /* a character constant */
   {
     unsigned int i;
     
     *str = *str + 1;                /* advance over the leading quote */
     val = 0;
     for(i=0; **str && **str != SINGLE_QUOTE && i < sizeof(LONG); *str+=1,i++)
      {
	if (**str == '\\')  /* escape the next char */
	  *str += 1;
	
	val <<= 8;
	val |= (ULONG)((unsigned)**str);
      }

     if (**str != SINGLE_QUOTE)       /* constant must have been too long */
      {
	fprintf(stderr, "Warning: character constant not terminated or too "
		"long (max len == %ld bytes)\n", sizeof(LONG));
	while(**str && **str != SINGLE_QUOTE)
	  *str += 1;
      }
     else if (**str != '\0')
       *str += 1;
   }
  else if (isdigit(**str))            /* a regular number */
   {
     val = STRTOUL(*str, str, 0);

     *str = skipwhite(*str);
   }
  else if (**str == USE_LAST_RESULT)  /* a `.' meaning use the last result */
   {
     val = last_result;
     *str = skipwhite(*str+1);
   }
  else if (**str == LPAREN)           /* a parenthesized expression */
   {
     *str = skipwhite(*str + 1);

     val = assignment_expr(str);      /* start at top and come back down */

     if (**str == RPAREN)
       *str = *str + 1;
     else
       fprintf(stderr, "Mismatched paren's\n");
   }
  else if (isalpha(**str) || **str == '_')          /* a variable name */
   {
     if ((var_name = get_var_name(str)) == NULL)
      {
	fprintf(stderr, "Can't get var name!\n");
	return 0;	
      }
     
     if (get_var(var_name, &val) == 0)
      {
	fprintf(stderr, "No such variable: %s (assigning value of zero)\n",
		var_name);

	val = 0;
	v = add_var(var_name, val);
	if (v == NULL)
	  return 0;
      }
     
     *str = skipwhite(*str);
     if (strncmp(*str, "++", 2) == 0 || strncmp(*str, "--", 2) == 0)
      {
	if ((v = lookup_var(var_name)) != NULL)
	  {
	    val = v->value;
	    if (**str == '+')
	      v->value++;
	    else 
	      v->value--;
	    *str = *str + 2;
	  }
	else
	  {
	    fprintf(stderr, "%s is a read-only variable\n", var_name);
	  }
      }

     free(var_name);
   }
  else
   {
     fprintf(stderr, "Expecting left paren, unary op, constant or variable.");
     fprintf(stderr, "  Got: `%s'\n", *str);
     return 0;
   }


  return val;
}


/*
 * Here are the functions that manipulate variables.
 */

/*
   this is a hook function for external read-only
   variables.  If it is set and we don't find a
   variable name in our name space, we call it to
   look for the variable.  If it finds the name, it
   fills in val and returns 1. If it returns 0, it
   didn't find the variable.
*/   
static int (*external_var_lookup)(char *name, ULONG *val) = NULL;

/*
   this very ugly function declaration is for the function
   set_var_lookup_hook which accepts one argument, "func", which
   is a pointer to a function that returns int (and accepts a
   char * and ULONG *).  set_var_lookup_hook returns a pointer to
   a function that returns int and accepts char * and ULONG *.

   It's very ugly looking but fairly basic in what it does.  You
   pass in a function to set as the variable name lookup up hook
   and it passes back to you the old function (which you should
   call if it is non-null and your function fails to find the
   variable name).
*/   
int (*set_var_lookup_hook(int (*func)(char *name, ULONG *val)))(char *name, ULONG *val)
{
  int (*old_func)(char *name, ULONG *val) = external_var_lookup;

  external_var_lookup = func;

  return old_func;
}


variable *
lookup_var(char *name)
{
  variable *v;

  for(v=vars; v; v=v->next)
    if (v->name && strcmp(v->name, name) == 0)
      return v;

  return NULL;
}


variable *
add_var(char *name, ULONG value)
{
  variable *v;
  ULONG tmp;

  /* first make sure this isn't an external read-only variable */
  if (external_var_lookup)
    if (external_var_lookup(name, &tmp) != 0)
     {
       fprintf(stderr, "Can't assign/create %s, it is a read-only var\n",name);
       return NULL;	 
     }
      
      
  v = (variable *)malloc(sizeof(variable));
  if (v == NULL)
   {
     fprintf(stderr, "No memory to add variable: %s\n", name);
     return NULL;
   }

  v->name = strdup(name);
  v->value = value;
  v->next = vars;

  vars = v;  /* set head of list to the new guy */

  return v;
}

/*
   This routine and the companion get_var() are external
   interfaces to the variable manipulation routines.
*/   
void
set_var(char *name, ULONG val)
{
  variable *v;

  v = lookup_var(name);
  if (v != NULL)
    v->value = val;
  else
    add_var(name, val);
}


/*
   This function returns 1 on success of finding
   a variable and 0 on failure to find a variable.
   If a variable is found, val is filled with its
   value.
*/   
int
get_var(char *name, ULONG *val)
{
  variable *v;

  v = lookup_var(name);
  if (v != NULL)
   {
     *val = v->value;
     return 1;
   }
  else if (external_var_lookup != NULL)
   {
     return external_var_lookup(name, val);
   }
    
    return 0;
}

#define DEFAULT_LEN 32

char *
get_var_name(char **str)
{
  int i, len=DEFAULT_LEN;
  char *buff;

  if (isalpha(**str) == 0 && **str != '_')
    return NULL;

  buff = (char *)malloc(len*sizeof(char));
  if (buff == NULL)
    return NULL;

  /*
   * First get the variable name
   */
  i=0;
  while(**str && (isalnum(**str) || **str == '_'))
   {
     if (i >= len-1)
      {
	len *= 2;
	buff = (char *)realloc(buff, len);
	if (buff == NULL)
	  return NULL;
      }
     
     buff[i++] = **str;
     *str = *str+1;
   }

  buff[i] = '\0';  /* null terminate */

  while (isalnum(**str) || **str == '_')  /* skip over any remaining junk */
    *str = *str+1;

  return buff;
}



char *
skipwhite(char *str)
{
  if (str == NULL)
    return NULL;
  
  while(*str && (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\f'))
    str++;

  return str;
}
