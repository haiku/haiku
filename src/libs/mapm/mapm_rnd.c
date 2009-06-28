
/* 
 *  M_APM  -  mapm_rnd.c
 *
 *  Copyright (C) 1999 - 2007   Michael C. Ring
 *
 *  Permission to use, copy, and distribute this software and its
 *  documentation for any purpose with or without fee is hereby granted,
 *  provided that the above copyright notice appear in all copies and
 *  that both that copyright notice and this permission notice appear
 *  in supporting documentation.
 *
 *  Permission to modify the software is granted. Permission to distribute
 *  the modified code is granted. Modifications are to be distributed by
 *  using the file 'license.txt' as a template to modify the file header.
 *  'license.txt' is available in the official MAPM distribution.
 *
 *  This software is provided "as is" without express or implied warranty.
 */

/*
 *      $Id: mapm_rnd.c,v 1.12 2007/12/03 01:47:17 mike Exp $
 *
 *      This file contains the Random Number Generator function.
 *
 *      $Log: mapm_rnd.c,v $
 *      Revision 1.12  2007/12/03 01:47:17  mike
 *      Update license
 *
 *      Revision 1.11  2003/10/25 22:55:43  mike
 *      add support for National Instruments LabWindows CVI
 *
 *      Revision 1.10  2002/11/03 22:41:03  mike
 *      Updated function parameters to use the modern style
 *
 *      Revision 1.9  2002/02/14 21:50:45  mike
 *      add _set_random_seed
 *
 *      Revision 1.8  2001/07/16 19:30:32  mike
 *      add function M_free_all_rnd
 *
 *      Revision 1.7  2001/03/20 17:19:45  mike
 *      use a new multiplier
 *
 *      Revision 1.6  2000/08/20 23:46:07  mike
 *      add more possible multupliers (no code changes)
 *
 *      Revision 1.5  1999/09/19 23:32:14  mike
 *      added comments
 *
 *      Revision 1.4  1999/09/18 03:49:25  mike
 *      *** empty log message ***
 *
 *      Revision 1.3  1999/09/18 03:35:36  mike
 *      only prototype get_microsec for non-DOS
 *
 *      Revision 1.2  1999/09/18 02:35:36  mike
 *      delete debug printf's
 *
 *      Revision 1.1  1999/09/18 02:26:52  mike
 *      Initial revision
 */

#include "m_apm_lc.h"

#ifndef _HAVE_NI_LABWIN_CVI_
#ifdef MSDOS
#include <time.h>
#include <sys/timeb.h>
#else
#include <sys/time.h>
extern  void	M_get_microsec(unsigned long *, long *);
#endif
#endif

#ifdef _HAVE_NI_LABWIN_CVI_
#include <time.h>
#include <utility.h>
#include <ansi/math.h>
#endif

extern  void	M_reverse_string(char *);
extern  void    M_get_rnd_seed(M_APM);

static	M_APM   M_rnd_aa;
static  M_APM   M_rnd_mm;
static  M_APM   M_rnd_XX;
static  M_APM   M_rtmp0;
static  M_APM   M_rtmp1;

static  int     M_firsttime2 = TRUE;

/*
        Used Knuth's The Art of Computer Programming, Volume 2 as
        the basis. Assuming the random number is X, compute
	(where all the math is performed on integers) :

	X = (a * X + c) MOD m

	From Knuth:

	'm' should be large, at least 2^30 : we use 1.0E+15

	'a' should be between .01m and .99m and not have a simple
	pattern. 'a' should not have any large factors in common 
	with 'm' and (since 'm' is a power of 10) if 'a' MOD 200 
	= 21 then all 'm' different possible values will be 
	generated before 'X' starts to repeat.

	We use 'a' = 716805947629621.

	This is a prime number and also meets 'a' MOD 200 = 21. 
	Commented out below are many potential multipliers that 
	are all prime and meet 'a' MOD 200 = 21.

	There are few restrictions on 'c' except 'c' can have no
	factor in common with 'm', hence we set 'c' = 'a'.

	On the first call, the system time is used to initialize X.
*/

