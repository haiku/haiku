/*
** This file is in the public domain, so clarified as of
** 1996-06-05 by Arthur David Olson.
*/

#ifndef lint
#ifndef NOID
static char	elsieid[] = "@(#)localtime.c	8.5";
#endif /* !defined NOID */
#endif /* !defined lint */

/*
** Leap second handling from Bradley White.
** POSIX-style TZ environment variable handling from Guy Harris.
*/

/*LINTLIBRARY*/

#include "private.h"
#include "tzfile.h"
#include "fcntl.h"
#include "float.h"	/* for FLT_MAX and DBL_MAX */

#ifndef TZ_ABBR_MAX_LEN
#define TZ_ABBR_MAX_LEN	16
#endif /* !defined TZ_ABBR_MAX_LEN */

#ifndef TZ_ABBR_CHAR_SET
#define TZ_ABBR_CHAR_SET \
	"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 :+-._"
#endif /* !defined TZ_ABBR_CHAR_SET */

#ifndef TZ_ABBR_ERR_CHAR
#define TZ_ABBR_ERR_CHAR	'_'
#endif /* !defined TZ_ABBR_ERR_CHAR */

/*
** SunOS 4.1.1 headers lack O_BINARY.
*/

#ifdef O_BINARY
#define OPEN_MODE	(O_RDONLY | O_BINARY)
#endif /* defined O_BINARY */
#ifndef O_BINARY
#define OPEN_MODE	O_RDONLY
#endif /* !defined O_BINARY */

#ifndef WILDABBR
/*
** Someone might make incorrect use of a time zone abbreviation:
**	1.	They might reference tzname[0] before calling tzset (explicitly
**		or implicitly).
**	2.	They might reference tzname[1] before calling tzset (explicitly
**		or implicitly).
**	3.	They might reference tzname[1] after setting to a time zone
**		in which Daylight Saving Time is never observed.
**	4.	They might reference tzname[0] after setting to a time zone
**		in which Standard Time is never observed.
**	5.	They might reference tm.TM_ZONE after calling offtime.
** What's best to do in the above cases is open to debate;
** for now, we just set things up so that in any of the five cases
** WILDABBR is used. Another possibility: initialize tzname[0] to the
** string "tzname[0] used before set", and similarly for the other cases.
** And another: initialize tzname[0] to "ERA", with an explanation in the
** manual page of what this "time zone abbreviation" means (doing this so
** that tzname[0] has the "normal" length of three characters).
*/
#define WILDABBR	"   "
#endif /* !defined WILDABBR */

static char		wildabbr[] = WILDABBR;

static const char	gmt[] = "GMT";

/*
** The DST rules to use if TZ has no rules and we can't load TZDEFRULES.
** We default to US rules as of 1999-08-17.
** POSIX 1003.1 section 8.1.1 says that the default DST rules are
** implementation dependent; for historical reasons, US rules are a
** common default.
*/
#ifndef TZDEFRULESTRING
#define TZDEFRULESTRING ",M4.1.0,M10.5.0"
#endif /* !defined TZDEFDST */

struct ttinfo {				/* time type information */
	long		tt_gmtoff;	/* UTC offset in seconds */
	int		tt_isdst;	/* used to set tm_isdst */
	int		tt_abbrind;	/* abbreviation list index */
	int		tt_ttisstd;	/* TRUE if transition is std time */
	int		tt_ttisgmt;	/* TRUE if transition is UTC */
};

struct lsinfo {				/* leap second information */
	time_t		ls_trans;	/* transition time */
	long		ls_corr;	/* correction to apply */
};

#define BIGGEST(a, b)	(((a) > (b)) ? (a) : (b))

#ifdef TZNAME_MAX
#define MY_TZNAME_MAX	TZNAME_MAX
#endif /* defined TZNAME_MAX */
#ifndef TZNAME_MAX
#define MY_TZNAME_MAX	255
#endif /* !defined TZNAME_MAX */

