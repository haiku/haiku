/* glibc_test():
 Tests of fseek and fseeko.
 Copyright (C) 2000, 2001, 2002 Free Software Foundation, Inc.
 This file is part of the GNU C Library.
 Contributed by Ulrich Drepper <drepper@redhat.com>, 2000.
 */

/* gnulib_test_fseek() is inspired by gnulib's test-fseek.c and test-ftell.c:
 Copyright (C) 2007, 2008, 2009, 2010 Free Software Foundation, Inc.
 */


#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>


void
error(int status, int errorNum, const char* errorText)
{
	fprintf(stderr, "%s (errno: %x)\n", errorText, errorNum);
	exit(status);
}


int
glibc_test(void)
{
	const char *tmpdir;
	char *fname;
	int fd;
	FILE *fp;
	const char outstr[] = "hello world!\n";
	char strbuf[sizeof outstr];
	char buf[200];
	struct stat st1;
	struct stat st2;
	int result = 0;

	tmpdir = getenv("TMPDIR");
	if (tmpdir == NULL || tmpdir[0] == '\0')
		tmpdir = "/tmp";

	asprintf(&fname, "%s/tst-fseek.XXXXXX", tmpdir);
	if (fname == NULL)
		error(EXIT_FAILURE, errno, "cannot generate name for temporary file");

	/* Create a temporary file.   */
	fd = mkstemp(fname);
	if (fd == -1)
		error(EXIT_FAILURE, errno, "cannot open temporary file");

	fp = fdopen(fd, "w+");
	if (fp == NULL)
		error(EXIT_FAILURE, errno, "cannot get FILE for temporary file");

	setbuffer(fp, strbuf, sizeof(outstr) - 1);

	if (fwrite(outstr, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: write error\n", __LINE__);
		result = 1;
		goto out;
	}

	/* The EOF flag must be reset.  */
	if (fgetc(fp) != EOF) {
		printf("%d: managed to read at end of file\n", __LINE__);
		result = 1;
	} else if (!feof(fp)) {
		printf("%d: EOF flag not set\n", __LINE__);
		result = 1;
	}
	if (fseek(fp, 0, SEEK_CUR) != 0) {
		printf("%d: fseek(fp, 0, SEEK_CUR) failed\n", __LINE__);
		result = 1;
	} else if (feof(fp)) {
		printf("%d: fseek() didn't reset EOF flag\n", __LINE__);
		result = 1;
	}

	/* Do the same for fseeko().  */
	if (fgetc(fp) != EOF) {
		printf("%d: managed to read at end of file\n", __LINE__);
		result = 1;
	} else if (!feof(fp)) {
		printf("%d: EOF flag not set\n", __LINE__);
		result = 1;
	}
	if (fseeko(fp, 0, SEEK_CUR) != 0) {
		printf("%d: fseek(fp, 0, SEEK_CUR) failed\n", __LINE__);
		result = 1;
	} else if (feof(fp)) {
		printf("%d: fseek() didn't reset EOF flag\n", __LINE__);
		result = 1;
	}

	/* Go back to the beginning of the file: absolute.  */
	if (fseek(fp, 0, SEEK_SET) != 0) {
		printf("%d: fseek(fp, 0, SEEK_SET) failed\n", __LINE__);
		result = 1;
	} else if (fflush(fp) != 0) {
		printf("%d: fflush() failed\n", __LINE__);
		result = 1;
	} else if (lseek(fd, 0, SEEK_CUR) != 0) {
		printf("%d: lseek() returned different position\n", __LINE__);
		result = 1;
	} else if (fread(buf, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: fread() failed\n", __LINE__);
		result = 1;
	} else if (memcmp(buf, outstr, sizeof(outstr) - 1) != 0) {
		printf("%d: content after fseek(,,SEEK_SET) wrong\n", __LINE__);
		result = 1;
	}

	/* Now with fseeko.  */
	if (fseeko(fp, 0, SEEK_SET) != 0) {
		printf("%d: fseeko(fp, 0, SEEK_SET) failed\n", __LINE__);
		result = 1;
	} else if (fflush(fp) != 0) {
		printf("%d: fflush() failed\n", __LINE__);
		result = 1;
	} else if (lseek(fd, 0, SEEK_CUR) != 0) {
		printf("%d: lseek() returned different position\n", __LINE__);
		result = 1;
	} else if (fread(buf, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: fread() failed\n", __LINE__);
		result = 1;
	} else if (memcmp(buf, outstr, sizeof(outstr) - 1) != 0) {
		printf("%d: content after fseeko(,,SEEK_SET) wrong\n", __LINE__);
		result = 1;
	}

	/* Go back to the beginning of the file: relative.  */
	if (fseek(fp, -((int) sizeof(outstr) - 1), SEEK_CUR) != 0) {
		printf("%d: fseek(fp, 0, SEEK_SET) failed\n", __LINE__);
		result = 1;
	} else if (fflush(fp) != 0) {
		printf("%d: fflush() failed\n", __LINE__);
		result = 1;
	} else if (lseek(fd, 0, SEEK_CUR) != 0) {
		printf("%d: lseek() returned different position\n", __LINE__);
		result = 1;
	} else if (fread(buf, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: fread() failed\n", __LINE__);
		result = 1;
	} else if (memcmp(buf, outstr, sizeof(outstr) - 1) != 0) {
		printf("%d: content after fseek(,,SEEK_SET) wrong\n", __LINE__);
		result = 1;
	}

	/* Now with fseeko.  */
	if (fseeko(fp, -((int) sizeof(outstr) - 1), SEEK_CUR) != 0) {
		printf("%d: fseeko(fp, 0, SEEK_SET) failed\n", __LINE__);
		result = 1;
	} else if (fflush(fp) != 0) {
		printf("%d: fflush() failed\n", __LINE__);
		result = 1;
	} else if (lseek(fd, 0, SEEK_CUR) != 0) {
		printf("%d: lseek() returned different position\n", __LINE__);
		result = 1;
	} else if (fread(buf, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: fread() failed\n", __LINE__);
		result = 1;
	} else if (memcmp(buf, outstr, sizeof(outstr) - 1) != 0) {
		printf("%d: content after fseeko(,,SEEK_SET) wrong\n", __LINE__);
		result = 1;
	}

	/* Go back to the beginning of the file: from the end.  */
	if (fseek(fp, -((int) sizeof(outstr) - 1), SEEK_END) != 0) {
		printf("%d: fseek(fp, 0, SEEK_SET) failed\n", __LINE__);
		result = 1;
	} else if (fflush(fp) != 0) {
		printf("%d: fflush() failed\n", __LINE__);
		result = 1;
	} else if (lseek(fd, 0, SEEK_CUR) != 0) {
		printf("%d: lseek() returned different position\n", __LINE__);
		result = 1;
	} else if (fread(buf, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: fread() failed\n", __LINE__);
		result = 1;
	} else if (memcmp(buf, outstr, sizeof(outstr) - 1) != 0) {
		printf("%d: content after fseek(,,SEEK_SET) wrong\n", __LINE__);
		result = 1;
	}

	/* Now with fseeko.  */
	if (fseeko(fp, -((int) sizeof(outstr) - 1), SEEK_END) != 0) {
		printf("%d: fseeko(fp, 0, SEEK_SET) failed\n", __LINE__);
		result = 1;
	} else if (fflush(fp) != 0) {
		printf("%d: fflush() failed\n", __LINE__);
		result = 1;
	} else if (lseek(fd, 0, SEEK_CUR) != 0) {
		printf("%d: lseek() returned different position\n", __LINE__);
		result = 1;
	} else if (fread(buf, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: fread() failed\n", __LINE__);
		result = 1;
	} else if (memcmp(buf, outstr, sizeof(outstr) - 1) != 0) {
		printf("%d: content after fseeko(,,SEEK_SET) wrong\n", __LINE__);
		result = 1;
	}

	if (fwrite(outstr, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: write error 2\n", __LINE__);
		result = 1;
		goto out;
	}

	if (fwrite(outstr, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: write error 3\n", __LINE__);
		result = 1;
		goto out;
	}

	if (fwrite(outstr, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: write error 4\n", __LINE__);
		result = 1;
		goto out;
	}

	if (fwrite(outstr, sizeof(outstr) - 1, 1, fp) != 1) {
		printf("%d: write error 5\n", __LINE__);
		result = 1;
		goto out;
	}

	if (fputc('1', fp) == EOF || fputc('2', fp) == EOF) {
		printf("%d: cannot add characters at the end\n", __LINE__);
		result = 1;
		goto out;
	}

	/* Check the access time.  */
	if (fstat(fd, &st1) < 0) {
		printf("%d: fstat() before fseeko() failed\n\n", __LINE__);
		result = 1;
	} else {
		sleep(1);

		if (fseek(fp, -(2 + 2 * (sizeof(outstr) - 1)), SEEK_CUR) != 0) {
			printf("%d: fseek() after write characters failed\n", __LINE__);
			result = 1;
			goto out;
		} else {
			time_t t;
			/* Make sure the timestamp actually can be different.  */
			sleep(1);
			t = time(NULL);

			if (fstat(fd, &st2) < 0) {
				printf("%d: fstat() after fseeko() failed\n\n", __LINE__);
				result = 1;
			}
			if (st1.st_ctime >= t) {
				printf("%d: st_ctime not updated\n", __LINE__);
				result = 1;
			}
			if (st1.st_mtime >= t) {
				printf("%d: st_mtime not updated\n", __LINE__);
				result = 1;
			}
			if (st1.st_ctime >= st2.st_ctime) {
				printf("%d: st_ctime not changed\n", __LINE__);
				result = 1;
			}
			if (st1.st_mtime >= st2.st_mtime) {
				printf("%d: st_mtime not changed\n", __LINE__);
				result = 1;
			}
		}
	}

	if (fread(buf, 1, 2 + 2 * (sizeof(outstr) - 1), fp) != 2 + 2
		* (sizeof(outstr) - 1)) {
		printf("%d: reading 2 records plus bits failed\n", __LINE__);
		result = 1;
	} else if (memcmp(buf, outstr, sizeof(outstr) - 1) != 0
		|| memcmp(&buf[sizeof(outstr) - 1], outstr, sizeof(outstr) - 1) != 0
		|| buf[2 * (sizeof(outstr) - 1)] != '1'
		|| buf[2 * (sizeof(outstr) - 1) + 1] != '2') {
		printf("%d: reading records failed\n", __LINE__);
		result = 1;
	} else if (ungetc('9', fp) == EOF) {
		printf("%d: ungetc() failed\n", __LINE__);
		result = 1;
	} else if (fseek(fp, -(2 + 2 * (sizeof(outstr) - 1)), SEEK_END) != 0) {
		printf("%d: fseek after ungetc failed\n", __LINE__);
		result = 1;
	} else if (fread(buf, 1, 2 + 2 * (sizeof(outstr) - 1), fp)
		!= 2 + 2 * (sizeof(outstr) - 1)) {
		printf("%d: reading 2 records plus bits failed\n", __LINE__);
		result = 1;
	} else if (memcmp(buf, outstr, sizeof(outstr) - 1) != 0
		|| memcmp(&buf[sizeof(outstr) - 1], outstr, sizeof(outstr) - 1) != 0
		|| buf[2 * (sizeof(outstr) - 1)] != '1') {
		printf("%d: reading records for the second time failed\n", __LINE__);
		result = 1;
	} else if (buf[2 * (sizeof(outstr) - 1) + 1] == '9') {
		printf("%d: unget character not ignored\n", __LINE__);
		result = 1;
	} else if (buf[2 * (sizeof(outstr) - 1) + 1] != '2') {
		printf("%d: unget somehow changed character\n", __LINE__);
		result = 1;
	}

	fclose(fp);

	fp = fopen(fname, "r");
	if (fp == NULL) {
		printf("%d: fopen() failed\n\n", __LINE__);
		result = 1;
	} else if (fstat(fileno(fp), &st1) < 0) {
		printf("%d: fstat() before fseeko() failed\n\n", __LINE__);
		result = 1;
	} else if (fseeko(fp, 0, SEEK_END) != 0) {
		printf("%d: fseeko(fp, 0, SEEK_END) failed\n", __LINE__);
		result = 1;
	} else if (ftello(fp) != st1.st_size) {
		printf("%d: fstat st_size %zd ftello %zd\n", __LINE__,
			(size_t) st1.st_size, (size_t) ftello(fp));
		result = 1;
	} else
		printf("%d: SEEK_END works\n", __LINE__);
	if (fp != NULL)
		fclose(fp);

	fp = fopen(fname, "r");
	if (fp == NULL) {
		printf("%d: fopen() failed\n\n", __LINE__);
		result = 1;
	} else if (fstat(fileno(fp), &st1) < 0) {
		printf("%d: fstat() before fgetc() failed\n\n", __LINE__);
		result = 1;
	} else if (fgetc(fp) == EOF) {
		printf("%d: fgetc() before fseeko() failed\n\n", __LINE__);
		result = 1;
	} else if (fseeko(fp, 0, SEEK_END) != 0) {
		printf("%d: fseeko(fp, 0, SEEK_END) failed\n", __LINE__);
		result = 1;
	} else if (ftello(fp) != st1.st_size) {
		printf("%d: fstat st_size %zd ftello %zd\n", __LINE__,
			(size_t) st1.st_size, (size_t) ftello(fp));
		result = 1;
	} else
		printf("%d: SEEK_END works\n", __LINE__);
	if (fp != NULL)
		fclose(fp);

out:
	unlink(fname);

	return result;
}


#define ASSERT(expr)														  \
	do {									  								  \
		if(!(expr)) {								 						  \
			fprintf(stderr, "%s:%d: assertion failed\n", __FILE__, __LINE__); \
			fflush(stderr);						 							  \
			exit(EXIT_FAILURE);												  \
		}																	  \
	} while(0)


int
gnulib_test_fseek(void)
{
	if (system("echo '#!/bin/sh' >/tmp/fseek_test.data") != 0)
		error(EXIT_FAILURE, errno, "cannot create /tmp/fseek_test.data");

	FILE *fp = fopen("/tmp/fseek_test.data", "r+");
	if (fp == NULL)
		error(EXIT_FAILURE, errno, "unable to open /tmp/fseek_test.data");

	// fetch two chars from the file
	int ch = fgetc(fp);
	ASSERT(ch == '#');
	ch = fgetc(fp);
	ASSERT(ch == '!');

	// test simple seeks to current pos, start and end
	ASSERT(ftell(fp) == 2);
	ASSERT(fseek(fp, 0, SEEK_CUR) == 0);
	ASSERT(ftell(fp) == 2);
	ASSERT(fseek(fp, 0, SEEK_SET) == 0);
	ASSERT(ftell(fp) == 0);
	ASSERT(fseek(fp, 0, SEEK_END) == 0);
	ASSERT(ftell(fp) == 10);
	ASSERT(fgetc(fp) == EOF);

	/* Position somewhere in the middle of the file ... */
	ASSERT(fseek(fp, 2, SEEK_SET) == 0);
	ASSERT(ftell(fp) == 2);
	ch = fgetc(fp);
	ASSERT(ch == '/');

	// ... and test that ungetc moves the file position backwards
	ASSERT(ftell(fp) == 3);
	ASSERT(ungetc(ch, fp) == ch);
	ASSERT(ftell(fp) == 2);
	ASSERT(fgetc(fp) == ch);
	ASSERT(ungetc(ch, fp) == ch);
	ASSERT(ftell(fp) == 2);
	ASSERT(ungetc('!', fp) == '!');
	ASSERT(ftell(fp) == 1);
	ASSERT(ungetc('#', fp) == '#');
	ASSERT(ftell(fp) == 0);
	ASSERT(fseek(fp, 0, SEEK_CUR) == 0);
	ASSERT(ftell(fp) == 0);
	ASSERT(fseek(fp, 2, SEEK_SET) == 0);
	ASSERT(ftell(fp) == 2);

	// test pushing other data with ungetc
	ASSERT(ungetc('x', fp) == 'x');
	ASSERT(ftell(fp) == 1);
	ASSERT(ungetc('y', fp) == 'y');
	ASSERT(ftell(fp) == 0);
	ASSERT(fgetc(fp) == 'y');
	ASSERT(ftell(fp) == 1);
	ASSERT(fgetc(fp) == 'x');
	ASSERT(ftell(fp) == 2);

	// test that fseek discards any data that was pushed with ungetc
	ASSERT(ungetc('x', fp) == 'x');
	ASSERT(ftell(fp) == 1);
	ASSERT(ungetc('y', fp) == 'y');
	ASSERT(fseek(fp, 0, SEEK_CUR) == 0);
	ASSERT(ftell(fp) == 0);
	ASSERT(fgetc(fp) == '#');
	ASSERT(ftell(fp) == 1);
	ASSERT(fgetc(fp) == '!');
	ASSERT(ftell(fp) == 2);

	// test that ungetc resets EOF
	ASSERT(fseek(fp, 0, SEEK_END) == 0);
	ASSERT(ftell(fp) == 10);
	ASSERT(!feof(fp));
	ASSERT(fgetc(fp) == EOF);
	ASSERT(feof(fp));
	ASSERT(ungetc(' ', fp) == ' ');
	ASSERT(!feof(fp));
	ASSERT(fgetc(fp) == ' ');
	ASSERT(!feof(fp));
	ASSERT(fgetc(fp) == EOF);
	ASSERT(feof(fp));

	// test that fseek restores EOF
	ASSERT(fseek(fp, 2, SEEK_SET) == 0);
	ASSERT(ftell(fp) == 2);
	ASSERT(fseek(fp, 0, SEEK_END) == 0);
	ASSERT(ftell(fp) == 10);
	ASSERT(ungetc(' ', fp) == ' ');
	ASSERT(fseek(fp, 0, SEEK_END) == 0);
	ASSERT(fgetc(fp) == EOF);
	ASSERT(ftell(fp) == 10);

	return 0;
}


int
main(void)
{
	int result = 0;
	if (result == 0)
		result = glibc_test();
	if (result == 0)
		result = gnulib_test_fseek();

	return result;
}
