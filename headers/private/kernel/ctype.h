/* 
** Copyright 2001, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#ifndef _CTYPE_H
#define _CTYPE_H

#ifdef __cplusplus
extern "C"
{
#endif

int isalnum(int c);
int isalpha(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int isascii(int c);
int toascii(int c);

unsigned char tolower(unsigned char c);
unsigned char toupper(unsigned char c);

#ifdef __cplusplus
} /* "C" */
#endif

#endif