struct state {
	int		leapcnt;
	int		timecnt;
	int		typecnt;
	int		charcnt;
	int		goback;
	int		goahead;
	time_t		ats[TZ_MAX_TIMES];
	unsigned char	types[TZ_MAX_TIMES];
	struct ttinfo	ttis[TZ_MAX_TYPES];
	char		chars[BIGGEST(BIGGEST(TZ_MAX_CHARS + 1, sizeof gmt),
				(2 * (MY_TZNAME_MAX + 1)))];
	struct lsinfo	lsis[TZ_MAX_LEAPS];
};

struct rule {
	int		r_type;		/* type of rule--see below */
	int		r_day;		/* day number of rule */
	int		r_week;		/* week number of rule */
	int		r_mon;		/* month number of rule */
	long		r_time;		/* transition time of rule */
};

#define JULIAN_DAY		0	/* Jn - Julian day */
#define DAY_OF_YEAR		1	/* n - day of year */
#define MONTH_NTH_DAY_OF_WEEK	2	/* Mm.n.d - month, week, day of week */

/*
** Prototypes for static functions.
*/

static struct tm *	gmtsub P((const time_t * timep, long offset,
				struct tm * tmp));
static struct tm *	localsub P((const time_t * timep, long offset,
				struct tm * tmp));
static int		increment_overflow P((int * number, int delta));
static int		leaps_thru_end_of P((int y));
static int		long_increment_overflow P((long * number, int delta));
static int		long_normalize_overflow P((long * tensptr,
				int * unitsptr, int base));
static int		normalize_overflow P((int * tensptr, int * unitsptr,
				int base));
static time_t		time1 P((struct tm * tmp,
				struct tm * (*funcp) P((const time_t *,
				long, struct tm *)),
				long offset));
static time_t		time2 P((struct tm *tmp,
				struct tm * (*funcp) P((const time_t *,
				long, struct tm*)),
				long offset, int * okayp));
static time_t		time2sub P((struct tm *tmp,
				struct tm * (*funcp) P((const time_t *,
				long, struct tm*)),
				long offset, int * okayp, int do_norm_secs));
static struct tm *	timesub P((const time_t * timep, long offset,
				const struct state * sp, struct tm * tmp));
static int		tmcomp P((const struct tm * atmp,
				const struct tm * btmp));

/*
** Other prototypes.
*/
struct tm* __gmtime_r_fallback(const time_t* timep, struct tm* tmp);
time_t __mktime_fallback(struct tm* tmp);
time_t timegm(struct tm* const tmp);

static struct state	lclmem;
static struct state	gmtmem;
#define lclptr		(&lclmem)
#define gmtptr		(&gmtmem)

static int		gmt_is_set;

static const int	mon_lengths[2][MONSPERYEAR] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }
};

static const int	year_lengths[2] = {
	DAYSPERNYEAR, DAYSPERLYEAR
};

/*
** The easy way to behave "as if no library function calls" localtime
** is to not call it--so we drop its guts into "localsub", which can be
** freely called. (And no, the PANS doesn't require the above behavior--
** but it *is* desirable.)
**
** The unused offset argument is for the benefit of mktime variants.
*/

/*ARGSUSED*/
static struct tm *
localsub(timep, offset, tmp)
const time_t * const	timep;
const long		offset;
struct tm * const	tmp;
{
	register struct state *		sp;
	register const struct ttinfo *	ttisp;
	register int			i;
	register struct tm *		result;
	const time_t			t = *timep;