/*
 *  the following constants are all potential multipliers. they are
 *  all prime numbers that also meet the criteria of NUM mod 200 = 21.
 */

/*
439682071525421   439682071528421   439682071529221   439682071529821
439682071530421   439682071532021   439682071538821   439682071539421
439682071540021   439682071547021   439682071551221   439682071553821
439682071555421   439682071557221   439682071558021   439682071558621
439682071559821   439652381461621   439652381465221   439652381465621
439652381466421   439652381467421   439652381468621   439652381470021
439652381471221   439652381477021   439652381484221   439652381488421
439652381491021   439652381492021   439652381494021   439652381496821
617294387035621   617294387038621   617294387039221   617294387044421
617294387045221   617294387048621   617294387051621   617294387051821
617294387053621   617294387058421   617294387064221   617294387065621
617294387068621   617294387069221   617294387069821   617294387070421
617294387072021   617294387072621   617294387073821   617294387076821
649378126517621   649378126517821   649378126518221   649378126520821
649378126523821   649378126525621   649378126526621   649378126528421
649378126529621   649378126530821   649378126532221   649378126533221
649378126535221   649378126539421   649378126543621   649378126546021
649378126546421   649378126549421   649378126550821   649378126555021
649378126557421   649378126560221   649378126561621   649378126562021
649378126564621   649378126565821   672091582360421   672091582364221
672091582364621   672091582367021   672091582368421   672091582369021
672091582370821   672091582371421   672091582376821   672091582380821
716805243983221   716805243984821   716805947623621   716805947624621
716805947629021   716805947629621   716805947630621   716805947633621
716805947634221   716805947635021   716805947635621   716805947642221
*/

/****************************************************************************/
void	M_free_all_rnd()
{
if (M_firsttime2 == FALSE)
  {
   m_apm_free(M_rnd_aa);
   m_apm_free(M_rnd_mm);
   m_apm_free(M_rnd_XX);
   m_apm_free(M_rtmp0);
   m_apm_free(M_rtmp1);

   M_firsttime2 = TRUE;
  }
}
/****************************************************************************/
void	m_apm_set_random_seed(char *ss)
{
M_APM   btmp;

if (M_firsttime2)
  {
   btmp = M_get_stack_var();
   m_apm_get_random(btmp);
   M_restore_stack(1);
  }

m_apm_set_string(M_rnd_XX, ss);
}
/****************************************************************************/
/*
 *  compute X = (a * X + c) MOD m       where c = a
 */
void	m_apm_get_random(M_APM mrnd)
{

if (M_firsttime2)         /* use the system time as the initial seed value */
  {
   M_firsttime2 = FALSE;
   
   M_rnd_aa = m_apm_init();
   M_rnd_XX = m_apm_init();
   M_rnd_mm = m_apm_init();
   M_rtmp0  = m_apm_init();
   M_rtmp1  = m_apm_init();

   /* set the multiplier M_rnd_aa and M_rnd_mm */
   
   m_apm_set_string(M_rnd_aa, "716805947629621");
   m_apm_set_string(M_rnd_mm, "1.0E15");

   M_get_rnd_seed(M_rnd_XX);
  }

m_apm_multiply(M_rtmp0, M_rnd_XX, M_rnd_aa);
m_apm_add(M_rtmp1, M_rtmp0, M_rnd_aa);
m_apm_integer_div_rem(M_rtmp0, M_rnd_XX, M_rtmp1, M_rnd_mm);
m_apm_copy(mrnd, M_rnd_XX);
mrnd->m_apm_exponent -= 15;
}
/****************************************************************************/
void	M_reverse_string(char *s)
{
int	ct;
char	ch, *p1, *p2;

if ((ct = strlen(s)) <= 1)
  return;

p1 = s;
p2 = s + ct - 1;
ct /= 2;

while (TRUE)
  {
   ch    = *p1;
   *p1++ = *p2;
   *p2-- = ch;

   if (--ct == 0)
     break;
  }
}
/****************************************************************************/

#ifndef _HAVE_NI_LABWIN_CVI_

#ifdef MSDOS

/****************************************************************************/
/*
 *  for DOS / Win 9x/NT systems : use 'ftime' 
 */
