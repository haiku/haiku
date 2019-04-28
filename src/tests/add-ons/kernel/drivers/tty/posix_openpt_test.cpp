
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <signal.h>

void die(const char *ptr, ...)
{
	va_list vlist;
	va_start(vlist, ptr);
	char ch;

	while ((ch = *ptr++) != '\0') {
		if (ch == '%' && *ptr != '\0') {
			ch = *ptr++;
			switch(ch) {
			case 'd': printf("%d", va_arg(vlist, int)); break;
			case 'e': printf("%e", va_arg(vlist, double)); break;
			case 'c': printf("%c", va_arg(vlist, int)); break;
			case 's': printf("%s", va_arg(vlist, char *)); break;
			default: va_end(vlist);exit(1);

			}

		} else
			printf("%c", ch);


	}

	va_end(vlist);
	exit(1);
}


int main()
{
	int pid;
	int ptm;
	int pts;
	char *ptr;
	char *program_name[2] = { (char*)"/bin/uname", NULL};
	char buf[512];
	int n;

	if ((ptm = posix_openpt(O_RDWR)) == -1) {
		die("posix_openpt error: %s\n", strerror(errno));
	}
	if (grantpt(ptm) == -1) {
		die("grantpt error: %s\n", strerror(errno));
	}
	if (unlockpt(ptm) == -1) {
		die("unlockpt error: %s\n", strerror(errno));
	}

	pid = fork();
	if (pid < 0) {
		die("fork error: %s\n", strerror(errno));
	} else if (pid == 0) { // child
		if (setsid() == (pid_t)-1) {
			die("setsid() error: %s\n", strerror(errno));
		}
		if ((ptr = (char *) ptsname(ptm)) == NULL) {
			die("ptsname error: %s\n", strerror(errno));
		}
		if ((pts = open(ptr, O_RDWR)) < 0) {
			die("open of slave failed: %a\n", strerror(errno));
		}
		close(ptm);

		if (dup2(pts, STDIN_FILENO) != STDIN_FILENO
			|| dup2(pts, STDOUT_FILENO) != STDOUT_FILENO
			|| dup2(pts, STDERR_FILENO) != STDERR_FILENO) {
			die("error doing dup's : %s\n", strerror(errno));
		}

		if (execve(*program_name, program_name , NULL) == -1) {
			die("execve error: %s\n", strerror(errno));
		}
		exit(1);
	} else { // parent
		if (dup2(ptm, STDIN_FILENO) != STDIN_FILENO) {
			die("dup2 of parent failed");
		}
		while (1) {
			n = read(ptm, buf, 511);
			if (n <= 0) {
				break;
			}
			write(STDOUT_FILENO, buf, n);
		}
	}
	return 0;
}