	sp = lclptr;
	if ((sp->goback && t < sp->ats[0]) ||
		(sp->goahead && t > sp->ats[sp->timecnt - 1])) {
			time_t			newt = t;
			register time_t		seconds;
			register time_t		tcycles;
			register int_fast64_t	icycles;

			if (t < sp->ats[0])
				seconds = sp->ats[0] - t;
			else	seconds = t - sp->ats[sp->timecnt - 1];
			--seconds;
			tcycles = seconds / YEARSPERREPEAT / AVGSECSPERYEAR;
			++tcycles;
			icycles = tcycles;
			if (tcycles - icycles >= 1 || icycles - tcycles >= 1)
				return NULL;
			seconds = icycles;
			seconds *= YEARSPERREPEAT;
			seconds *= AVGSECSPERYEAR;
			if (t < sp->ats[0])
				newt += seconds;
			else	newt -= seconds;
			if (newt < sp->ats[0] ||
				newt > sp->ats[sp->timecnt - 1])
					return NULL;	/* "cannot happen" */
			result = localsub(&newt, offset, tmp);
			if (result == tmp) {
				register time_t	newy;

				newy = tmp->tm_year;
				if (t < sp->ats[0])
					newy -= icycles * YEARSPERREPEAT;
				else	newy += icycles * YEARSPERREPEAT;
				tmp->tm_year = newy;
				if (tmp->tm_year != newy)
					return NULL;
			}
			return result;
	}
	if (sp->timecnt == 0 || t < sp->ats[0]) {
		i = 0;
		while (sp->ttis[i].tt_isdst)
			if (++i >= sp->typecnt) {
				i = 0;
				break;
			}
	} else {
		register int	lo = 1;
		register int	hi = sp->timecnt;

		while (lo < hi) {
			register int	mid = (lo + hi) >> 1;

			if (t < sp->ats[mid])
				hi = mid;
			else	lo = mid + 1;
		}
		i = (int) sp->types[lo - 1];
	}
	ttisp = &sp->ttis[i];
	/*
	** To get (wrong) behavior that's compatible with System V Release 2.0
	** you'd replace the statement below with
	**	t += ttisp->tt_gmtoff;
	**	timesub(&t, 0L, sp, tmp);
	*/
	result = timesub(&t, ttisp->tt_gmtoff, sp, tmp);
	tmp->tm_isdst = ttisp->tt_isdst;
	tzname[tmp->tm_isdst] = &sp->chars[ttisp->tt_abbrind];
#ifdef TM_ZONE
	tmp->TM_ZONE = &sp->chars[ttisp->tt_abbrind];
#endif /* defined TM_ZONE */
	return result;
}


/*
** gmtsub is to gmtime as localsub is to localtime.
*/

static struct tm *
gmtsub(timep, offset, tmp)
const time_t * const	timep;
const long		offset;
struct tm * const	tmp;
{
	register struct tm *	result;

	if (!gmt_is_set)
		gmt_is_set = TRUE;
	result = timesub(timep, offset, gmtptr, tmp);
#ifdef TM_ZONE
	/*
	** Could get fancy here and deliver something such as
	** "UTC+xxxx" or "UTC-xxxx" if offset is non-zero,
	** but this is no time for a treasure hunt.
	*/
	if (offset != 0)
		tmp->TM_ZONE = wildabbr;
	else
		tmp->TM_ZONE = gmtptr->chars;
#endif /* defined TM_ZONE */
	return result;
}



// used in Haiku as fallback when the locale backend could not be loaded
struct tm*
__gmtime_r_fallback(const time_t* timep, struct tm* tmp)
{
	return gmtsub(timep, 0L, tmp);
}

/*
** Return the number of leap years through the end of the given year
** where, to make the math easy, the answer for year zero is defined as zero.
*/

static int
leaps_thru_end_of(y)
register const int	y;
{
	return (y >= 0) ? (y / 4 - y / 100 + y / 400) :
		-(leaps_thru_end_of(-(y + 1)) + 1);
}

