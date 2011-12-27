/*
 * Copyright 2010, Oliver Tappe, zooey@hirschkaefer.de
 * Distributed under the terms of the MIT License.
 */

#include <ctype.h>
#include <errno.h>
#include <langinfo.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wctype.h>


// #pragma mark - setlocale ----------------------------------------------------


void
test_setlocale()
{
	const char* locales[] = {
		"POSIX",
		"C",
		"de_DE",
		"en_US",
		"en_US.US-ASCII",
		"hr_HR.ISO-8859-2",
		"nl_NL",
		"nb_NO",
		"fr_FR.UTF-8@collation=phonebook",
		"de_DE.iso8859-1",
		"De_dE.IsO8859-15",
		"de_DE.utf8",
		"de_DE.UTF-8",
		"de_DE@euro",
		"de_DE@EURO",
		"de_DE.utf-8@Euro",
		"POSIX",
		"C",
		NULL
	};
	const char* expectedLocales[] = {
		"POSIX",
		"POSIX",
		"de_DE",
		"en_US",
		"en_US.US-ASCII",
		"hr_HR.ISO-8859-2",
		"nl_NL",
		"nb_NO",
		"fr_FR.UTF-8@collation=phonebook",
		"de_DE.iso8859-1",
		"De_dE.IsO8859-15",
		"de_DE.utf8",
		"de_DE.UTF-8",
		"de_DE@euro",
		"de_DE@EURO",
		"de_DE.utf-8@Euro",
		"POSIX",
		"POSIX"
	};
	const char* categoryNames[] = {
		"LC_ALL",
		"LC_COLLATE",
		"LC_CTYPE",
		"LC_MONETARY",
		"LC_NUMERIC",
		"LC_TIME",
		"LC_MESSAGES"
	};
	printf("setlocale()\n");

	int problemCount = 0;
	for (int i = 0; locales[i] != NULL; ++i) {
		char* result = setlocale(LC_ALL, locales[i]);
		if (!result || strcmp(result, expectedLocales[i]) != 0) {
			printf("\tPROBLEM: setlocale(LC_ALL, \"%s\") = \"%s\" "
					"(expected \"%s\")\n",
				locales[i], result, expectedLocales[i]);
			problemCount++;
		}
	}

	for (int i = 1; i <= LC_LAST; ++i)
		setlocale(i, locales[i + 1]);
	char* result = setlocale(LC_ALL, NULL);
	const char* expectedResult
		= "LC_COLLATE=de_DE;LC_CTYPE=en_US;LC_MESSAGES=nb_NO;"
			"LC_MONETARY=en_US.US-ASCII;LC_NUMERIC=hr_HR.ISO-8859-2;"
			"LC_TIME=nl_NL";
	if (!result || strcmp(result, expectedResult) != 0) {
		printf("\tPROBLEM: setlocale(LC_ALL, NULL) = \"%s\" "
				"(expected \"%s\")\n", result, expectedResult);
		problemCount++;
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - localeconv ---------------------------------------------------


void
dumpGrouping(const char* grouping, char* buf)
{
	for (char* bufPtr = buf; *grouping; ++grouping)
		bufPtr += sprintf(bufPtr, "\\x%02x", *grouping);
}


void
test_localeconv(const char* locale, const lconv* localeConv)
{
	setlocale(LC_MONETARY, locale);
	setlocale(LC_NUMERIC, locale);
	printf("localeconv for '%s'\n", locale);

	int problemCount = 0;
	struct lconv* lc = localeconv();
	if (!lc)
		printf("not ok - got no result from localeconv()\n");
	else {
		if (strcmp(lc->decimal_point, localeConv->decimal_point) != 0) {
			printf("\tPROBLEM: lc.decimal_point = \"%s\" (expected \"%s\")\n",
				lc->decimal_point, localeConv->decimal_point);
			problemCount++;
		}
		if (strcmp(lc->thousands_sep, localeConv->thousands_sep) != 0) {
			printf("\tPROBLEM: lc.thousands_sep = \"%s\" (expected \"%s\")\n",
				lc->thousands_sep, localeConv->thousands_sep);
			problemCount++;
		}
		if (strcmp(lc->grouping, localeConv->grouping) != 0) {
			char gotGrouping[20], expectedGrouping[20];
			dumpGrouping(lc->grouping, gotGrouping);
			dumpGrouping(localeConv->grouping, expectedGrouping);
			printf("\tPROBLEM: lc.grouping = \"%s\" (expected \"%s\")\n",
				gotGrouping, expectedGrouping);
			problemCount++;
		}
		if (strcmp(lc->int_curr_symbol, localeConv->int_curr_symbol) != 0) {
			printf("\tPROBLEM: lc.int_curr_symbol = \"%s\" (expected \"%s\")\n",
				lc->int_curr_symbol, localeConv->int_curr_symbol);
			problemCount++;
		}
		if (strcmp(lc->currency_symbol, localeConv->currency_symbol) != 0) {
			printf("\tPROBLEM: lc.currency_symbol = \"%s\" (expected \"%s\")\n",
				lc->currency_symbol, localeConv->currency_symbol);
			problemCount++;
		}
		if (strcmp(lc->mon_decimal_point, localeConv->mon_decimal_point) != 0) {
			printf("\tPROBLEM: lc.mon_decimal_point = \"%s\" "
					"(expected \"%s\")\n",
				lc->mon_decimal_point, localeConv->mon_decimal_point);
			problemCount++;
		}
		if (strcmp(lc->mon_thousands_sep, localeConv->mon_thousands_sep) != 0) {
			printf("\tPROBLEM: lc.mon_thousands_sep = \"%s\" "
					"(expected \"%s\")\n",
				lc->mon_thousands_sep, localeConv->mon_thousands_sep);
			problemCount++;
		}
		if (strcmp(lc->mon_grouping, localeConv->mon_grouping) != 0) {
			char gotGrouping[20], expectedGrouping[20];
			dumpGrouping(lc->mon_grouping, gotGrouping);
			dumpGrouping(localeConv->mon_grouping, expectedGrouping);
			printf("\tPROBLEM: lc.mon_grouping: \"%s\" (expected \"%s\")\n",
				gotGrouping, expectedGrouping);
			problemCount++;
		}
		if (strcmp(lc->positive_sign, localeConv->positive_sign) != 0) {
			printf("\tPROBLEM: lc.positive_sign = \"%s\" (expected \"%s\")\n",
				lc->positive_sign, localeConv->positive_sign);
			problemCount++;
		}
		if (strcmp(lc->negative_sign, localeConv->negative_sign) != 0) {
			printf("\tPROBLEM: lc.negative_sign = \"%s\" (expected \"%s\")\n",
				lc->negative_sign, localeConv->negative_sign);
			problemCount++;
		}
		if (lc->frac_digits != localeConv->frac_digits) {
			printf("\tPROBLEM: lc.frac_digits = %d (expected %d)\n",
				lc->frac_digits, localeConv->frac_digits);
			problemCount++;
		}
		if (lc->int_frac_digits != localeConv->int_frac_digits) {
			printf("\tPROBLEM: lc.int_frac_digits = %d (expected %d)\n",
				lc->int_frac_digits, localeConv->int_frac_digits);
			problemCount++;
		}
		if (lc->p_cs_precedes != localeConv->p_cs_precedes) {
			printf("\tPROBLEM: lc.p_cs_precedes = %d (expected %d)\n",
				lc->p_cs_precedes, localeConv->p_cs_precedes);
			problemCount++;
		}
		if (lc->p_sep_by_space != localeConv->p_sep_by_space) {
			printf("\tPROBLEM: lc.p_sep_by_space = %d (expected %d)\n",
				lc->p_sep_by_space, localeConv->p_sep_by_space);
			problemCount++;
		}
		if (lc->p_sign_posn != localeConv->p_sign_posn) {
			printf("\tPROBLEM: lc.p_sign_posn = %d (expected %d)\n",
				lc->p_sign_posn, localeConv->p_sign_posn);
			problemCount++;
		}
		if (lc->n_cs_precedes != localeConv->n_cs_precedes) {
			printf("\tPROBLEM: lc.n_cs_precedes = %d (expected %d)\n",
				lc->n_cs_precedes, localeConv->n_cs_precedes);
			problemCount++;
		}
		if (lc->n_sep_by_space != localeConv->n_sep_by_space) {
			printf("\tPROBLEM: lc.n_sep_by_space = %d (expected %d)\n",
				lc->n_sep_by_space, localeConv->n_sep_by_space);
			problemCount++;
		}
		if (lc->n_sign_posn != localeConv->n_sign_posn) {
			printf("\tPROBLEM: lc.n_sign_posn = %d (expected %d)\n",
				lc->n_sign_posn, localeConv->n_sign_posn);
			problemCount++;
		}
		if (lc->int_p_cs_precedes != localeConv->int_p_cs_precedes) {
			printf("\tPROBLEM: lc.int_p_cs_precedes = %d (expected %d)\n",
				lc->int_p_cs_precedes, localeConv->int_p_cs_precedes);
			problemCount++;
		}
		if (lc->int_p_sep_by_space != localeConv->int_p_sep_by_space) {
			printf("\tPROBLEM: lc.int_p_sep_by_space = %d (expected %d)\n",
				lc->int_p_sep_by_space, localeConv->int_p_sep_by_space);
			problemCount++;
		}
		if (lc->int_p_sign_posn != localeConv->int_p_sign_posn) {
			printf("\tPROBLEM: lc.int_p_sign_posn = %d (expected %d)\n",
				lc->int_p_sign_posn, localeConv->int_p_sign_posn);
			problemCount++;
		}
		if (lc->int_n_cs_precedes != localeConv->int_n_cs_precedes) {
			printf("\tPROBLEM: lc.int_n_cs_precedes = %d (expected %d)\n",
				lc->int_n_cs_precedes, localeConv->int_n_cs_precedes);
			problemCount++;
		}
		if (lc->int_n_sep_by_space != localeConv->int_n_sep_by_space) {
			printf("\tPROBLEM: lc.int_n_sep_by_space = %d (expected %d)\n",
				lc->int_n_sep_by_space, localeConv->int_n_sep_by_space);
			problemCount++;
		}
		if (lc->int_n_sign_posn != localeConv->int_n_sign_posn) {
			printf("\tPROBLEM: lc.int_n_sign_posn = %d (expected %d)\n",
				lc->int_n_sign_posn, localeConv->int_n_sign_posn);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_localeconv()
{
	const lconv lconv_posix = {
		(char*)".",
		(char*)"",
		(char*)"",
		(char*)"",
		(char*)"",
		(char*)"",
		(char*)"",
		(char*)"",
		(char*)"",
		(char*)"",
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX,
		CHAR_MAX
	};
	test_localeconv("POSIX", &lconv_posix);

	const lconv lconv_de = {
		(char*)",",
		(char*)".",
		(char*)"\x03",
		(char*)"EUR ",
		(char*)"€",
		(char*)",",
		(char*)".",
		(char*)"\x03",
		(char*)"",
		(char*)"-",
		2,
		2,
		0,
		1,
		0,
		1,
		1,
		1,
		0,
		1,
		0,
		1,
		1,
		1
	};
	test_localeconv("de_DE", &lconv_de);

	const lconv lconv_de_iso = {
		(char*)",",
		(char*)".",
		(char*)"\x03",
		(char*)"EUR ",
		(char*)"EUR",
		(char*)",",
		(char*)".",
		(char*)"\x03",
		(char*)"",
		(char*)"-",
		2,
		2,
		0,
		1,
		0,
		1,
		1,
		1,
		0,
		1,
		0,
		1,
		1,
		1
	};
	test_localeconv("de_DE.ISO8859-1", &lconv_de_iso);

	const lconv lconv_hr = {
		(char*)",",
		(char*)".",
		(char*)"\x03",
		(char*)"HRK ",
		(char*)"kn",
		(char*)",",
		(char*)".",
		(char*)"\x03",
		(char*)"",
		(char*)"-",
		2,
		2,
		0,
		1,
		0,
		1,
		1,
		1,
		0,
		1,
		0,
		1,
		1,
		1
	};
	test_localeconv("hr_HR.ISO8859-2", &lconv_hr);

	const lconv lconv_de_CH = {
		(char*)".",
		(char*)"'",
		(char*)"\x03",
		(char*)"CHF ",
		(char*)"CHF",
		(char*)".",
		(char*)"'",
		(char*)"\x03",
		(char*)"",
		(char*)"-",
		2,
		2,
		1,
		1,
		1,
		0,
		4,
		4,
		1,
		1,
		1,
		0,
		4,
		4
	};
	test_localeconv("de_CH", &lconv_de_CH);

	const lconv lconv_gu_IN = {
		(char*)".",
		(char*)",",
		(char*)"\x03\x02",
		(char*)"INR ",
		(char*)"\xE2\x82\xB9",
		(char*)".",
		(char*)",",
		(char*)"\x03\x02",
		(char*)"",
		(char*)"-",
		2,
		2,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1
	};
	test_localeconv("gu_IN", &lconv_gu_IN);

	const lconv lconv_it = {
		(char*)",",
		(char*)".",
		(char*)"\x03",
		(char*)"EUR ",
		(char*)"€",
		(char*)",",
		(char*)".",
		(char*)"\x03",
		(char*)"",
		(char*)"-",
		2,
		2,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1
	};
	test_localeconv("it_IT", &lconv_it);

	const lconv lconv_nl = {
		(char*)",",
		(char*)".",
		(char*)"\x03",
		(char*)"EUR ",
		(char*)"€",
		(char*)",",
		(char*)".",
		(char*)"\x03",
		(char*)"",
		(char*)"-",
		2,
		2,
		1,
		1,
		1,
		1,
		2,
		2,
		1,
		1,
		1,
		1,
		2,
		2
	};
	test_localeconv("nl_NL", &lconv_nl);

	const lconv lconv_nb = {
		(char*)",",
		(char*)" ",
		(char*)"\x03",
		(char*)"NOK ",
		(char*)"kr",
		(char*)",",
		(char*)" ",
		(char*)"\x03",
		(char*)"",
		(char*)"-",
		2,
		2,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1,
		1
	};
	test_localeconv("nb_NO", &lconv_nb);
}


// #pragma mark - strftime -----------------------------------------------------


struct strftime_data {
	const char* format;
	const char* result;
};


void
test_strftime(const char* locale, const strftime_data data[])
{
	setlocale(LC_TIME, locale);
	printf("strftime for '%s'\n", locale);

	time_t nowSecs = 1279391169;	// pure magic
	tm* now = localtime(&nowSecs);
	int problemCount = 0;
	for(int i = 0; data[i].format != NULL; ++i) {
		char buf[100];
		strftime(buf, 100, data[i].format, now);
		if (strcmp(buf, data[i].result) != 0) {
			printf("\tPROBLEM: strftime(\"%s\") = \"%s\" (expected \"%s\")\n",
				data[i].format, buf, data[i].result);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_strftime()
{
	setenv("TZ", "GMT", 1);

	const strftime_data strftime_posix[] = {
		{ "%c", "Sat Jul 17 18:26:09 2010" },
		{ "%x", "07/17/10" },
		{ "%X", "18:26:09" },
		{ "%a", "Sat" },
		{ "%A", "Saturday" },
		{ "%b", "Jul" },
		{ "%B", "July" },
		{ NULL, NULL }
	};
	test_strftime("POSIX", strftime_posix);

	const strftime_data strftime_de[] = {
		{ "%c", "Samstag, 17. Juli 2010 18:26:09 GMT" },
		{ "%x", "17.07.2010" },
		{ "%X", "18:26:09" },
		{ "%a", "Sa." },
		{ "%A", "Samstag" },
		{ "%b", "Jul" },
		{ "%B", "Juli" },
		{ NULL, NULL }
	};
	test_strftime("de_DE.UTF-8", strftime_de);

	const strftime_data strftime_hr[] = {
		{ "%c", "subota, 17. srpnja 2010. 18:26:09 GMT" },
		{ "%x", "17. 07. 2010." },
		{ "%X", "18:26:09" },
		{ "%a", "sub" },
		{ "%A", "subota" },
		{ "%b", "srp" },
		{ "%B", "srpnja" },
		{ NULL, NULL }
	};
	test_strftime("hr_HR.ISO8859-2", strftime_hr);

	const strftime_data strftime_gu[] = {
		{ "%c", "શનિવાર, 17 જુલાઈ, 2010 06:26:09 PM GMT" },
		{ "%x", "17 જુલાઈ, 2010" },
		{ "%X", "06:26:09 PM" },
		{ "%a", "શનિ" },
		{ "%A", "શનિવાર" },
		{ "%b", "જુલાઈ" },
		{ "%B", "જુલાઈ" },
		{ NULL, NULL }
	};
	test_strftime("gu_IN", strftime_gu);

	const strftime_data strftime_it[] = {
		{ "%c", "sabato 17 luglio 2010 18:26:09 GMT" },
		{ "%x", "17/lug/2010" },
		{ "%X", "18:26:09" },
		{ "%a", "sab" },
		{ "%A", "sabato" },
		{ "%b", "lug" },
		{ "%B", "luglio" },
		{ NULL, NULL }
	};
	test_strftime("it_IT", strftime_it);

	const strftime_data strftime_nl[] = {
		{ "%c", "zaterdag 17 juli 2010 18:26:09 GMT" },
		{ "%x", "17 jul. 2010" },
		{ "%X", "18:26:09" },
		{ "%a", "za" },
		{ "%A", "zaterdag" },
		{ "%b", "jul." },
		{ "%B", "juli" },
		{ NULL, NULL }
	};
	test_strftime("nl_NL", strftime_nl);

	const strftime_data strftime_nb[] = {
		{ "%c", "kl. 18:26:09 GMT lørdag 17. juli 2010" },
		{ "%x", "17. juli 2010" },
		{ "%X", "18:26:09" },
		{ "%a", "lør." },
		{ "%A", "lørdag" },
		{ "%b", "juli" },
		{ "%B", "juli" },
		{ NULL, NULL }
	};
	test_strftime("nb_NO", strftime_nb);
}


// #pragma mark - ctype --------------------------------------------------------


unsigned short
determineFullClassInfo(int i)
{
	unsigned short classInfo = 0;

	if (isblank(i))
		classInfo |= _ISblank;
	if (iscntrl(i))
		classInfo |= _IScntrl;
	if (ispunct(i))
		classInfo |= _ISpunct;
	if (isalnum(i))
		classInfo |= _ISalnum;
	if (isupper(i))
		classInfo |= _ISupper;
	if (islower(i))
		classInfo |= _ISlower;
	if (isalpha(i))
		classInfo |= _ISalpha;
	if (isdigit(i))
		classInfo |= _ISdigit;
	if (isxdigit(i))
		classInfo |= _ISxdigit;
	if (isspace(i))
		classInfo |= _ISspace;
	if (isprint(i))
		classInfo |= _ISprint;
	if (isgraph(i))
		classInfo |= _ISgraph;

	return classInfo;
}


void
test_ctype(const char* locale, const unsigned short int classInfos[],
	const int toLowerMap[], const int toUpperMap[])
{
	setlocale(LC_CTYPE, locale);
	printf("ctype of %s locale\n", locale);

	int problemCount = 0;
	for (int i = -1; i < 256; ++i) {
		unsigned short classInfo = determineFullClassInfo(i);

		if (i < 255) {
			char iAsChar = (char)i;
			unsigned short classInfoFromChar = determineFullClassInfo(iAsChar);

			if (classInfo != classInfoFromChar) {
				printf("\tPROBLEM: ctype((int)%d)=%x, but ctype((char)%d)=%x\n",
					i, classInfo, i, classInfoFromChar);
				problemCount++;
			}
		}
		if (classInfo != classInfos[i + 1]) {
			printf("\tPROBLEM: ctype(%d) = %x (expected %x)\n", i, classInfo,
				classInfos[i + 1]);
			problemCount++;
		}
		int lower = tolower(i);
		if (lower != toLowerMap[i + 1]) {
			printf("\tPROBLEM: tolower(%d) = %x (expected %x)\n", i, lower,
				toLowerMap[i + 1]);
			problemCount++;
		}
		int upper = toupper(i);
		if (upper != toUpperMap[i + 1]) {
			printf("\tPROBLEM: toupper(%d) = %x (expected %x)\n", i, upper,
				toUpperMap[i + 1]);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_ctype()
{
	const unsigned short int classInfos_posix[257] = {
		/*  -1 */   0,	// neutral value
		/*   0 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/*   8 */	_IScntrl, _ISblank|_IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl, _IScntrl,
		/*  16 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/*  24 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/*  32 */	_ISblank|_ISspace|_ISprint, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  40 */	_ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  48 */	_ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph,
		/*  56 */	_ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  64 */	_ISpunct|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
		/*  72 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
		/*  80 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
		/*  88 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  96 */	_ISpunct|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
		/* 104 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
		/* 112 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
		/* 120 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _IScntrl,
		/* 128 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 136 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 144 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 152 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 160 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 168 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 176 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 184 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 192 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 200 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 208 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 216 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 224 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 232 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 240 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 248 */	0, 0, 0, 0, 0, 0, 0, 0,
	};
	const int toLowerMap_posix[257] = {
		/*  -1 */    -1,	// identity value
		/*   0 */	  0,   1,   2,   3,   4,   5,   6,   7,
		/*   8 */	  8,   9,  10,  11,  12,  13,  14,  15,
		/*  16 */	 16,  17,  18,  19,  20,  21,  22,  23,
		/*  24 */	 24,  25,  26,  27,  28,  29,  30,  31,
		/*  32 */	 32,  33,  34,  35,  36,  37,  38,  39,
		/*  40 */	 40,  41,  42,  43,  44,  45,  46,  47,
		/*  48 */	'0', '1', '2', '3', '4', '5', '6', '7',
		/*  56 */	'8', '9',  58,  59,  60,  61,  62,  63,
		/*  64 */	 64, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
		/*  72 */	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		/*  80 */	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
		/*  88 */	'x', 'y', 'z',  91,  92,  93,  94,  95,
		/*  96 */	 96, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
		/* 104 */	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		/* 112 */	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
		/* 120 */	'x', 'y', 'z', 123, 124, 125, 126, 127,
		/* 128 */	128, 129, 130, 131, 132, 133, 134, 135,
		/* 136 */	136, 137, 138, 139, 140, 141, 142, 143,
		/* 144 */	144, 145, 146, 147, 148, 149, 150, 151,
		/* 152 */	152, 153, 154, 155, 156, 157, 158, 159,
		/* 160 */	160, 161, 162, 163, 164, 165, 166, 167,
		/* 168 */	168, 169, 170, 171, 172, 173, 174, 175,
		/* 176 */	176, 177, 178, 179, 180, 181, 182, 183,
		/* 184 */	184, 185, 186, 187, 188, 189, 190, 191,
		/* 192 */	192, 193, 194, 195, 196, 197, 198, 199,
		/* 200 */	200, 201, 202, 203, 204, 205, 206, 207,
		/* 208 */	208, 209, 210, 211, 212, 213, 214, 215,
		/* 216 */	216, 217, 218, 219, 220, 221, 222, 223,
		/* 224 */	224, 225, 226, 227, 228, 229, 230, 231,
		/* 232 */	232, 233, 234, 235, 236, 237, 238, 239,
		/* 240 */	240, 241, 242, 243, 244, 245, 246, 247,
		/* 248 */	248, 249, 250, 251, 252, 253, 254, 255,
	};
	const int toUpperMap_posix[257] = {
		/*  -1 */    -1,	// identity value
		/*   0 */	  0,   1,   2,   3,   4,   5,   6,   7,
		/*   8 */	  8,   9,  10,  11,  12,  13,  14,  15,
		/*  16 */	 16,  17,  18,  19,  20,  21,  22,  23,
		/*  24 */	 24,  25,  26,  27,  28,  29,  30,  31,
		/*  32 */	 32,  33,  34,  35,  36,  37,  38,  39,
		/*  40 */	 40,  41,  42,  43,  44,  45,  46,  47,
		/*  48 */	'0', '1', '2', '3', '4', '5', '6', '7',
		/*  56 */	'8', '9',  58,  59,  60,  61,  62,  63,
		/*  64 */	 64, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
		/*  72 */	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		/*  80 */	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
		/*  88 */	'X', 'Y', 'Z',  91,  92,  93,  94,  95,
		/*  96 */	 96, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
		/* 104 */	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		/* 112 */	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
		/* 120 */	'X', 'Y', 'Z', 123, 124, 125, 126, 127,
		/* 128 */	128, 129, 130, 131, 132, 133, 134, 135,
		/* 136 */	136, 137, 138, 139, 140, 141, 142, 143,
		/* 144 */	144, 145, 146, 147, 148, 149, 150, 151,
		/* 152 */	152, 153, 154, 155, 156, 157, 158, 159,
		/* 160 */	160, 161, 162, 163, 164, 165, 166, 167,
		/* 168 */	168, 169, 170, 171, 172, 173, 174, 175,
		/* 176 */	176, 177, 178, 179, 180, 181, 182, 183,
		/* 184 */	184, 185, 186, 187, 188, 189, 190, 191,
		/* 192 */	192, 193, 194, 195, 196, 197, 198, 199,
		/* 200 */	200, 201, 202, 203, 204, 205, 206, 207,
		/* 208 */	208, 209, 210, 211, 212, 213, 214, 215,
		/* 216 */	216, 217, 218, 219, 220, 221, 222, 223,
		/* 224 */	224, 225, 226, 227, 228, 229, 230, 231,
		/* 232 */	232, 233, 234, 235, 236, 237, 238, 239,
		/* 240 */	240, 241, 242, 243, 244, 245, 246, 247,
		/* 248 */	248, 249, 250, 251, 252, 253, 254, 255,
	};
	test_ctype("POSIX", classInfos_posix, toLowerMap_posix, toUpperMap_posix);

	const unsigned short int classInfos_de[257] = {
		/*  -1 */   0,	// neutral value
		/*   0 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/*   8 */	_IScntrl, _ISblank|_IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl, _IScntrl,
		/*  16 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/*  24 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/*  32 */	_ISblank|_ISspace|_ISprint, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  40 */	_ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  48 */	_ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph,
		/*  56 */	_ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  64 */	_ISpunct|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
		/*  72 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
		/*  80 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
		/*  88 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  96 */	_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
		/* 104 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
		/* 112 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
		/* 120 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _IScntrl,
		/* 128 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl|_ISspace, _IScntrl, _IScntrl,
		/* 136 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/* 144 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/* 152 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/* 160 */	_ISprint|_ISspace|_ISblank, _ISprint|_ISgraph|_ISpunct, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph,
		/* 168 */	_ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISpunct, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph,
		/* 176 */	_ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph, _ISprint|_ISgraph|_ISpunct,
		/* 184 */	_ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISpunct, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph|_ISpunct,
		/* 192 */	_ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper,
		/* 200 */	_ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper,
		/* 208 */	_ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph,
		/* 216 */	_ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISupper, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower,
		/* 224 */	_ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower,
		/* 232 */	_ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower,
		/* 240 */	_ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph,
		/* 248 */	_ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower, _ISprint|_ISgraph|_ISalnum|_ISalpha|_ISlower,
	};
	const int toLowerMap_de[257] = {
		/*  -1 */    -1,	// identity value
		/*   0 */	  0,   1,   2,   3,   4,   5,   6,   7,
		/*   8 */	  8,   9,  10,  11,  12,  13,  14,  15,
		/*  16 */	 16,  17,  18,  19,  20,  21,  22,  23,
		/*  24 */	 24,  25,  26,  27,  28,  29,  30,  31,
		/*  32 */	 32,  33,  34,  35,  36,  37,  38,  39,
		/*  40 */	 40,  41,  42,  43,  44,  45,  46,  47,
		/*  48 */	'0', '1', '2', '3', '4', '5', '6', '7',
		/*  56 */	'8', '9',  58,  59,  60,  61,  62,  63,
		/*  64 */	 64, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
		/*  72 */	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		/*  80 */	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
		/*  88 */	'x', 'y', 'z',  91,  92,  93,  94,  95,
		/*  96 */	 96, 'a', 'b', 'c', 'd', 'e', 'f', 'g',
		/* 104 */	'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o',
		/* 112 */	'p', 'q', 'r', 's', 't', 'u', 'v', 'w',
		/* 120 */	'x', 'y', 'z', 123, 124, 125, 126, 127,
		/* 128 */	128, 129, 130, 131, 132, 133, 134, 135,
		/* 136 */	136, 137, 138, 139, 140, 141, 142, 143,
		/* 144 */	144, 145, 146, 147, 148, 149, 150, 151,
		/* 152 */	152, 153, 154, 155, 156, 157, 158, 159,
		/* 160 */	160, 161, 162, 163, 164, 165, 166, 167,
		/* 168 */	168, 169, 170, 171, 172, 173, 174, 175,
		/* 176 */	176, 177, 178, 179, 180, 181, 182, 183,
		/* 184 */	184, 185, 186, 187, 188, 189, 190, 191,
		/* 192 */	224, 225, 226, 227, 228, 229, 230, 231,
		/* 200 */	232, 233, 234, 235, 236, 237, 238, 239,
		/* 208 */	240, 241, 242, 243, 244, 245, 246, 215,
		/* 216 */	248, 249, 250, 251, 252, 253, 254, 223,
		/* 224 */	224, 225, 226, 227, 228, 229, 230, 231,
		/* 232 */	232, 233, 234, 235, 236, 237, 238, 239,
		/* 240 */	240, 241, 242, 243, 244, 245, 246, 247,
		/* 248 */	248, 249, 250, 251, 252, 253, 254, 255,
	};
	const int toUpperMap_de[257] = {
		/*  -1 */    -1,	// identity value
		/*   0 */	  0,   1,   2,   3,   4,   5,   6,   7,
		/*   8 */	  8,   9,  10,  11,  12,  13,  14,  15,
		/*  16 */	 16,  17,  18,  19,  20,  21,  22,  23,
		/*  24 */	 24,  25,  26,  27,  28,  29,  30,  31,
		/*  32 */	 32,  33,  34,  35,  36,  37,  38,  39,
		/*  40 */	 40,  41,  42,  43,  44,  45,  46,  47,
		/*  48 */	'0', '1', '2', '3', '4', '5', '6', '7',
		/*  56 */	'8', '9',  58,  59,  60,  61,  62,  63,
		/*  64 */	 64, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
		/*  72 */	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		/*  80 */	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
		/*  88 */	'X', 'Y', 'Z',  91,  92,  93,  94,  95,
		/*  96 */	 96, 'A', 'B', 'C', 'D', 'E', 'F', 'G',
		/* 104 */	'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O',
		/* 112 */	'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W',
		/* 120 */	'X', 'Y', 'Z', 123, 124, 125, 126, 127,
		/* 128 */	128, 129, 130, 131, 132, 133, 134, 135,
		/* 136 */	136, 137, 138, 139, 140, 141, 142, 143,
		/* 144 */	144, 145, 146, 147, 148, 149, 150, 151,
		/* 152 */	152, 153, 154, 155, 156, 157, 158, 159,
		/* 160 */	160, 161, 162, 163, 164, 165, 166, 167,
		/* 168 */	168, 169, 170, 171, 172, 173, 174, 175,
		/* 176 */	176, 177, 178, 179, 180, 181, 182, 183,
		/* 184 */	184, 185, 186, 187, 188, 189, 190, 191,
		/* 192 */	192, 193, 194, 195, 196, 197, 198, 199,
		/* 200 */	200, 201, 202, 203, 204, 205, 206, 207,
		/* 208 */	208, 209, 210, 211, 212, 213, 214, 215,
		/* 216 */	216, 217, 218, 219, 220, 221, 222, 223,
		/* 224 */	192, 193, 194, 195, 196, 197, 198, 199,
		/* 232 */	200, 201, 202, 203, 204, 205, 206, 207,
		/* 240 */	208, 209, 210, 211, 212, 213, 214, 247,
		/* 248 */	216, 217, 218, 219, 220, 221, 222, 255,
	};
	test_ctype("de_DE.ISO8859-1", classInfos_de, toLowerMap_de, toUpperMap_de);

	const unsigned short int classInfos_utf8[257] = {
		/*  -1 */   0,	// neutral value
		/*   0 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/*   8 */	_IScntrl, _ISblank|_IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl|_ISspace, _IScntrl, _IScntrl,
		/*  16 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/*  24 */	_IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl, _IScntrl,
		/*  32 */	_ISblank|_ISspace|_ISprint, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  40 */	_ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  48 */	_ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph,
		/*  56 */	_ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISdigit|_ISxdigit|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  64 */	_ISpunct|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
		/*  72 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
		/*  80 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph,
		/*  88 */	_ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISupper|_ISalpha|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph,
		/*  96 */	_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISxdigit|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
		/* 104 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
		/* 112 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph,
		/* 120 */	_ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISalnum|_ISlower|_ISalpha|_ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _ISpunct|_ISprint|_ISgraph, _ISprint|_ISgraph, _IScntrl,
		/* 128 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 136 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 144 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 152 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 160 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 168 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 176 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 184 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 192 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 200 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 208 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 216 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 224 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 232 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 240 */	0, 0, 0, 0, 0, 0, 0, 0,
		/* 248 */	0, 0, 0, 0, 0, 0, 0, 0,
	};
	test_ctype("de_DE.UTF-8", classInfos_utf8, toLowerMap_posix,
		toUpperMap_posix);
}


// #pragma mark - wctype -------------------------------------------------------


unsigned short
determineWideFullClassInfo(int i)
{
	unsigned short classInfo = 0;

	if (iswblank(i))
		classInfo |= _ISblank;
	if (iswcntrl(i))
		classInfo |= _IScntrl;
	if (iswpunct(i))
		classInfo |= _ISpunct;
	if (iswalnum(i))
		classInfo |= _ISalnum;
	if (iswupper(i))
		classInfo |= _ISupper;
	if (iswlower(i))
		classInfo |= _ISlower;
	if (iswalpha(i))
		classInfo |= _ISalpha;
	if (iswdigit(i))
		classInfo |= _ISdigit;
	if (iswxdigit(i))
		classInfo |= _ISxdigit;
	if (iswspace(i))
		classInfo |= _ISspace;
	if (iswprint(i))
		classInfo |= _ISprint;
	if (iswgraph(i))
		classInfo |= _ISgraph;

	return classInfo;
}


void
test_wctype(const char* locale, const wchar_t* text,
	const unsigned short int wcs[], const unsigned short int classInfos[])
{
	setlocale(LC_CTYPE, locale);
	printf("wctype of %s locale\n", locale);

	int problemCount = 0;
	unsigned short classInfo = determineWideFullClassInfo(WEOF);
	if (classInfo != 0) {
		printf("\tPROBLEM: classinfo for WEOF = %x (expected 0)\n", classInfo);
		problemCount++;
	}
	wint_t wc = *text;
	for (int i = 0; i < 48; wc = *++text, ++i) {
		classInfo = determineWideFullClassInfo(wc);
		if (wc != wcs[i]) {
			printf("\tPROBLEM: wc for char #%d = %x (expected %x)\n", i, wc,
				wcs[i]);
			problemCount++;
		}

		if (classInfo != classInfos[i]) {
			printf("\tPROBLEM: classinfo for #%d = %x (expected %x)\n", i,
				classInfo, classInfos[i]);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_wctype()
{
	// haiku wide chars are always in UTF32, so nothing should change between
	// different locales

	const wchar_t* text = L"Hi there, how do you do? (äÜößáéúíó€'¤¹²$%#@) 12";

	const unsigned short int wcs[48] = {
		0x48, 0x69, 0x20, 0x74, 0x68, 0x65, 0x72, 0x65,
		0x2c, 0x20, 0x68, 0x6f, 0x77, 0x20, 0x64, 0x6f,
		0x20, 0x79, 0x6f, 0x75, 0x20, 0x64, 0x6f, 0x3f,
		0x20, 0x28, 0xe4, 0xdc, 0xf6, 0xdf, 0xe1, 0xe9,
		0xfa, 0xed, 0xf3, 0x20ac, 0x27, 0xa4, 0xb9, 0xb2,
		0x24, 0x25, 0x23, 0x40, 0x29, 0x20, 0x31, 0x32
	};
	const unsigned short int classInfos[48] = {
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISupper,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISspace|_ISprint|_ISblank,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower|_ISxdigit,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower|_ISxdigit,
		_ISprint|_ISgraph|_ISpunct,
		_ISspace|_ISprint|_ISblank,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISspace|_ISprint|_ISblank,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower|_ISxdigit,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISspace|_ISprint|_ISblank,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISspace|_ISprint|_ISblank,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower|_ISxdigit,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISpunct,
		_ISspace|_ISprint|_ISblank,
		_ISprint|_ISgraph|_ISpunct,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISupper,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph|_ISalpha|_ISalnum|_ISlower,
		_ISprint|_ISgraph,
		_ISprint|_ISgraph|_ISpunct,
		_ISprint|_ISgraph,
		_ISprint|_ISgraph,
		_ISprint|_ISgraph,
		_ISprint|_ISgraph,
		_ISpunct|_ISprint|_ISgraph,
		_ISpunct|_ISprint|_ISgraph,
		_ISpunct|_ISprint|_ISgraph,
		_ISpunct|_ISprint|_ISgraph,
		_ISspace|_ISprint|_ISblank,
		_ISprint|_ISgraph|_ISalnum|_ISdigit|_ISxdigit,
		_ISprint|_ISgraph|_ISalnum|_ISdigit|_ISxdigit
	};

	test_wctype("POSIX", text, wcs, classInfos);
	test_wctype("de_DE.ISO8859-1", text, wcs, classInfos);
	test_wctype("de_DE.ISO8859-15", text, wcs, classInfos);
	test_wctype("de_DE.UTF-8", text, wcs, classInfos);
}


// #pragma mark - wctrans ------------------------------------------------------


void
test_wctrans(const char* locale, const wchar_t* text, wctrans_t transition,
	const wchar_t* expectedResult)
{
	setlocale(LC_CTYPE, locale);
	printf("towctrans(%s) of %s locale\n",
		transition == _ISlower ? "tolower" : "toupper", locale);

	int problemCount = 0;
	wint_t wc = *text;
	for (int i = 0; wc != 0; wc = *++text, ++i) {
		errno = 0;
		wint_t result = towctrans(wc, transition);
		if (result != expectedResult[i] || errno != 0) {
			printf("\tPROBLEM: result for char #%d = %x (expected %x), "
					"errno = %x (expected %x)\n",
				i, result, expectedResult[i], errno, 0);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_wctrans()
{
	// haiku wide chars are always in UTF32, so nothing should change between
	// different locales

	setlocale(LC_CTYPE, "POSIX");
	printf("wctrans setup\n");

	int problemCount = 0;
	errno = 0;
	wctrans_t toU = wctrans("toupper");
	if (errno != 0 || toU != _ISupper) {
		printf("\tPROBLEM: wctrans(\"upper\") = %x (expected %x), "
				"errno=%x (expected %x)\n",
			toU, _ISupper, errno, 0);
		problemCount++;
	}
	errno = 0;
	wctrans_t toL = wctrans("tolower");
	if (errno != 0 || toL != _ISlower) {
		printf("\tPROBLEM: wctrans(\"lower\") = %x (expected %x), "
				"errno=%x (expected %x)\n",
			toL, _ISlower, errno, 0);
		problemCount++;
	}
	errno = 0;
	wctrans_t invalid1 = wctrans(NULL);
	if (errno != EINVAL || invalid1 != 0) {
		printf("\tPROBLEM: wctrans(NULL) = %x (expected %x), "
				"errno=%x (expected %x)\n",
			invalid1, 0, errno, EINVAL);
		problemCount++;
	}
	errno = 0;
	wctrans_t invalid2 = wctrans("invalid");
	if (errno != EINVAL || invalid2 != 0) {
		printf("\tPROBLEM: wctrans(\"invalid\") = %x (expected %x), "
				"errno=%x (expected %x)\n",
			invalid2, 0, errno, EINVAL);
		problemCount++;
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");

	const wchar_t* text = L"Hi there, how do you do? (äÜößáéúíó€'¤¹²$%#@) 12";
	const wchar_t* textU = L"HI THERE, HOW DO YOU DO? (ÄÜÖßÁÉÚÍÓ€'¤¹²$%#@) 12";
	const wchar_t* textL = L"hi there, how do you do? (äüößáéúíó€'¤¹²$%#@) 12";

	test_wctrans("POSIX", text, toU, textU);
	test_wctrans("de_DE.ISO8859-1", text, toU, textU);
	test_wctrans("de_DE.ISO8859-15", text, toU, textU);
	test_wctrans("de_DE.UTF-8", text, toU, textU);
	test_wctrans("fr_Fr", text, toU, textU);

	test_wctrans("POSIX", text, toL, textL);
	test_wctrans("de_DE.ISO8859-1", text, toL, textL);
	test_wctrans("de_DE.ISO8859-15", text, toL, textL);
	test_wctrans("de_DE.UTF-8", text, toL, textL);
	test_wctrans("fr_Fr", text, toL, textL);
}


// #pragma mark - wcwidth ------------------------------------------------------


void
test_wcwidth()
{
	setlocale(LC_ALL, "fr_FR.UTF-8");
	printf("wcwidth()\n");

	/* many of the following tests have been copied from gnulib */

	int problemCount = 0;
	int result = 0;

	/* Test width of ASCII characters.  */
	for (wchar_t wc = 0x20; wc < 0x7F; wc++) {
		result = wcwidth(wc);
		if (result != 1) {
			printf("\tPROBLEM: wcwidth(%x)=%x (expected %x)\n", wc, result, 1);
			problemCount++;
		}
	}

	struct {
		wchar_t wc;
		int result;
	} data[] = {
		{ 0x0, 0 },
		{ 0x1, -1 },
		{ 0x1F, -1 },
		{ 0x80, -1 },
		{ 0x9F, -1 },
		{ 0xA0, 1 },
		{ 0x0301, 0 },
		{ 0x05B0, 0 },
		{ 0x200E, 0 },
		{ 0x2060, 0 },
		{ 0xE0001, 0 },
		{ 0xE0044, 0 },
		{ 0x200B, 0 },
		{ 0xFEFF, 0 },
		{ 0x3000, 2 },
		{ 0xB250, 2 },
		{ 0xFF1A, 2 },
		{ 0x20369, 2 },
		{ 0x2F876, 2 },
		{ 0x0, 0 },
	};
	for (int i = 0; data[i].wc != 0 || i == 0; i++) {
		result = wcwidth(data[i].wc);
		if (result != data[i].result) {
			printf("\tPROBLEM: wcwidth(%x)=%x (expected %x)\n", data[i].wc,
				result, data[i].result);
			problemCount++;
		}
	}

	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


// #pragma mark - nl_langinfo --------------------------------------------------


void
test_langinfo(const char* locale, const char* langinfos[])
{
	setlocale(LC_ALL, locale);
	printf("langinfo of %s locale\n", locale);

	int problemCount = 0;
	for (int i = -1; langinfos[i + 1] != NULL; ++i) {
		const char* langinfo = nl_langinfo(i);
		if (strcmp(langinfo, langinfos[i + 1]) != 0) {
			printf("\tPROBLEM: langinfo for #%d = '%s' (expected '%s')\n", i,
				langinfo, langinfos[i + 1]);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_langinfo()
{
	const char* li_posix[] = {
		"", 	// out of bounds
		"US-ASCII",
		"%a %b %e %H:%M:%S %Y",
		"%m/%d/%y",
		"%H:%M:%S",
		"%I:%M:%S %p",
		"AM",
		"PM",

		"Sunday", "Monday", "Tuesday", "Wednesday",	"Thursday", "Friday",
		"Saturday",

		"Sun", "Mon", "Tue", "Wed",	"Thu", "Fri", "Sat",

		"January", "February", "March", "April", "May", "June",
		"July", "August", "September", "October", "November", "December",

		"Jan", "Feb", "Mar", "Apr", "May", "Jun",
		"Jul", "Aug", "Sep", "Oct", "Nov", "Dec",

		"%EC, %Ey, %EY",
		"%Ex",
		"%Ec",
		"%EX",
		"%O",

		".",
		"",

		"^[yY]",
		"^[nN]",

		"",

		"", 	// out of bounds
		NULL
	};
	test_langinfo("POSIX", li_posix);

	const char* li_de[] = {
		"", 	// out of bounds
		"UTF-8",
		"%A, %e. %B %Y %H:%M:%S %Z",
		"%d.%m.%Y",
		"%H:%M:%S",
		"%I:%M:%S %p",
		"vorm.",
		"nachm.",

		"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag",
		"Samstag",

		"So.", "Mo.", "Di.", "Mi.", "Do.", "Fr.", "Sa.",

		"Januar", "Februar", "März", "April", "Mai", "Juni",
		"Juli", "August", "September", "Oktober", "November", "Dezember",

		"Jan", "Feb", "Mär", "Apr", "Mai", "Jun",
		"Jul", "Aug", "Sep", "Okt", "Nov", "Dez",

		"%EC, %Ey, %EY",
		"%Ex",
		"%Ec",
		"%EX",
		"%O",

		",",
		".",

		"^[yY]",
		"^[nN]",

		"€",

		"", 	// out of bounds
		NULL,
	};
	test_langinfo("de_DE.UTF-8", li_de);

	const char* li_de_iso[] = {
		"", 	// out of bounds
		"ISO8859-15",
		"%A, %e. %B %Y %H:%M:%S %Z",
		"%d.%m.%Y",
		"%H:%M:%S",
		"%I:%M:%S %p",
		"vorm.",
		"nachm.",

		"Sonntag", "Montag", "Dienstag", "Mittwoch", "Donnerstag", "Freitag",
		"Samstag",

		"So.", "Mo.", "Di.", "Mi.", "Do.", "Fr.", "Sa.",

		"Januar", "Februar", "M\xE4rz", "April", "Mai", "Juni",
		"Juli", "August", "September", "Oktober", "November", "Dezember",

		"Jan", "Feb", "M\xE4r", "Apr", "Mai", "Jun",
		"Jul", "Aug", "Sep", "Okt", "Nov", "Dez",

		"%EC, %Ey, %EY",
		"%Ex",
		"%Ec",
		"%EX",
		"%O",

		",",
		".",

		"^[yY]",
		"^[nN]",

		"\xA4",

		"", 	// out of bounds
		NULL
	};
	test_langinfo("de_DE.ISO8859-15", li_de_iso);
}


// #pragma mark - collation ----------------------------------------------------


struct coll_data {
	const char* a;
	const char* b;
	int result;
	int err;
};


static int sign (int a)
{
	if (a < 0)
		return -1;
	if (a > 0)
		return 1;
	return 0;
}


void
test_coll(bool useStrxfrm, const char* locale, const coll_data coll[])
{
	setlocale(LC_COLLATE, locale);
	printf("%s in %s locale\n", useStrxfrm ? "strxfrm" : "strcoll", locale);

	int problemCount = 0;
	for (unsigned int i = 0; i < sizeof(coll) / sizeof(coll_data); ++i) {
		errno = 0;
		int result;
		char funcCall[100];
		if (useStrxfrm) {
			char sortKeyA[100], sortKeyB[100];
			strxfrm(sortKeyA, coll[i].a, 100);
			strxfrm(sortKeyB, coll[i].b, 100);
			result = sign(strcmp(sortKeyA, sortKeyB));
			sprintf(funcCall, "strcmp(strxfrm(\"%s\"), strxfrm(\"%s\"))",
				coll[i].a, coll[i].b);
		} else {
			result = sign(strcoll(coll[i].a, coll[i].b));
			sprintf(funcCall, "strcoll(\"%s\", \"%s\")", coll[i].a, coll[i].b);
		}

		if (result != coll[i].result || errno != coll[i].err) {
			printf(
				"\tPROBLEM: %s = %d (expected %d), errno = %x (expected %x)\n",
				funcCall, result, coll[i].result, errno, coll[i].err);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_collation()
{
	const coll_data coll_posix[] = {
		{ "", "", 0, 0 },
		{ "test", "test", 0, 0 },
		{ "tester", "test", 1, 0 },
		{ "tEst", "teSt", -1, 0 },
		{ "test", "tester", -1, 0 },
		{ "tast", "täst", -1, EINVAL },
		{ "tæst", "test", 1, EINVAL },
	};
	test_coll(0, "POSIX", coll_posix);
	test_coll(1, "POSIX", coll_posix);

	const coll_data coll_en[] = {
		{ "", "", 0, 0 },
		{ "test", "test", 0, 0 },
		{ "tester", "test", 1, 0 },
		{ "tEst", "test", 1, 0 },
		{ "test", "tester", -1, 0 },
		{ "täst", "täst", 0, 0 },
		{ "tast", "täst", -1, 0 },
		{ "tbst", "täst", 1, 0 },
		{ "tbst", "tæst", 1, 0 },
		{ "täst", "tÄst", -1, 0 },
		{ "tBst", "tÄst", 1, 0 },
		{ "tBst", "täst", 1, 0 },
		{ "taest", "tæst", -1, 0 },
		{ "tafst", "tæst", 1, 0 },
		{ "taa", "täa", -1, 0 },
		{ "tab", "täb", -1, 0 },
		{ "tad", "täd", -1, 0 },
		{ "tae", "täe", -1, 0 },
		{ "taf", "täf", -1, 0 },
		{ "cote", "coté", -1, 0 },
		{ "coté", "côte", -1, 0 },
		{ "côte", "côté", -1, 0 },
	};
	test_coll(0, "en_US.UTF-8", coll_en);
	test_coll(1, "en_US.UTF-8", coll_en);

	const coll_data coll_de[] = {
		{ "", "", 0, 0 },
		{ "test", "test", 0, 0 },
		{ "tester", "test", 1, 0 },
		{ "tEst", "test", 1, 0 },
		{ "test", "tester", -1, 0 },
		{ "täst", "täst", 0, 0 },
		{ "tast", "täst", -1, 0 },
		{ "tbst", "täst", 1, 0 },
		{ "tbst", "tæst", 1, 0 },
		{ "täst", "tÄst", -1, 0 },
		{ "tBst", "tÄst", 1, 0 },
		{ "tBst", "täst", 1, 0 },
		{ "taest", "tæst", -1, 0 },
		{ "tafst", "tæst", 1, 0 },
		{ "taa", "tä", 1, 0 },
		{ "tab", "tä", 1, 0 },
		{ "tad", "tä", 1, 0 },
		{ "tae", "tä", 1, 0 },
		{ "taf", "tä", 1, 0 },
		{ "cote", "coté", -1, 0 },
		{ "coté", "côte", -1, 0 },
		{ "côte", "côté", -1, 0 },
	};
	test_coll(0, "de_DE.UTF-8", coll_de);
	test_coll(1, "de_DE.UTF-8", coll_de);

	const coll_data coll_de_phonebook[] = {
		{ "", "", 0, 0 },
		{ "test", "test", 0, 0 },
		{ "tester", "test", 1, 0 },
		{ "tEst", "test", 1, 0 },
		{ "test", "tester", -1, 0 },
		{ "täst", "täst", 0, 0 },
		{ "tast", "täst", 1, 0 },
		{ "tbst", "täst", 1, 0 },
		{ "tbst", "tæst", 1, 0 },
		{ "täst", "tÄst", -1, 0 },
		{ "tBst", "tÄst", 1, 0 },
		{ "tBst", "täst", 1, 0 },
		{ "taest", "tæst", -1, 0 },
		{ "tafst", "tæst", 1, 0 },
		{ "taa", "tä", -1, 0 },
		{ "tab", "tä", -1, 0 },
		{ "tad", "tä", -1, 0 },
		{ "tae", "tä", -1, 0 },
		{ "taf", "tä", 1, 0 },
		{ "cote", "coté", -1, 0 },
		{ "coté", "côte", -1, 0 },
		{ "côte", "côté", -1, 0 },
	};
	test_coll(0, "de_DE.UTF-8@collation=phonebook", coll_de_phonebook);
	test_coll(1, "de_DE.UTF-8@collation=phonebook", coll_de_phonebook);

	const coll_data coll_fr[] = {
		{ "", "", 0, 0 },
		{ "test", "test", 0, 0 },
		{ "tester", "test", 1, 0 },
		{ "tEst", "test", 1, 0 },
		{ "test", "tester", -1, 0 },
		{ "täst", "täst", 0, 0 },
		{ "tast", "täst", -1, 0 },
		{ "tbst", "täst", 1, 0 },
		{ "tbst", "tæst", 1, 0 },
		{ "täst", "tÄst", -1, 0 },
		{ "tBst", "tÄst", 1, 0 },
		{ "tBst", "täst", 1, 0 },
		{ "taest", "tæst", -1, 0 },
		{ "tafst", "tæst", 1, 0 },
		{ "taa", "tä", 1, 0 },
		{ "tab", "tä", 1, 0 },
		{ "tad", "tä", 1, 0 },
		{ "tae", "tä", 1, 0 },
		{ "taf", "tä", 1, 0 },
		{ "cote", "coté", -1, 0 },
		{ "coté", "côte", 1, 0 },
		{ "côte", "côté", -1, 0 },
	};
	test_coll(0, "fr_FR.UTF-8", coll_fr);
	test_coll(1, "fr_FR.UTF-8", coll_fr);
}


// #pragma mark - time conversion ----------------------------------------------


void
test_localtime(const char* tz, time_t nowSecs, const tm& expected)
{
	setenv("TZ", tz, 1);
	printf("localtime for '%s'\n", tz);

	tm now;
	tm* result = localtime_r(&nowSecs, &now);
	int problemCount = 0;
	if (result == NULL) {
		printf("\tPROBLEM: localtime(\"%ld\") = NULL\n", nowSecs);
		problemCount++;
	}
	if (now.tm_year != expected.tm_year) {
		printf("\tPROBLEM: localtime().tm_year = %d (expected %d)\n",
			now.tm_year, expected.tm_year);
		problemCount++;
	}
	if (now.tm_mon != expected.tm_mon) {
		printf("\tPROBLEM: localtime().tm_mon = %d (expected %d)\n",
			now.tm_mon, expected.tm_mon);
		problemCount++;
	}
	if (now.tm_mday != expected.tm_mday) {
		printf("\tPROBLEM: localtime().tm_mday = %d (expected %d)\n",
			now.tm_mday, expected.tm_mday);
		problemCount++;
	}
	if (now.tm_hour != expected.tm_hour) {
		printf("\tPROBLEM: localtime().tm_hour = %d (expected %d)\n",
			now.tm_hour, expected.tm_hour);
		problemCount++;
	}
	if (now.tm_min != expected.tm_min) {
		printf("\tPROBLEM: localtime().tm_min = %d (expected %d)\n",
			now.tm_min, expected.tm_min);
		problemCount++;
	}
	if (now.tm_sec != expected.tm_sec) {
		printf("\tPROBLEM: localtime().tm_sec = %d (expected %d)\n",
			now.tm_sec, expected.tm_sec);
		problemCount++;
	}
	if (now.tm_wday != expected.tm_wday) {
		printf("\tPROBLEM: localtime().tm_wday = %d (expected %d)\n",
			now.tm_wday, expected.tm_wday);
		problemCount++;
	}
	if (now.tm_yday != expected.tm_yday) {
		printf("\tPROBLEM: localtime().tm_yday = %d (expected %d)\n",
			now.tm_yday, expected.tm_yday);
		problemCount++;
	}
	if (now.tm_isdst != expected.tm_isdst) {
		printf("\tPROBLEM: localtime().tm_isdst = %d (expected %d)\n",
			now.tm_isdst, expected.tm_isdst);
		problemCount++;
	}
	if (now.tm_gmtoff != expected.tm_gmtoff) {
		printf("\tPROBLEM: localtime().tm_gmtoff = %d (expected %d)\n",
			now.tm_gmtoff, expected.tm_gmtoff);
		problemCount++;
	}
	if (strcasecmp(now.tm_zone, expected.tm_zone) != 0) {
		printf("\tPROBLEM: localtime().tm_zone = '%s' (expected '%s')\n",
			now.tm_zone, expected.tm_zone);
		problemCount++;
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_gmtime(const char* tz, time_t nowSecs, const tm& expected)
{
	setenv("TZ", tz, 1);
	printf("gmtime for '%s'\n", tz);

	tm now;
	tm* result = gmtime_r(&nowSecs, &now);
	int problemCount = 0;
	if (result == NULL) {
		printf("\tPROBLEM: localtime(\"%ld\") = NULL\n", nowSecs);
		problemCount++;
	}
	if (now.tm_year != expected.tm_year) {
		printf("\tPROBLEM: localtime().tm_year = %d (expected %d)\n",
			now.tm_year, expected.tm_year);
		problemCount++;
	}
	if (now.tm_mon != expected.tm_mon) {
		printf("\tPROBLEM: localtime().tm_mon = %d (expected %d)\n",
			now.tm_mon, expected.tm_mon);
		problemCount++;
	}
	if (now.tm_mday != expected.tm_mday) {
		printf("\tPROBLEM: localtime().tm_mday = %d (expected %d)\n",
			now.tm_mday, expected.tm_mday);
		problemCount++;
	}
	if (now.tm_hour != expected.tm_hour) {
		printf("\tPROBLEM: localtime().tm_hour = %d (expected %d)\n",
			now.tm_hour, expected.tm_hour);
		problemCount++;
	}
	if (now.tm_min != expected.tm_min) {
		printf("\tPROBLEM: localtime().tm_min = %d (expected %d)\n",
			now.tm_min, expected.tm_min);
		problemCount++;
	}
	if (now.tm_sec != expected.tm_sec) {
		printf("\tPROBLEM: localtime().tm_sec = %d (expected %d)\n",
			now.tm_sec, expected.tm_sec);
		problemCount++;
	}
	if (now.tm_wday != expected.tm_wday) {
		printf("\tPROBLEM: localtime().tm_wday = %d (expected %d)\n",
			now.tm_wday, expected.tm_wday);
		problemCount++;
	}
	if (now.tm_yday != expected.tm_yday) {
		printf("\tPROBLEM: localtime().tm_yday = %d (expected %d)\n",
			now.tm_yday, expected.tm_yday);
		problemCount++;
	}
	if (now.tm_isdst != expected.tm_isdst) {
		printf("\tPROBLEM: localtime().tm_isdst = %d (expected %d)\n",
			now.tm_isdst, expected.tm_isdst);
		problemCount++;
	}
	if (now.tm_gmtoff != expected.tm_gmtoff) {
		printf("\tPROBLEM: localtime().tm_gmtoff = %d (expected %d)\n",
			now.tm_gmtoff, expected.tm_gmtoff);
		problemCount++;
	}
	if (strcasecmp(now.tm_zone, expected.tm_zone) != 0) {
		printf("\tPROBLEM: localtime().tm_zone = '%s' (expected '%s')\n",
			now.tm_zone, expected.tm_zone);
		problemCount++;
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_mktime(const char* tz, tm& tm, time_t expected, int expectedWeekDay,
	int expectedYearDay)
{
	setenv("TZ", tz, 1);
	printf("mktime for '%s'\n", tz);

	time_t result = mktime(&tm);
	int problemCount = 0;
	if (result != expected) {
		printf("\tPROBLEM: mktime() = %ld (expected %ld)\n", result, expected);
		problemCount++;
	}
	if (tm.tm_wday != expectedWeekDay) {
		printf("\tPROBLEM: mktime().tm_wday = %d (expected %d)\n",
			tm.tm_wday, expectedWeekDay);
		problemCount++;
	}
	if (tm.tm_yday != expectedYearDay) {
		printf("\tPROBLEM: mktime().tm_yday = %d (expected %d)\n",
			tm.tm_yday, expectedYearDay);
		problemCount++;
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_timeconversions()
{
	setlocale(LC_ALL, "en_US");
	{
		time_t testTime = 1279391169;	// Sat Jul 17 18:26:09 GMT 2010
		tm gtm = {
			9, 26, 18, 17, 6, 110, 6, 197, 0, 0, (char*)"GMT"
		};
		test_localtime("GMT", testTime, gtm);
		test_gmtime("GMT", testTime, gtm);
		gtm.tm_wday = -1;
		gtm.tm_yday = -1;
		test_mktime("GMT", gtm, testTime, 6, 197);

		tm gtmplus2 = {
			9, 26, 16, 17, 6, 110, 6, 197, 0, -2 * 3600, (char*)"GMT+2"
		};
		test_localtime("GMT+2", testTime, gtmplus2);
		test_gmtime("GMT+2", testTime, gtm);
		gtmplus2.tm_wday = -1;
		gtmplus2.tm_yday = -1;
		test_mktime("GMT+2", gtmplus2, testTime, 6, 197);

		tm gtmminus2 = {
			9, 26, 20, 17, 6, 110, 6, 197, 0, 2 * 3600, (char*)"GMT-2"
		};
		test_localtime("GMT-2", testTime, gtmminus2);
		test_gmtime("GMT-2", testTime, gtm);
		gtmminus2.tm_wday = -1;
		gtmminus2.tm_yday = -1;
		test_mktime("GMT-2", gtmminus2, testTime, 6, 197);

		tm btm = {
			9, 26, 20, 17, 6, 110, 6, 197, 1, 2 * 3600, (char*)"CEST"
		};
		test_localtime(":Europe/Berlin", testTime, btm);
		test_gmtime(":Europe/Berlin", testTime, gtm);
		btm.tm_wday = -1;
		btm.tm_yday = -1;
		test_mktime(":Europe/Berlin", btm, testTime, 6, 197);

		tm ctm = {
			9, 26, 20, 17, 6, 110, 6, 197, 1, 2 * 3600, (char*)"CEST"
		};
		test_localtime("CET", testTime, ctm);
		test_gmtime("CET", testTime, gtm);
		ctm.tm_wday = -1;
		ctm.tm_yday = -1;
		test_mktime("CET", ctm, testTime, 6, 197);

		tm latm = {
			9, 26, 11, 17, 6, 110, 6, 197, 1, -7 * 3600, (char*)"PDT"
		};
		test_localtime(":America/Los_Angeles", testTime, latm);
		test_gmtime(":America/Los_Angeles", testTime, gtm);
		latm.tm_wday = -1;
		latm.tm_yday = -1;
		test_mktime(":America/Los_Angeles", latm, testTime, 6, 197);

		tm ttm = {
			9, 26, 3, 18, 6, 110, 0, 198, 0, 9 * 3600, (char*)"GMT+09:00"
		};
		test_localtime(":Asia/Tokyo", testTime, ttm);
		test_gmtime(":Asia/Tokyo", testTime, gtm);
		ttm.tm_wday = -1;
		ttm.tm_yday = -1;
		test_mktime(":Asia/Tokyo", ttm, testTime, 0, 198);
	}

	{
		time_t testTime = 1268159169;	// Tue Mar 9 18:26:09 GMT 2010
		tm gtm = {
			9, 26, 18, 9, 2, 110, 2, 67, 0, 0, (char*)"GMT"
		};
		test_localtime("GMT", testTime, gtm);
		test_gmtime("GMT", testTime, gtm);
		gtm.tm_wday = -1;
		gtm.tm_yday = -1;
		test_mktime("GMT", gtm, testTime, 2, 67);

		tm btm = {
			9, 26, 19, 9, 2, 110, 2, 67, 0, 3600, (char*)"CET"
		};
		test_localtime(":Europe/Berlin", testTime, btm);
		test_gmtime(":Europe/Berlin", testTime, gtm);
		btm.tm_wday = -1;
		btm.tm_yday = -1;
		test_mktime(":Europe/Berlin", btm, testTime, 2, 67);

		tm ctm = {
			9, 26, 19, 9, 2, 110, 2, 67, 0, 3600, (char*)"CET"
		};
		test_localtime("CET", testTime, ctm);
		test_gmtime("CET", testTime, gtm);
		ctm.tm_wday = -1;
		ctm.tm_yday = -1;
		test_mktime("CET", ctm, testTime, 2, 67);

		tm latm = {
			9, 26, 10, 9, 2, 110, 2, 67, 0, -8 * 3600, (char*)"PST"
		};
		test_localtime(":America/Los_Angeles", testTime, latm);
		test_gmtime(":America/Los_Angeles", testTime, gtm);
		latm.tm_wday = -1;
		latm.tm_yday = -1;
		test_mktime(":America/Los_Angeles", latm, testTime, 2, 67);

		tm ttm = {
			9, 26, 3, 10, 2, 110, 3, 68, 0, 9 * 3600, (char*)"GMT+09:00"
		};
		test_localtime(":Asia/Tokyo", testTime, ttm);
		test_gmtime(":Asia/Tokyo", testTime, gtm);
		ttm.tm_wday = -1;
		ttm.tm_yday = -1;
		test_mktime(":Asia/Tokyo", ttm, testTime, 3, 68);
	}

	{
		time_t testTime = 0;	// Thu Jan 1 00:00:00 GMT 1970
		tm gtm = {
			0, 0, 0, 1, 0, 70, 4, 0, 0, 0, (char*)"GMT"
		};
		test_localtime("GMT", testTime, gtm);
		test_gmtime("GMT", testTime, gtm);
		gtm.tm_wday = -1;
		gtm.tm_yday = -1;
		test_mktime("GMT", gtm, testTime, 4, 0);

		tm btm = {
			0, 0, 1, 1, 0, 70, 4, 0, 0, 1 * 3600, (char*)"CET"
		};
		test_localtime(":Europe/Berlin", testTime, btm);
		test_gmtime(":Europe/Berlin", testTime, gtm);
		btm.tm_wday = -1;
		btm.tm_yday = -1;
		test_mktime(":Europe/Berlin", btm, testTime, 4, 0);

		tm ctm = {
			0, 0, 1, 1, 0, 70, 4, 0, 0, 1 * 3600, (char*)"CET"
		};
		test_localtime("CET", testTime, ctm);
		test_gmtime("CET", testTime, gtm);
		ctm.tm_wday = -1;
		ctm.tm_yday = -1;
		test_mktime("CET", ctm, testTime, 4, 0);

		tm latm = {
			0, 0, 16, 31, 11, 69, 3, 364, 0, -8 * 3600, (char*)"PST"
		};
		test_localtime(":America/Los_Angeles", testTime, latm);
		test_gmtime(":America/Los_Angeles", testTime, gtm);
		latm.tm_wday = -1;
		latm.tm_yday = -1;
		test_mktime(":America/Los_Angeles", latm, testTime, 3, 364);

		tm ttm = {
			0, 0, 9, 1, 0, 70, 4, 0, 0, 9 * 3600, (char*)"GMT+09:00"
		};
		test_localtime(":Asia/Tokyo", testTime, ttm);
		test_gmtime(":Asia/Tokyo", testTime, gtm);
		ttm.tm_wday = -1;
		ttm.tm_yday = -1;
		test_mktime(":Asia/Tokyo", ttm, testTime, 4, 0);
	}
}


// #pragma mark - printf -------------------------------------------------------


struct sprintf_data {
	const char* format;
	double value;
	const char* result;
};


void
test_sprintf(const char* locale, const sprintf_data data[])
{
	setlocale(LC_ALL, locale);
	printf("sprintf for '%s'\n", locale);

	int problemCount = 0;
	for(int i = 0; data[i].format != NULL; ++i) {
		char buf[100];
		if (strchr(data[i].format, 'd') != NULL)
			sprintf(buf, data[i].format, (int)data[i].value);
		else if (strchr(data[i].format, 'f') != NULL)
			sprintf(buf, data[i].format, data[i].value);
		if (strcmp(buf, data[i].result) != 0) {
			printf("\tPROBLEM: sprintf(\"%s\") = \"%s\" (expected \"%s\")\n",
				data[i].format, buf, data[i].result);
			problemCount++;
		}
	}
	if (problemCount)
		printf("\t%d problem(s) found!\n", problemCount);
	else
		printf("\tall fine\n");
}


void
test_sprintf()
{
	const sprintf_data sprintf_posix[] = {
		{ "%d", 123, "123" },
		{ "%d", -123, "-123" },
		{ "%d", 123456, "123456" },
		{ "%'d", 123456, "123456" },
		{ "%f", 123, "123.000000" },
		{ "%f", -123, "-123.000000" },
		{ "%.2f", 123456.789, "123456.79" },
		{ "%'.2f", 123456.789, "123456.79" },
		{ NULL, 0.0, NULL }
	};
	test_sprintf("POSIX", sprintf_posix);

	const sprintf_data sprintf_de[] = {
		{ "%d", 123, "123" },
		{ "%d", -123, "-123" },
		{ "%d", 123456, "123456" },
		{ "%'d", 123456, "123.456" },
		{ "%f", 123, "123,000000" },
		{ "%f", -123, "-123,000000" },
		{ "%.2f", 123456.789, "123456,79" },
		{ "%'.2f", 123456.789, "123.456,79" },
		{ NULL, 0.0, NULL }
	};
	test_sprintf("de_DE.UTF-8", sprintf_de);

	const sprintf_data sprintf_gu[] = {
		{ "%d", 123, "123" },
		{ "%d", -123, "-123" },
		{ "%d", 123456, "123456" },
		{ "%'d", 123456, "123,456" },
		{ "%f", 123, "123.000000" },
		{ "%f", -123, "-123.000000" },
		{ "%.2f", 123456.789, "123456.79" },
		{ "%'.2f", 123456.789, "1,23,456.79" },
		{ NULL, 0.0, NULL }
	};
	test_sprintf("gu_IN", sprintf_gu);

	const sprintf_data sprintf_nb[] = {
		{ "%d", 123, "123" },
		{ "%d", -123, "-123" },
		{ "%d", 123456, "123456" },
		{ "%'d", 123456, "123 456" },
		{ "%f", 123, "123,000000" },
		{ "%f", -123, "-123,000000" },
		{ "%.2f", 123456.789, "123456,79" },
		{ "%'.2f", 123456.789, "123 456,79" },
		{ NULL, 0.0, NULL }
	};
	test_sprintf("nb_NO", sprintf_nb);
}


// #pragma mark - main ---------------------------------------------------------


/*
 * Test several different aspects of the POSIX locale and the functions
 * influenced by it.
 */
int
main(void)
{
	test_setlocale();
	test_localeconv();
	test_strftime();
	test_ctype();
	test_wctype();
	test_wctrans();
	test_wcwidth();
	test_langinfo();
	test_collation();
	test_timeconversions();
	test_sprintf();

	return 0;
}
