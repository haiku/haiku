/* ANSI-C code produced by gperf version 2.7.2 */
/* Command-line: gperf -acCgopt -k'1,2,5,9,$' -L ANSI-C -N locfile_hash programs/locfile-kw.gperf  */
/* Copyright (C) 1996, 1997, 1998, 1999, 2000 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@gnu.org>, 1996.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#include <string.h>

#include "locfile-token.h"
struct keyword_t ;

#define TOTAL_KEYWORDS 175
#define MIN_WORD_LENGTH 3
#define MAX_WORD_LENGTH 22
#define MIN_HASH_VALUE 3
#define MAX_HASH_VALUE 687
/* maximum key range = 685, duplicates = 0 */

#ifdef __GNUC__
__inline
#else
#ifdef __cplusplus
inline
#endif
#endif
static unsigned int
hash (register const char *str, register unsigned int len)
{
  static const unsigned short asso_values[] =
    {
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
        5,   0, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688,  15, 688,   0,   0,   0,
        5,   0,   0,   0, 688, 688,   0, 688,   0,   5,
      688, 688,  15,   0,   5,  15, 688, 688, 688,   0,
      688, 688, 688, 688, 688,  75, 688,   0,   0,  65,
        5,   0,  85,  40,   5, 155, 688,  10, 105, 120,
      125,  35,   5,  20,   5, 190,   0, 125,  35,  10,
       30,  35,   0, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688, 688, 688, 688, 688,
      688, 688, 688, 688, 688, 688
    };
  register int hval = len;

  switch (hval)
    {
      default:
      case 9:
        hval += asso_values[(unsigned char)str[8]];
      case 8:
      case 7:
      case 6:
      case 5:
        hval += asso_values[(unsigned char)str[4]];
      case 4:
      case 3:
      case 2:
        hval += asso_values[(unsigned char)str[1]];
      case 1:
        hval += asso_values[(unsigned char)str[0]];
        break;
    }
  return hval + asso_values[(unsigned char)str[len - 1]];
}

