/*
 * $Id: edit_field.h,v 1.3 2005/09/24 22:59:52 tom Exp $
 *
 * Interface of edit_field.c
 */

#ifndef EDIT_FORM_H_incl
#define EDIT_FORM_H_incl 1

#include <form.h>

#define EDIT_FIELD(c) (MAX_FORM_COMMAND + c)

#define MY_HELP		EDIT_FIELD('h')
#define MY_QUIT		EDIT_FIELD('q')
#define MY_EDT_MODE	EDIT_FIELD('e')
#define MY_INS_MODE	EDIT_FIELD('t')

extern void help_edit_field(void);
extern int edit_field(FORM * form, int *result);

#endif /* EDIT_FORM_H_incl */
