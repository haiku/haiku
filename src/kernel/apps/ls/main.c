/*
** Copyright 2002, Travis Geiselbrecht. All rights reserved.
** Distributed under the terms of the NewOS License.
*/
#include <syscalls.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>

extern char *__progname;

void (*disp_func)(const char *, struct stat *) = NULL;
static int show_all = 0;

mode_t perms [9] = {
	S_IRUSR,
	S_IWUSR,
	S_IXUSR,
	S_IRGRP,
	S_IWGRP,
	S_IXGRP,
	S_IROTH,
	S_IWOTH,
	S_IXOTH
};
	
static void display_l(const char *filename, struct stat *stat)
{
	const char *type;
	char perm[11];
	int i;
	memset(perm, '-', 10);
	perm[10] = '\0';
	
	for (i=0; i < sizeof(perms) / sizeof(mode_t); i++) {
		if (stat->st_mode & perms[i]) {
			switch(i % 3) {
				case 0:
					perm[i + 1] = 'r';
					break;
				case 1:
					perm[i+1] = 'w';
					break;
				case 2:
					perm[i+1] = 'x';
			}
		}
	}
	if (S_ISDIR(stat->st_mode))
		perm[0] = 'd';
			
	printf("%10s %12lld %s\n" ,perm ,stat->st_size ,filename);
}

static void display(const char *filename, struct stat *stat)
{
	printf("%s\n", filename);
}

int main(int argc, char *argv[])
{
	int rc;
	int rc2;
	int count = 0;
	struct stat st;
	char *arg;
	int ch;
	uint64 totbytes = 0;
	
	disp_func = display;
	
	while ((ch = getopt(argc, argv, "al")) != -1) {
		switch (ch) {
			case 'a':
				show_all = 1;
				break;
			case 'l':
				disp_func = display_l;
				break;
		}
	}
	argc -= optind;
	argv += optind;

	if(*argv == NULL) {
		arg = ".";
	} else {
		arg = *argv;
	}
	
	rc = stat(arg, &st);
	if(rc < 0) {
		printf("%s: %s: %s\n", __progname,
		       arg, strerror(rc));
		goto err_ls;
	}

	if (S_ISDIR(st.st_mode)) {
		char buf[1024];
		DIR *thedir = opendir(arg);
		
		if (!thedir) {
			printf("ls: %s: %s\n", arg, strerror(errno));
		} else {
			if (show_all) {
				/* process the '.' entry */
				rc = stat(arg, &st);
				if (rc == 0) {
					(*disp_func)(".", &st);
					totbytes += st.st_size;
				}
			}
			
			for(;;) {
				char buf2[1024];
				struct dirent *de = readdir(thedir);

				if (!de)
					break;

				memset(buf2, 0, sizeof(buf2));
				if (strcmp(arg, ".") != 0) {
					strlcpy(buf2, arg, sizeof(buf2));
					strlcat(buf2, "/", sizeof(buf2));
				}
				strlcat(buf2, de->d_name, sizeof(buf2));

				rc2 = stat(buf2, &st);
				if (rc2 == 0) {
					(*disp_func)(de->d_name, &st);
					totbytes += st.st_size;
				}
				count++;
			}
			closedir(thedir);
		}

		printf("%lld bytes in %d files\n", totbytes, count);
	} else {
		(*disp_func)(arg, &st);
	}

err_ls:
	return 0;
}