static struct tm *
timesub(timep, offset, sp, tmp)
const time_t * const			timep;
const long				offset;
register const struct state * const	sp;
register struct tm * const		tmp;
{
	register const struct lsinfo *	lp;
	register time_t			tdays;
	register int			idays;	/* unsigned would be so 2003 */
	register long			rem;
	int				y;
	register const int *		ip;
	register long			corr;
	register int			hit;
	register int			i;

	corr = 0;
	hit = 0;
	i = sp->leapcnt;
	while (--i >= 0) {
		lp = &sp->lsis[i];
		if (*timep >= lp->ls_trans) {
			if (*timep == lp->ls_trans) {
				hit = ((i == 0 && lp->ls_corr > 0) ||
					lp->ls_corr > sp->lsis[i - 1].ls_corr);
				if (hit)
					while (i > 0 &&
						sp->lsis[i].ls_trans ==
						sp->lsis[i - 1].ls_trans + 1 &&
						sp->lsis[i].ls_corr ==
						sp->lsis[i - 1].ls_corr + 1) {
							++hit;
							--i;
					}
			}
			corr = lp->ls_corr;
			break;
		}
	}
	y = EPOCH_YEAR;
	tdays = *timep / SECSPERDAY;
	rem = *timep - tdays * SECSPERDAY;
	while (tdays < 0 || tdays >= year_lengths[isleap(y)]) {
		int		newy;
		register time_t	tdelta;
		register int	idelta;
		register int	leapdays;

		tdelta = tdays / DAYSPERLYEAR;
		idelta = tdelta;
		if (tdelta - idelta >= 1 || idelta - tdelta >= 1)
			return NULL;
		if (idelta == 0)
			idelta = (tdays < 0) ? -1 : 1;
		newy = y;
		if (increment_overflow(&newy, idelta))
			return NULL;
		leapdays = leaps_thru_end_of(newy - 1) -
			leaps_thru_end_of(y - 1);
		tdays -= ((time_t) newy - y) * DAYSPERNYEAR;
		tdays -= leapdays;
		y = newy;
	}
	{
		register long	seconds;

		seconds = tdays * SECSPERDAY + 0.5;
		tdays = seconds / SECSPERDAY;
		rem += seconds - tdays * SECSPERDAY;
	}
	/*
	** Given the range, we can now fearlessly cast...
	*/
	idays = tdays;
	rem += offset - corr;
	while (rem < 0) {
		rem += SECSPERDAY;
		--idays;
	}
	while (rem >= SECSPERDAY) {
		rem -= SECSPERDAY;
		++idays;
	}
	while (idays < 0) {
		if (increment_overflow(&y, -1))
			return NULL;
		idays += year_lengths[isleap(y)];
	}
	while (idays >= year_lengths[isleap(y)]) {
		idays -= year_lengths[isleap(y)];
		if (increment_overflow(&y, 1))
			return NULL;
	}
	tmp->tm_year = y;
	if (increment_overflow(&tmp->tm_year, -TM_YEAR_BASE))
		return NULL;
	tmp->tm_yday = idays;
	/*
	** The "extra" mods below avoid overflow problems.
	*/
	tmp->tm_wday = EPOCH_WDAY +
		((y - EPOCH_YEAR) % DAYSPERWEEK) *
		(DAYSPERNYEAR % DAYSPERWEEK) +
		leaps_thru_end_of(y - 1) -
		leaps_thru_end_of(EPOCH_YEAR - 1) +
		idays;
	tmp->tm_wday %= DAYSPERWEEK;
	if (tmp->tm_wday < 0)
		tmp->tm_wday += DAYSPERWEEK;
	tmp->tm_hour = (int) (rem / SECSPERHOUR);
	rem %= SECSPERHOUR;
	tmp->tm_min = (int) (rem / SECSPERMIN);
	/*
	** A positive leap second requires a special
	** representation. This uses "... ??:59:60" et seq.
	*/
	tmp->tm_sec = (int) (rem % SECSPERMIN) + hit;
	ip = mon_lengths[isleap(y)];
	for (tmp->tm_mon = 0; idays >= ip[tmp->tm_mon]; ++(tmp->tm_mon))
		idays -= ip[tmp->tm_mon];
	tmp->tm_mday = (int) (idays + 1);
	tmp->tm_isdst = 0;
#ifdef TM_GMTOFF
	tmp->TM_GMTOFF = offset;
#endif /* defined TM_GMTOFF */
	return tmp;
}