#ifdef __GNUC__
__inline
#endif
const struct keyword_t *
locfile_hash (register const char *str, register unsigned int len)
{
  static const struct keyword_t wordlist[] =
    {
      {""}, {""}, {""},
      {"END",                    tok_end,                    0},
      {""}, {""}, {""},
      {"LC_TIME",                tok_lc_time,                0},
      {"era",                    tok_era,                    0},
      {"date",                   tok_date,                   0},
      {"LC_ADDRESS",             tok_lc_address,             0},
      {"LC_MESSAGES",            tok_lc_messages,            0},
      {"LC_TELEPHONE",           tok_lc_telephone,           0},
      {"LC_CTYPE",               tok_lc_ctype,               0},
      {"era_t_fmt",              tok_era_t_fmt,              0},
      {"print",                  tok_print,                  0},
      {"height",                 tok_height,                 0},
      {"LC_IDENTIFICATION",      tok_lc_identification,      0},
      {""},
      {"era_d_fmt",              tok_era_d_fmt,              0},
      {"LC_COLLATE",             tok_lc_collate,             0},
      {"IGNORE",                 tok_ignore,                 0},
      {"LC_NAME",                tok_lc_name,                0},
      {"backward",               tok_backward,               0},
      {"week",                   tok_week,                   0},
      {"LC_NUMERIC",             tok_lc_numeric,             0},
      {"reorder-end",            tok_reorder_end,            0},
      {""},
      {"reorder-after",          tok_reorder_after,          0},
      {"UNDEFINED",              tok_undefined,              0},
      {""},
      {"LC_MONETARY",            tok_lc_monetary,            0},
      {""},
      {"repertoiremap",          tok_repertoiremap,          0},
      {"LC_MEASUREMENT",         tok_lc_measurement,         0},
      {""}, {""}, {""},
      {"LC_PAPER",               tok_lc_paper,               0},
      {""}, {""}, {""}, {""},
      {"day",                    tok_day,                    0},
      {""}, {""},
      {"yesstr",                 tok_yesstr,                 0},
      {""}, {""}, {""}, {""}, {""},
      {"toupper",                tok_toupper,                0},
      {"era_year",               tok_era_year,               0},
      {""}, {""},
      {"order_start",            tok_order_start,            0},
      {"tolower",                tok_tolower,                0},
      {""}, {""},
      {"graph",                  tok_graph,                  0},
      {""}, {""}, {""},
      {"order_end",              tok_order_end,              0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
      {"abday",                  tok_abday,                  0},
      {""},
      {"yesexpr",                tok_yesexpr,                0},
      {""}, {""},
      {"t_fmt",                  tok_t_fmt,                  0},
      {""}, {""}, {""}, {""},
      {"d_fmt",                  tok_d_fmt,                  0},
      {""}, {""},
      {"date_fmt",               tok_date_fmt,               0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"grouping",               tok_grouping,               0},
      {""}, {""},
      {"tel_dom_fmt",            tok_tel_dom_fmt,            0},
      {""}, {""}, {""}, {""},
      {"era_d_t_fmt",            tok_era_d_t_fmt,            0},
      {"contact",                tok_contact,                0},
      {"tel",                    tok_tel,                    0},
      {"else",                   tok_else,                   0},
      {"alpha",                  tok_alpha,                  0},
      {"country_ab3",            tok_country_ab3,            0},
      {""}, {""}, {""}, {""},
      {"country_ab2",            tok_country_ab2,            0},
      {"country_post",           tok_country_post,           0},
      {"fax",                    tok_fax,                    0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"map",                    tok_map,                    0},
      {""},
      {"blank",                  tok_blank,                  0},
      {""},
      {"forward",                tok_forward,                0},
      {"audience",               tok_audience,               0},
      {""},
      {"punct",                  tok_punct,                  0},
      {"define",                 tok_define,                 0},
      {"abbreviation",           tok_abbreviation,           0},
      {""},
      {"copy",                   tok_copy,                   0},
      {""}, {""}, {""},
      {"decimal_point",          tok_decimal_point,          0},
      {""},
      {"upper",                  tok_upper,                  0},
      {""}, {""},
      {"category",               tok_category,               0},
      {""},
      {"conversion_rate",        tok_conversion_rate,        0},
      {""}, {""}, {""}, {""},
      {"lower",                  tok_lower,                  0},
      {""},
      {"collating-element",      tok_collating_element,      0},
      {"duo_p_sep_by_space",     tok_duo_p_sep_by_space,     0},
      {""},
      {"title",                  tok_title,                  0},
      {""}, {""},
      {"timezone",               tok_timezone,               0},
      {""},
      {"digit",                  tok_digit,                  0},
      {""}, {""}, {""}, {""},
      {"postal_fmt",             tok_postal_fmt,             0},
      {""},
      {"d_t_fmt",                tok_d_t_fmt,                0},
      {"position",               tok_position,               0},
      {"p_sep_by_space",         tok_p_sep_by_space,         0},
      {"nostr",                  tok_nostr,                  0},
      {"noexpr",                 tok_noexpr,                 0},
      {""},
      {"charconv",               tok_charconv,               0},
      {""},
      {"width",                  tok_width,                  0},
      {"country_car",            tok_country_car,            0},
      {"comment_char",           tok_comment_char,           0},
      {""}, {""}, {""}, {""},
      {"lang_ab",                tok_lang_ab,                0},
      {"lang_lib",               tok_lang_lib,               0},
      {"lang_name",              tok_lang_name,              0},
      {""}, {""}, {""}, {""},
      {"elif",                   tok_elif,                   0},
      {""},
      {"xdigit",                 tok_xdigit,                 0},
      {""}, {""}, {""},
      {"space",                  tok_space,                  0},
      {""},
      {"address",                tok_address,                0},
      {""}, {""}, {""}, {""}, {""},
      {"name_fmt",               tok_name_fmt,               0},
      {""},
      {"t_fmt_ampm",             tok_t_fmt_ampm,             0},
      {""},
      {"name_mr",                tok_name_mr,                0},
      {""},
      {"from",                   tok_from,                   0},
      {""},
      {"escape_char",            tok_escape_char,            0},
      {"duo_valid_to",           tok_duo_valid_to,           0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"reorder-sections-end",   tok_reorder_sections_end,   0},
      {""},
      {"reorder-sections-after", tok_reorder_sections_after, 0},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"territory",              tok_territory,              0},
      {""}, {""},
      {"country_name",           tok_country_name,           0},
      {"language",               tok_language,               0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
      {"tel_int_fmt",            tok_tel_int_fmt,            0},
      {"mon_grouping",           tok_mon_grouping,           0},
      {"positive_sign",          tok_positive_sign,          0},
      {""},
      {"abmon",                  tok_abmon,                  0},
      {"measurement",            tok_measurement,            0},
      {""}, {""}, {""},
      {"coll_weight_max",        tok_coll_weight_max,        0},
      {"collating-symbol",       tok_collating_symbol,       0},
      {""}, {""}, {""}, {""},
      {"script",                 tok_script,                 0},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"cal_direction",          tok_cal_direction,          0},
      {""}, {""}, {""}, {""},
      {"duo_n_sep_by_space",     tok_duo_n_sep_by_space,     0},
      {""}, {""}, {""}, {""},
      {"mon",                    tok_mon,                    0},
      {"translit_start",         tok_translit_start,         0},
      {"translit_ignore",        tok_translit_ignore,        0},
      {""},
      {"translit_end",           tok_translit_end,           0},
      {"first_weekday",          tok_first_weekday,          0},
      {""}, {""},
      {"p_sign_posn",            tok_p_sign_posn,            0},
      {""},
      {"first_workday",          tok_first_workday,          0},
      {"n_sep_by_space",         tok_n_sep_by_space,         0},
      {""},
      {"source",                 tok_source,                 0},
      {"mon_decimal_point",      tok_mon_decimal_point,      0},
      {"symbol-equivalence",     tok_symbol_equivalence,     0},
      {""},
      {"endif",                  tok_endif,                  0},
      {""}, {""}, {""},
      {"duo_valid_from",         tok_duo_valid_from,         0},
      {"default_missing",        tok_default_missing,        0},
      {""}, {""},
      {"int_p_sep_by_space",     tok_int_p_sep_by_space,     0},
      {""},
      {"alt_digits",             tok_alt_digits,             0},
      {""},
      {"duo_int_p_sep_by_space", tok_duo_int_p_sep_by_space, 0},
      {""}, {""},
      {"duo_p_sign_posn",        tok_duo_p_sign_posn,        0},
      {""}, {""}, {""},
      {"duo_currency_symbol",    tok_duo_currency_symbol,    0},
      {""}, {""}, {""},
      {"outdigit",               tok_outdigit,               0},
      {""}, {""}, {""}, {""},
      {"revision",               tok_revision,               0},
      {""}, {""}, {""}, {""},
      {"name_gen",               tok_name_gen,               0},
      {""},
      {"email",                  tok_email,                  0},
      {""},
      {"uno_valid_to",           tok_uno_valid_to,           0},
      {"negative_sign",          tok_negative_sign,          0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
      {"alnum",                  tok_alnum,                  0},
      {""}, {""}, {""}, {""}, {""},
      {"country_num",            tok_country_num,            0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"am_pm",                  tok_am_pm,                  0},
      {""},
      {"mon_thousands_sep",      tok_mon_thousands_sep,      0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"currency_symbol",        tok_currency_symbol,        0},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"country_isbn",           tok_country_isbn,           0},
      {""}, {""}, {""}, {""},
      {"name_ms",                tok_name_ms,                0},
      {"name_mrs",               tok_name_mrs,               0},
      {""}, {""}, {""}, {""},
      {"thousands_sep",          tok_thousands_sep,          0},
      {""},
      {"cntrl",                  tok_cntrl,                  0},
      {""}, {""}, {""}, {""}, {""},
      {"n_sign_posn",            tok_n_sign_posn,            0},
      {"include",                tok_include,                0},
      {""}, {""},
      {"ifdef",                  tok_ifdef,                  0},
      {""},
      {"duo_p_cs_precedes",      tok_duo_p_cs_precedes,      0},
      {""}, {""}, {""}, {""}, {""},
      {"p_cs_precedes",          tok_p_cs_precedes,          0},
      {"uno_valid_from",         tok_uno_valid_from,         0},
      {"undef",                  tok_undef,                  0},
      {""}, {""},
      {"int_n_sep_by_space",     tok_int_n_sep_by_space,     0},
      {"lang_term",              tok_lang_term,              0},
      {""}, {""},
      {"duo_int_n_sep_by_space", tok_duo_int_n_sep_by_space, 0},
      {""},
      {"duo_int_p_sign_posn",    tok_duo_int_p_sign_posn,    0},
      {"duo_n_sign_posn",        tok_duo_n_sign_posn,        0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""},
      {"application",            tok_application,            0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
      {"int_p_sign_posn",        tok_int_p_sign_posn,        0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"duo_int_curr_symbol",    tok_duo_int_curr_symbol,    0},
      {""}, {""}, {""}, {""}, {""},
      {"int_prefix",             tok_int_prefix,             0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"duo_frac_digits",        tok_duo_frac_digits,        0},
      {""}, {""}, {""}, {""}, {""},
      {"duo_int_p_cs_precedes",  tok_duo_int_p_cs_precedes,  0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""},
      {"frac_digits",            tok_frac_digits,            0},
      {""}, {""},
      {"charclass",              tok_charclass,              0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""},
      {"duo_n_cs_precedes",      tok_duo_n_cs_precedes,      0},
      {""}, {""},
      {"int_curr_symbol",        tok_int_curr_symbol,        0},
      {""}, {""},
      {"n_cs_precedes",          tok_n_cs_precedes,          0},
      {""},
      {"int_select",             tok_int_select,             0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"duo_int_n_sign_posn",    tok_duo_int_n_sign_posn,    0},
      {"class",                  tok_class,                  0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
      {"int_p_cs_precedes",      tok_int_p_cs_precedes,      0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""},
      {"duo_int_frac_digits",    tok_duo_int_frac_digits,    0},
      {""}, {""}, {""}, {""}, {""},
      {"int_n_sign_posn",        tok_int_n_sign_posn,        0},
      {""}, {""}, {""},
      {"name_miss",              tok_name_miss,              0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""},
      {"duo_int_n_cs_precedes",  tok_duo_int_n_cs_precedes,  0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"int_frac_digits",        tok_int_frac_digits,        0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {"section-symbol",         tok_section_symbol,         0},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""}, {""},
      {""}, {""}, {""}, {""}, {""},
      {"int_n_cs_precedes",      tok_int_n_cs_precedes,      0}
    };

  if (len <= MAX_WORD_LENGTH && len >= MIN_WORD_LENGTH)
    {
      register int key = hash (str, len);

      if (key <= MAX_HASH_VALUE && key >= 0)
        {
          register const char *s = wordlist[key].name;

          if (*str == *s && !strncmp (str + 1, s + 1, len - 1) && s[len] == '\0')
            return &wordlist[key];
        }
    }
  return 0;
}
