m4_divert(-1)                                               -*- Autoconf -*-

# C M4 Macros for Bison.
# Copyright (C) 2002, 2004, 2005 Free Software Foundation, Inc.

# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
# 02110-1301  USA


## ---------------- ##
## Identification.  ##
## ---------------- ##

# b4_copyright(TITLE, YEARS)
# --------------------------
m4_define([b4_copyright],
[/* A Bison parser, made by GNU Bison b4_version.  */

/* $1,
   Copyright (C) $2 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor,
   Boston, MA 02110-1301, USA.  */])


# b4_identification
# -----------------
m4_define([b4_identification],
[/* Identify Bison output.  */
[#]define YYBISON 1

/* Bison version.  */
[#]define YYBISON_VERSION "b4_version"

/* Skeleton name.  */
[#]define YYSKELETON_NAME b4_skeleton

/* Pure parsers.  */
[#]define YYPURE b4_pure

/* Using locations.  */
[#]define YYLSP_NEEDED b4_locations_flag
])



## ---------------- ##
## Default values.  ##
## ---------------- ##

m4_define_default([b4_epilogue], [])



## ------------------------ ##
## Pure/impure interfaces.  ##
## ------------------------ ##


# b4_user_args
# ------------
m4_define([b4_user_args],
[m4_ifset([b4_parse_param], [, b4_c_args(b4_parse_param)])])


# b4_parse_param
# --------------
# If defined, b4_parse_param arrives double quoted, but below we prefer
# it to be single quoted.
m4_define_default([b4_parse_param])
m4_define([b4_parse_param],
b4_parse_param))



## ------------ ##
## Data Types.  ##
## ------------ ##


# b4_ints_in(INT1, INT2, LOW, HIGH)
# ---------------------------------
# Return 1 iff both INT1 and INT2 are in [LOW, HIGH], 0 otherwise.
m4_define([b4_ints_in],
[m4_eval([$3 <= $1 && $1 <= $4 && $3 <= $2 && $2 <= $4])])


# b4_int_type(MIN, MAX)
# ---------------------
# Return the smallest int type able to handle numbers ranging from
# MIN to MAX (included).
m4_define([b4_int_type],
[m4_if(b4_ints_in($@,      [0],   [255]), [1], [unsigned char],
       b4_ints_in($@,   [-128],   [127]), [1], [signed char],

       b4_ints_in($@,      [0], [65535]), [1], [unsigned short int],
       b4_ints_in($@, [-32768], [32767]), [1], [short int],

       m4_eval([0 <= $1]),                [1], [unsigned int],

	                                       [int])])


# b4_int_type_for(NAME)
# ---------------------
# Return the smallest int type able to handle numbers ranging from
# `NAME_min' to `NAME_max' (included).
m4_define([b4_int_type_for],
[b4_int_type($1_min, $1_max)])


## ------------------ ##
## Decoding options.  ##
## ------------------ ##


# b4_location_if(IF-TRUE, IF-FALSE)
# ---------------------------------
# Expand IF-TRUE, if locations are used, IF-FALSE otherwise.
m4_define([b4_location_if],
[m4_if(b4_locations_flag, [1],
       [$1],
       [$2])])


# b4_pure_if(IF-TRUE, IF-FALSE)
# -----------------------------
# Expand IF-TRUE, if %pure-parser, IF-FALSE otherwise.
m4_define([b4_pure_if],
[m4_if(b4_pure, [1],
       [$1],
       [$2])])



## ------------------------- ##
## Assigning token numbers.  ##
## ------------------------- ##

# b4_token_define(TOKEN-NAME, TOKEN-NUMBER)
# -----------------------------------------
# Output the definition of this token as #define.
m4_define([b4_token_define],
[#define $1 $2
])


# b4_token_defines(LIST-OF-PAIRS-TOKEN-NAME-TOKEN-NUMBER)
# -------------------------------------------------------
# Output the definition of the tokens (if there are) as #defines.
m4_define([b4_token_defines],
[m4_if([$@], [[]], [],
[/* Tokens.  */
m4_map([b4_token_define], [$@])])
])


# b4_token_enum(TOKEN-NAME, TOKEN-NUMBER)
# ---------------------------------------
# Output the definition of this token as an enum.
m4_define([b4_token_enum],
[$1 = $2])


# b4_token_enums(LIST-OF-PAIRS-TOKEN-NAME-TOKEN-NUMBER)
# -----------------------------------------------------
# Output the definition of the tokens (if there are) as enums.
m4_define([b4_token_enums],
[m4_if([$@], [[]], [],
[/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
m4_map_sep([     b4_token_enum], [,
],
           [$@])
   };
#endif
])])


# b4_token_enums_defines(LIST-OF-PAIRS-TOKEN-NAME-TOKEN-NUMBER)
# -------------------------------------------------------------
# Output the definition of the tokens (if there are) as enums and #defines.
m4_define([b4_token_enums_defines],
[b4_token_enums($@)b4_token_defines($@)
])



## --------------------------------------------- ##
## Defining C functions in both K&R and ANSI-C.  ##
## --------------------------------------------- ##


# b4_c_function_def(NAME, RETURN-VALUE, [DECL1, NAME1], ...)
# ----------------------------------------------------------
# Declare the function NAME.
m4_define([b4_c_function_def],
[#if defined (__STDC__) || defined (__cplusplus)
b4_c_ansi_function_def($@)
#else
$2
$1 (b4_c_knr_formal_names(m4_shiftn(2, $@)))
b4_c_knr_formal_decls(m4_shiftn(2, $@))
#endif[]dnl
])


# b4_c_ansi_function_def(NAME, RETURN-VALUE, [DECL1, NAME1], ...)
# ---------------------------------------------------------------
# Declare the function NAME in ANSI.
m4_define([b4_c_ansi_function_def],
[$2
$1 (b4_c_ansi_formals(m4_shiftn(2, $@)))[]dnl
])


# b4_c_ansi_formals([DECL1, NAME1], ...)
# --------------------------------------
# Output the arguments ANSI-C definition.
m4_define([b4_c_ansi_formals],
[m4_case([$@],
         [],   [void],
         [[]], [void],
               [m4_map_sep([b4_c_ansi_formal], [, ], [$@])])])

m4_define([b4_c_ansi_formal],
[$1])


# b4_c_knr_formal_names([DECL1, NAME1], ...)
# ------------------------------------------
# Output the argument names.
m4_define([b4_c_knr_formal_names],
[m4_map_sep([b4_c_knr_formal_name], [, ], [$@])])

m4_define([b4_c_knr_formal_name],
[$2])


# b4_c_knr_formal_decls([DECL1, NAME1], ...)
# ------------------------------------------
# Output the K&R argument declarations.
m4_define([b4_c_knr_formal_decls],
[m4_map_sep([b4_c_knr_formal_decl],
            [
],
            [$@])])

m4_define([b4_c_knr_formal_decl],
[    $1;])



## ------------------------------------------------------------ ##
## Declaring (prototyping) C functions in both K&R and ANSI-C.  ##
## ------------------------------------------------------------ ##


# b4_c_function_decl(NAME, RETURN-VALUE, [DECL1, NAME1], ...)
# -----------------------------------------------------------
# Declare the function NAME.
m4_define([b4_c_function_decl],
[#if defined (__STDC__) || defined (__cplusplus)
b4_c_ansi_function_decl($@)
#else
$2 $1 ();
#endif[]dnl
])


# b4_c_ansi_function_decl(NAME, RETURN-VALUE, [DECL1, NAME1], ...)
# ----------------------------------------------------------------
# Declare the function NAME.
m4_define([b4_c_ansi_function_decl],
[$2 $1 (b4_c_ansi_formals(m4_shiftn(2, $@)));[]dnl
])




## --------------------- ##
## Calling C functions.  ##
## --------------------- ##


# b4_c_function_call(NAME, RETURN-VALUE, [DECL1, NAME1], ...)
# -----------------------------------------------------------
# Call the function NAME with arguments NAME1, NAME2 etc.
m4_define([b4_c_function_call],
[$1 (b4_c_args(m4_shiftn(2, $@)))[]dnl
])


# b4_c_args([DECL1, NAME1], ...)
# ------------------------------
# Output the arguments NAME1, NAME2...
m4_define([b4_c_args],
[m4_map_sep([b4_c_arg], [, ], [$@])])

m4_define([b4_c_arg],
[$2])


## ----------- ##
## Synclines.  ##
## ----------- ##


# b4_syncline(LINE, FILE)
# -----------------------
m4_define([b4_syncline],
[m4_if(b4_synclines_flag, 1,
       [[#]line $1 $2])])


# b4_symbol_actions(FILENAME, LINENO,
#                   SYMBOL-TAG, SYMBOL-NUM,
#                   SYMBOL-ACTION, SYMBOL-TYPENAME)
# -------------------------------------------------
m4_define([b4_symbol_actions],
[m4_pushdef([b4_dollar_dollar],
   [m4_ifval([$6], [(yyvaluep->$6)], [(*yyvaluep)])])dnl
m4_pushdef([b4_at_dollar], [(*yylocationp)])dnl
      case $4: /* $3 */
b4_syncline([$2], [$1])
        $5;
b4_syncline([@oline@], [@ofile@])
        break;
m4_popdef([b4_at_dollar])dnl
m4_popdef([b4_dollar_dollar])dnl
])


# b4_yydestruct_generate(FUNCTION-DECLARATOR)
# -------------------------------------------
# Generate the "yydestruct" function, which declaration is issued using
# FUNCTION-DECLARATOR, which may be "b4_c_ansi_function_def" for ISO C
# or "b4_c_function_def" for K&R.
m4_define_default([b4_yydestruct_generate],
[[/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

]$1([yydestruct],
    [static void],
    [[const char *yymsg],    [yymsg]],
    [[int yytype],           [yytype]],
    [[YYSTYPE *yyvaluep],    [yyvaluep]]b4_location_if([,
    [[YYLTYPE *yylocationp], [yylocationp]]]))[
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;
]b4_location_if([  (void) yylocationp;
])[
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yytype, yyvaluep, yylocationp);

  switch (yytype)
    {
]m4_map([b4_symbol_actions], m4_defn([b4_symbol_destructors]))[
      default:
        break;
    }
}]dnl
])


# b4_yysymprint_generate(FUNCTION-DECLARATOR)
# -------------------------------------------
# Generate the "yysymprint" function, which declaration is issued using
# FUNCTION-DECLARATOR, which may be "b4_c_ansi_function_def" for ISO C
# or "b4_c_function_def" for K&R.
m4_define_default([b4_yysymprint_generate],
[[/*--------------------------------.
| Print this symbol on YYOUTPUT.  |
`--------------------------------*/

]$1([yysymprint],
    [static void],
    [[FILE *yyoutput],       [yyoutput]],
    [[int yytype],           [yytype]],
    [[YYSTYPE *yyvaluep],    [yyvaluep]]b4_location_if([,
    [[YYLTYPE *yylocationp], [yylocationp]]]))
{
  /* Pacify ``unused variable'' warnings.  */
  (void) yyvaluep;
b4_location_if([  (void) yylocationp;
])dnl
[
  if (yytype < YYNTOKENS)
    YYFPRINTF (yyoutput, "token %s (", yytname[yytype]);
  else
    YYFPRINTF (yyoutput, "nterm %s (", yytname[yytype]);

]b4_location_if([  YY_LOCATION_PRINT (yyoutput, *yylocationp);
  YYFPRINTF (yyoutput, ": ");
])dnl
[
# ifdef YYPRINT
  if (yytype < YYNTOKENS)
    YYPRINT (yyoutput, yytoknum[yytype], *yyvaluep);
# endif
  switch (yytype)
    {
]m4_map([b4_symbol_actions], m4_defn([b4_symbol_printers]))dnl
[      default:
        break;
    }
  YYFPRINTF (yyoutput, ")");
}
]])