/*
** Adapted from code provided by Robert Elz, who writes:
**	The "best" way to do mktime I think is based on an idea of Bob
**	Kridle's (so its said...) from a long time ago.
**	It does a binary search of the time_t space. Since time_t's are
**	just 32 bits, its a max of 32 iterations (even at 64 bits it
**	would still be very reasonable).
*/

#ifndef WRONG
#define WRONG	(-1)
#endif /* !defined WRONG */

/*
** Simplified normalize logic courtesy Paul Eggert.
*/

static int
increment_overflow(number, delta)
int *	number;
int	delta;
{
	int	number0;

	number0 = *number;
	*number += delta;
	return (*number < number0) != (delta < 0);
}

static int
long_increment_overflow(number, delta)
long *	number;
int	delta;
{
	long	number0;

	number0 = *number;
	*number += delta;
	return (*number < number0) != (delta < 0);
}

static int
normalize_overflow(tensptr, unitsptr, base)
int * const	tensptr;
int * const	unitsptr;
const int	base;
{
	register int	tensdelta;

	tensdelta = (*unitsptr >= 0) ?
		(*unitsptr / base) :
		(-1 - (-1 - *unitsptr) / base);
	*unitsptr -= tensdelta * base;
	return increment_overflow(tensptr, tensdelta);
}

static int
long_normalize_overflow(tensptr, unitsptr, base)
long * const	tensptr;
int * const	unitsptr;
const int	base;
{
	register int	tensdelta;

	tensdelta = (*unitsptr >= 0) ?
		(*unitsptr / base) :
		(-1 - (-1 - *unitsptr) / base);
	*unitsptr -= tensdelta * base;
	return long_increment_overflow(tensptr, tensdelta);
}

static int
tmcomp(atmp, btmp)
register const struct tm * const atmp;
register const struct tm * const btmp;
{
	register int	result;

	if ((result = (atmp->tm_year - btmp->tm_year)) == 0 &&
		(result = (atmp->tm_mon - btmp->tm_mon)) == 0 &&
		(result = (atmp->tm_mday - btmp->tm_mday)) == 0 &&
		(result = (atmp->tm_hour - btmp->tm_hour)) == 0 &&
		(result = (atmp->tm_min - btmp->tm_min)) == 0)
			result = atmp->tm_sec - btmp->tm_sec;
	return result;
}