void	M_get_rnd_seed(M_APM mm)
{
int              millisec;
time_t 		 timestamp;
unsigned long    ul;
char             ss[32], buf1[48], buf2[32];
struct timeb     timebuffer;
M_APM		 atmp;

atmp = M_get_stack_var();

ftime(&timebuffer);

millisec  = (int)timebuffer.millitm;    
timestamp = timebuffer.time;
ul        = (unsigned long)(timestamp / 7);
ul       += timestamp + 537;
strcpy(ss,ctime(&timestamp));        /* convert to string and copy to ss */

sprintf(buf1,"%d",(millisec / 10));
sprintf(buf2,"%lu",ul);

ss[0] = ss[18];
ss[1] = ss[17];
ss[2] = ss[15];
ss[3] = ss[14];
ss[4] = ss[12];
ss[5] = ss[11];
ss[6] = ss[9];
ss[7] = ss[23];
ss[8] = ss[20];
ss[9] = '\0';

M_reverse_string(buf2);
strcat(buf1,buf2);
strcat(buf1,ss);

m_apm_set_string(atmp, buf1);
atmp->m_apm_exponent = 15;
m_apm_integer_divide(mm, atmp, MM_One);

M_restore_stack(1);
}
/****************************************************************************/

#else

/****************************************************************************/
/*
 *  for unix systems : use 'gettimeofday'
 */
void	M_get_rnd_seed(M_APM mm)
{
unsigned long    sec3;
long             usec3;
char             buf1[32], buf2[32];
M_APM		 atmp;

atmp = M_get_stack_var();
M_get_microsec(&sec3,&usec3);

sprintf(buf1,"%ld",usec3);
sprintf(buf2,"%lu",sec3);
M_reverse_string(buf2);
strcat(buf1,buf2);

m_apm_set_string(atmp, buf1);
atmp->m_apm_exponent = 15;
m_apm_integer_divide(mm, atmp, MM_One);

M_restore_stack(1);
}
/****************************************************************************/
void	 M_get_microsec(unsigned long *sec, long *usec)
{
struct timeval time_now;           /* current time for elapsed time check */
struct timezone time_zone;         /* time zone for gettimeofday call     */

gettimeofday(&time_now, &time_zone);                  /* get current time */

*sec  = time_now.tv_sec;
*usec = time_now.tv_usec;
}
/****************************************************************************/

#endif
#endif

#ifdef _HAVE_NI_LABWIN_CVI_

/****************************************************************************/
/*
 *  for National Instruments LabWindows CVI
 */

void	M_get_rnd_seed(M_APM mm)
{
double		 timer0;
int		 millisec;
char             *cvi_time, *cvi_date, buf1[64], buf2[32];
M_APM		 atmp;

atmp = M_get_stack_var();

cvi_date = DateStr();
cvi_time = TimeStr();
timer0   = Timer();

/*
 *  note that Timer() is not syncronized to TimeStr(),
 *  but we don't care here since we are just looking
 *  for a random source of digits.
 */

millisec = (int)(0.01 + 1000.0 * (timer0 - floor(timer0)));

sprintf(buf1, "%d", millisec);

buf2[0]  = cvi_time[6];	/* time format: "HH:MM:SS" */
buf2[1]  = cvi_time[7];
buf2[2]  = cvi_time[3];
buf2[3]  = cvi_time[4];
buf2[4]  = cvi_time[0];
buf2[5]  = cvi_time[1];

buf2[6]  = cvi_date[3];	/* date format: "MM-DD-YYYY" */
buf2[7]  = cvi_date[4];
buf2[8]  = cvi_date[0];
buf2[9]  = cvi_date[1];
buf2[10] = cvi_date[8];
buf2[11] = cvi_date[9];
buf2[12] = cvi_date[7];

buf2[13] = '4';
buf2[14] = '7';
buf2[15] = '\0';

strcat(buf1, buf2);

m_apm_set_string(atmp, buf1);
atmp->m_apm_exponent = 15;
m_apm_integer_divide(mm, atmp, MM_One);

M_restore_stack(1);
}

#endif

/****************************************************************************/