static time_t
time2sub(tmp, funcp, offset, okayp, do_norm_secs)
struct tm * const	tmp;
struct tm * (* const	funcp) P((const time_t*, long, struct tm*));
const long		offset;
int * const		okayp;
const int		do_norm_secs;
{
	register const struct state *	sp;
	register int			dir;
	register int			i, j;
	register int			saved_seconds;
	register long			li;
	register time_t			lo;
	register time_t			hi;
	long				y;
	time_t				newt;
	time_t				t;
	struct tm			yourtm, mytm;

	*okayp = FALSE;
	yourtm = *tmp;
	if (do_norm_secs) {
		if (normalize_overflow(&yourtm.tm_min, &yourtm.tm_sec,
			SECSPERMIN))
				return WRONG;
	}
	if (normalize_overflow(&yourtm.tm_hour, &yourtm.tm_min, MINSPERHOUR))
		return WRONG;
	if (normalize_overflow(&yourtm.tm_mday, &yourtm.tm_hour, HOURSPERDAY))
		return WRONG;
	y = yourtm.tm_year;
	if (long_normalize_overflow(&y, &yourtm.tm_mon, MONSPERYEAR))
		return WRONG;
	/*
	** Turn y into an actual year number for now.
	** It is converted back to an offset from TM_YEAR_BASE later.
	*/
	if (long_increment_overflow(&y, TM_YEAR_BASE))
		return WRONG;
	while (yourtm.tm_mday <= 0) {
		if (long_increment_overflow(&y, -1))
			return WRONG;
		li = y + (1 < yourtm.tm_mon);
		yourtm.tm_mday += year_lengths[isleap(li)];
	}
	while (yourtm.tm_mday > DAYSPERLYEAR) {
		li = y + (1 < yourtm.tm_mon);
		yourtm.tm_mday -= year_lengths[isleap(li)];
		if (long_increment_overflow(&y, 1))
			return WRONG;
	}
	for ( ; ; ) {
		i = mon_lengths[isleap(y)][yourtm.tm_mon];
		if (yourtm.tm_mday <= i)
			break;
		yourtm.tm_mday -= i;
		if (++yourtm.tm_mon >= MONSPERYEAR) {
			yourtm.tm_mon = 0;
			if (long_increment_overflow(&y, 1))
				return WRONG;
		}
	}
	if (long_increment_overflow(&y, -TM_YEAR_BASE))
		return WRONG;
	yourtm.tm_year = y;
	if (yourtm.tm_year != y)
		return WRONG;
	if (yourtm.tm_sec >= 0 && yourtm.tm_sec < SECSPERMIN)
		saved_seconds = 0;
	else if (y + TM_YEAR_BASE < EPOCH_YEAR) {
		/*
		** We can't set tm_sec to 0, because that might push the
		** time below the minimum representable time.
		** Set tm_sec to 59 instead.
		** This assumes that the minimum representable time is
		** not in the same minute that a leap second was deleted from,
		** which is a safer assumption than using 58 would be.
		*/
		if (increment_overflow(&yourtm.tm_sec, 1 - SECSPERMIN))
			return WRONG;
		saved_seconds = yourtm.tm_sec;
		yourtm.tm_sec = SECSPERMIN - 1;
	} else {
		saved_seconds = yourtm.tm_sec;
		yourtm.tm_sec = 0;
	}
	/*
	** Do a binary search (this works whatever time_t's type is).
	*/
	if (!TYPE_SIGNED(time_t)) {
		lo = 0;
		hi = lo - 1;
	} else if (!TYPE_INTEGRAL(time_t)) {
		if (sizeof(time_t) > sizeof(float))
			hi = (time_t) DBL_MAX;
		else	hi = (time_t) FLT_MAX;
		lo = -hi;
	} else {
		lo = 1;
		for (i = 0; i < (int) TYPE_BIT(time_t) - 1; ++i)
			lo *= 2;
		hi = -(lo + 1);
	}
	for ( ; ; ) {
		t = lo / 2 + hi / 2;
		if (t < lo)
			t = lo;
		else if (t > hi)
			t = hi;
		if ((*funcp)(&t, offset, &mytm) == NULL) {
			/*
			** Assume that t is too extreme to be represented in
			** a struct tm; arrange things so that it is less
			** extreme on the next pass.
			*/
			dir = (t > 0) ? 1 : -1;
		} else	dir = tmcomp(&mytm, &yourtm);
		if (dir != 0) {
			if (t == lo) {
				++t;
				if (t <= lo)
					return WRONG;
				++lo;
			} else if (t == hi) {
				--t;
				if (t >= hi)
					return WRONG;
				--hi;
			}
			if (lo > hi)
				return WRONG;
			if (dir > 0)
				hi = t;
			else	lo = t;
			continue;
		}
		if (yourtm.tm_isdst < 0 || mytm.tm_isdst == yourtm.tm_isdst)
			break;
		/*
		** Right time, wrong type.
		** Hunt for right time, right type.
		** It's okay to guess wrong since the guess
		** gets checked.
		*/
		/*
		** The (void *) casts are the benefit of SunOS 3.3 on Sun 2's.
		*/
		sp = (const struct state *)
			(((void *) funcp == (void *) localsub) ?
			lclptr : gmtptr);
		for (i = sp->typecnt - 1; i >= 0; --i) {
			if (sp->ttis[i].tt_isdst != yourtm.tm_isdst)
				continue;
			for (j = sp->typecnt - 1; j >= 0; --j) {
				if (sp->ttis[j].tt_isdst == yourtm.tm_isdst)
					continue;
				newt = t + sp->ttis[j].tt_gmtoff -
					sp->ttis[i].tt_gmtoff;
				if ((*funcp)(&newt, offset, &mytm) == NULL)
					continue;
				if (tmcomp(&mytm, &yourtm) != 0)
					continue;
				if (mytm.tm_isdst != yourtm.tm_isdst)
					continue;
				/*
				** We have a match.
				*/
				t = newt;
				goto label;
			}
		}
		return WRONG;
	}
label:
	newt = t + saved_seconds;
	if ((newt < t) != (saved_seconds < 0))
		return WRONG;
	t = newt;
	if ((*funcp)(&t, offset, tmp))
		*okayp = TRUE;
	return t;
}

static time_t
time2(tmp, funcp, offset, okayp)
struct tm * const	tmp;
struct tm * (* const	funcp) P((const time_t*, long, struct tm*));
const long		offset;
int * const		okayp;
{
	time_t	t;

	/*
	** First try without normalization of seconds
	** (in case tm_sec contains a value associated with a leap second).
	** If that fails, try with normalization of seconds.
	*/
	t = time2sub(tmp, funcp, offset, okayp, FALSE);
	return *okayp ? t : time2sub(tmp, funcp, offset, okayp, TRUE);
}

static time_t
time1(tmp, funcp, offset)
struct tm * const	tmp;
struct tm * (* const	funcp) P((const time_t *, long, struct tm *));
const long		offset;
{
	register time_t			t;
	register const struct state *	sp;
	register int			samei, otheri;
	register int			sameind, otherind;
	register int			i;
	register int			nseen;
	int				seen[TZ_MAX_TYPES];
	int				types[TZ_MAX_TYPES];
	int				okay;

	if (tmp->tm_isdst > 1)
		tmp->tm_isdst = 1;
	t = time2(tmp, funcp, offset, &okay);
	/*
	** PCTS code courtesy Grant Sullivan.
	*/
	if (okay)
		return t;
	if (tmp->tm_isdst < 0)
		tmp->tm_isdst = 0;	/* reset to std and try again */
	/*
	** We're supposed to assume that somebody took a time of one type
	** and did some math on it that yielded a "struct tm" that's bad.
	** We try to divine the type they started from and adjust to the
	** type they need.
	*/
	/*
	** The (void *) casts are the benefit of SunOS 3.3 on Sun 2's.
	*/
	sp = (const struct state *) (((void *) funcp == (void *) localsub) ?
		lclptr : gmtptr);
	for (i = 0; i < sp->typecnt; ++i)
		seen[i] = FALSE;
	nseen = 0;
	for (i = sp->timecnt - 1; i >= 0; --i)
		if (!seen[sp->types[i]]) {
			seen[sp->types[i]] = TRUE;
			types[nseen++] = sp->types[i];
		}
	for (sameind = 0; sameind < nseen; ++sameind) {
		samei = types[sameind];
		if (sp->ttis[samei].tt_isdst != tmp->tm_isdst)
			continue;
		for (otherind = 0; otherind < nseen; ++otherind) {
			otheri = types[otherind];
			if (sp->ttis[otheri].tt_isdst == tmp->tm_isdst)
				continue;
			tmp->tm_sec += sp->ttis[otheri].tt_gmtoff -
					sp->ttis[samei].tt_gmtoff;
			tmp->tm_isdst = !tmp->tm_isdst;
			t = time2(tmp, funcp, offset, &okay);
			if (okay)
				return t;
			tmp->tm_sec -= sp->ttis[otheri].tt_gmtoff -
					sp->ttis[samei].tt_gmtoff;
			tmp->tm_isdst = !tmp->tm_isdst;
		}
	}
	return WRONG;
}


// used in Haiku as fallback when the locale backend could not be loaded
time_t
__mktime_fallback(struct tm* tmp)
{
	return time1(tmp, localsub, 0L);
}



time_t
timegm(tmp)
struct tm * const	tmp;
{
	tmp->tm_isdst = 0;
	return time1(tmp, gmtsub, 0L);
}
