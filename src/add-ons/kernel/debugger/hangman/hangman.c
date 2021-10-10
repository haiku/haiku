#ifdef _KERNEL_MODE
#	include <Drivers.h>
#	include <KernelExport.h>
#	include <module.h>
#	include <debug.h>
#endif

#include <directories.h>
#include <OS.h>
#include <image.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

/* as driver or module */
//#define AS_DRIVER 1

/* do we reboot on loose ? */
//#define FAIL_IN_BSOD_CAUSE_REBOOT 1

/* shortcut to be able to exit (and take a screenshot) */
#define CAN_EXIT_ON_DASH

#define MAX_FAILS_BEFORE_BSOD 0

#ifdef __HAIKU__
#	define FORTUNE_FILE kSystemDataDirectory "/fortunes/Fortunes"
#else
#	define FORTUNE_FILE "/etc/fortunes/default"
#endif

#define KCMD_HELP "A funny KDL hangman game :-)"

#define DEV_ENTRY "misc/hangman"

#define KERNEL_IMAGE_ID 1

#define MIN_LETTERS 3
#define MAX_LETTERS 10

#define MAX_CACHED_WORDS 5

char words[MAX_CACHED_WORDS][MAX_LETTERS+1];

#ifndef __HAIKU__

/* design ripped off from http://www.latms.berkeley.k12.ca.us/perl/node30.html :) */
static char hungman[] = \
"  ____      \n" \
"  |   |     \n" \
"  |   %c     \n" \
"  |  %c%c%c    \n" \
"  |  %c %c    \n" \
"  |         \n";

#else

/* some colors */
static char hungman_ansi[] = \
"  ____      \n" \
"  |   |     \n" \
"  |   \033[36m%c\033[0m     \n" \
"  |  \033[35m%c%c%c\033[0m    \n" \
"  |  \033[35m%c %c\033[0m    \n" \
"  |         \n";

#endif

// for gets,
#define BIGBUFFSZ 128
char bigbuffer[BIGBUFFSZ];

#define BIT_FROM_LETTER(l) (0x1 << (l - 'a'))

status_t init_words(char *from);
status_t init_words_from_threadnames(void);
void print_hangman(int fails);
void display_word(int current, uint32 tried_letters);
int play_hangman(void);
int kdlhangman(int argc, char **argv);

#ifdef _KERNEL_MODE

# ifdef __HAIKU__
extern int kgets(char *buf, int len);
#  define PRINTF kprintf
#  define GETS(a) ({int l; kprintf("hangman> "); l = kgets(a, sizeof(a)); l?a:NULL;})
#  define HIDDEN_LETTER '_'
#  define HUNGMAN hungman_ansi
# else
/* BeOS R5 version, needs some R5 kernel privates... */
/* the kernel pointer to the bsod_gets */
static char *(*bsod_gets)(char *, char *, int);
extern char *(*bsod_kgets)(char *, char *, int);
//extern char *bsod_gets(char *);
/* saved here before panic()ing */
char *(*bsod_saved_kgets)(char *, char *, int);
#  define PRINTF kprintf
#  define GETS(a) ((*bsod_kgets)?(*bsod_kgets):(*bsod_gets))("hangman> ", a, sizeof(a))
#  define HIDDEN_LETTER '_'
#  define HUNGMAN hungman
# endif
#else
/* userland version */
# define PRINTF printf
# define GETS(a) gets(a)
# define dprintf printf
# define HIDDEN_LETTER '.'
#  define HUNGMAN hungman_ansi
#endif /* !_KERNEL_MODE */


status_t
init_words(char *from)
{
	int fd;
	size_t sz, got, beg, end, i;
	int current;
	struct stat st;

	memset((void *)words, 0, sizeof(words));
	fd = open(from, O_RDONLY);
	if (fd < B_OK)
		return fd;
	/* lseek() seems to always return 0 from the kernel ??? */
	if (fstat(fd, &st)) {
		close(fd);
		return B_ERROR;
	}
	sz = (size_t)st.st_size;
//	sz = (size_t)lseek(fd, 0, SEEK_END);
//	dprintf("khangman: lseek(): %ld\n", sz);
	if (sz < 30) {
		dprintf("hangman: fortune file too small\n");
		return B_ERROR;
	}
//	lseek(fd, 0, SEEK_SET);
	//srand((unsigned int)(system_time() + (system_time() >> 32) + find_thread(NULL)));
	srand((unsigned int)(system_time() & 0x0ffffffff));
	for (current = 0; current < MAX_CACHED_WORDS; current++) {
		off_t offset = (rand() % (sz - MAX_LETTERS));
	//	dprintf("current %d, offset %ld\n", current, (long)offset);
		lseek(fd, offset, SEEK_SET);
		got = read(fd, bigbuffer, BIGBUFFSZ - 2);
	//	dprintf("--------------buff(%d):\n%20s\n", current, bigbuffer);
		for (beg = 0; beg < got && isalpha(bigbuffer[beg]); beg++);
		for (; beg < got && !isalpha(bigbuffer[beg]); beg++);
		if (beg + 1 < got && isalpha(bigbuffer[beg])) {
			for (end = beg; end < got && isalpha(bigbuffer[end]); end++);
			if (end < got && !isalpha(bigbuffer[end]) && beg + MIN_LETTERS < end) {
				/* got one */
				/* tolower */
				for (i = beg; i < end; i++)
					bigbuffer[i] = tolower(bigbuffer[i]);
				strncpy(&(words[current][0]), &(bigbuffer[beg]), end - beg);
			} else
				current--;
		} else
			current--;
	}
	close(fd);
/*
	for (current = 0; current < MAX_CACHED_WORDS; current++)
		dprintf("%s\n", words[current]);
*/
	return B_OK;
}


status_t
init_words_from_threadnames(void)
{
	size_t sz, got, beg, end, i;
	int current;
	thread_info ti;

	memset((void *)words, 0, sizeof(words));
	srand((unsigned int)(system_time() & 0x0ffffffff));
	for (current = 0; current < MAX_CACHED_WORDS; ) {
		int offset;
		char *p;
		if (get_thread_info(rand() % 200, &ti) != B_OK)
			continue;
		sz = strnlen(ti.name, B_OS_NAME_LENGTH);
		if (sz <= MIN_LETTERS)
			continue;
		offset = (rand() % (sz - MIN_LETTERS));
		//dprintf("thread '%-.32s' + %d\n", ti.name, offset);
		p = ti.name + offset;
		got = sz - offset;
		for (beg = 0; beg < got && isalpha(p[beg]); beg++);
		for (; beg < got && !isalpha(p[beg]); beg++);
		if (beg + 1 < got && isalpha(p[beg])) {
			for (end = beg; end < got && isalpha(p[end]); end++);
			if (end < got && !isalpha(p[end]) && beg + MIN_LETTERS < end) {
				/* got one */
				/* tolower */
				for (i = beg; i < end; i++)
					p[i] = tolower(p[i]);
				strncpy(&(words[current][0]), &(p[beg]), end - beg);
			} else
				continue;
		} else
			continue;
		current++;
	}
	/*
	for (current = 0; current < MAX_CACHED_WORDS; current++)
		dprintf("%s\n", words[current]);
	*/
	return B_OK;
}


void
print_hangman(int fails)
{
	PRINTF(HUNGMAN,
		(fails > 0)?'O':' ',
		(fails > 2)?'/':' ',
		(fails > 1)?'|':' ',
		(fails > 3)?'\\':' ',
		(fails > 4)?'/':' ',
		(fails > 5)?'\\':' ');
}


void
display_word(int current, uint32 tried_letters)
{
	int i = 0;
	PRINTF("word> ");
	while (words[current][i]) {
		PRINTF("%c", (BIT_FROM_LETTER(words[current][i]) & tried_letters)?(words[current][i]):HIDDEN_LETTER);
		i++;
	}
	PRINTF("\n");
}

int
play_hangman(void)
{
	int current;
	int score = 0;
	int bad_guesses;
	uint32 tried_letters;
	char *str;
	char try;
	int gotit, gotone;

	for (current = 0; current < MAX_CACHED_WORDS; current++) {
		tried_letters = 0;
		gotit = 0;
		for (bad_guesses = 0; bad_guesses < 6; bad_guesses++) {
			do {
				gotit = 0;
				gotone = 1;
				display_word(current, tried_letters);
				str = GETS(bigbuffer);
				if (!str) {
					str = bigbuffer;
					PRINTF("buffer:%s\n", str);
				}
#ifdef CAN_EXIT_ON_DASH
				if (str[0] == '-') /* emergency exit */
					return 0;
#endif
				if (!isalpha(str[0])) {
					PRINTF("not a letter\n");
				} else {
					try = tolower(str[0]);
					if (BIT_FROM_LETTER(try) & tried_letters) {
						PRINTF("%c already tried\n", try);
					} else {
						// REUSE
						str = words[current];
						gotit = 1;
						gotone = 0;
						tried_letters |= BIT_FROM_LETTER(try);
						while (*str) {
							if (!(BIT_FROM_LETTER(*str) & tried_letters))
								gotit=0;
							if (try == *str)
								gotone = 1;
							str++;
						}
					}
				}
				//PRINTF("gotone:%d, gotit:%d, tried_letters:%08lx\n", gotone, gotit, tried_letters);
			} while(tried_letters != 0x03ffffff && !gotit && gotone);
			if (gotit)
				break;
			print_hangman(bad_guesses+1);
		}
		if (bad_guesses < 6) {
			display_word(current, 0x03ffffff);
			if (strlen(words[current]) < 5)
				PRINTF("That was easy :-P\n");
			else if (strlen(words[current]) < 7)
				PRINTF("Good one !\n");
			else
				PRINTF("You got this hard one ! :-)\n");
			score ++;
		}
/**/
		else return score;
/**/
	}
	return score;
}


#ifdef _KERNEL_MODE /* driver parts */


#ifndef __HAIKU__ /* BeOS intimacy revealed */
//char *bsod_wrapper_gets(char *p, int len)
//char *bsod_wrapper_gets(int len, char *p)
char *
bsod_wrapper_gets(char *prompt, char *p, int len)
{
	/* fall back to the normal gets() */
	bsod_kgets = bsod_saved_kgets;
//	if (!bsod_kgets)
//		bsod_kgets = bsod_gets;
	/* and fake some typing */
	strcpy(p, fake_typed);
	return p;
}
#else

#endif


int
kdlhangman(int argc, char **argv)
{
	int score;

	if (argc > 1 && strcmp(argv[1], "--help") == 0) {
		PRINTF("%s\n", KCMD_HELP);
		return 0;
	}

	score = play_hangman();
PRINTF("score %d\n", score);
	if (score > (MAX_CACHED_WORDS - MAX_FAILS_BEFORE_BSOD)) {
		PRINTF("Congrats !\n");
	}
	if (score < (MAX_CACHED_WORDS - MAX_FAILS_BEFORE_BSOD)) {
#ifdef FAIL_IN_BSOD_CAUSE_REBOOT
		PRINTF("Hmmm, sorry, need to trash your hdd... Ok, just a reboot then\n");
		fake_typed = "reboot";
		bsod_kgets = bsod_wrapper_gets;
		return 1;
#else
		PRINTF("Hmmm, sorry, need to trash your hdd... Well, I'll be nice this time\n");
#endif
	}
	//return B_KDEBUG_CONT;
	return B_KDEBUG_QUIT;
}


#  ifdef AS_DRIVER

typedef struct {
	int dummy;
} cookie_t;

const char * device_names[]={DEV_ENTRY, NULL};


status_t
init_hardware(void) {
	return B_OK;
}


status_t
init_driver(void)
{
	status_t err;

	err = init_words(FORTUNE_FILE);
	if (err < B_OK) {
		dprintf("hangman: error reading fortune file: %s\n", strerror(err));
		return B_ERROR;
	}
	get_image_symbol(KERNEL_IMAGE_ID, "bsod_gets", B_SYMBOL_TYPE_ANY, (void **)&bsod_gets);
	add_debugger_command("kdlhangman", kdlhangman, KCMD_HELP);
	return B_OK;
}


void
uninit_driver(void)
{
	remove_debugger_command("kdlhangman", kdlhangman);
}


const char **
publish_devices()
{
	return device_names;
}


status_t
khangman_open(const char *name, uint32 flags, cookie_t **cookie)
{
	(void)name; (void)flags;
	*cookie = (void*)malloc(sizeof(cookie_t));
	if (*cookie == NULL) {
		dprintf("khangman_open : error allocating cookie\n");
		goto err0;
	}
	memset(*cookie, 0, sizeof(cookie_t));
	return B_OK;
err0:
	return B_ERROR;
}


status_t
khangman_close(void *cookie)
{
	(void)cookie;
	return B_OK;
}


status_t
khangman_free(cookie_t *cookie)
{
	free(cookie);
	return B_OK;
}


status_t
khangman_read(cookie_t *cookie, off_t position, void *data, size_t *numbytes)
{
	*numbytes = 0;
	return B_NOT_ALLOWED;
}


status_t
khangman_write(void *cookie, off_t position, const void *data, size_t *numbytes)
{
	(void)cookie; (void)position; (void)data; (void)numbytes;
	//*numbytes = 0;
	/* here we get to kdlhangman */
	fake_typed = "kdlhangman";
	bsod_saved_kgets = bsod_kgets;
	bsod_kgets = bsod_wrapper_gets;
	kernel_debugger("So much more fun in KDL...");

	return B_OK;
}


device_hooks khangman_hooks={
	(device_open_hook)khangman_open,
	khangman_close,
	(device_free_hook)khangman_free,
	NULL,
	(device_read_hook)khangman_read,
	khangman_write,
	NULL,
	NULL,
	NULL,
	NULL
};


device_hooks *
find_device(const char *name)
{
	(void)name;
	return &khangman_hooks;
}


#  else /* as module */


static status_t
std_ops(int32 op, ...)
{
	status_t err;

	switch (op) {
		case B_MODULE_INIT:
			err = init_words(FORTUNE_FILE);
			if (err < B_OK) {
				dprintf("hangman: error reading fortune file: %s\n",
					strerror(err));
				err = init_words_from_threadnames();
				if (err < B_OK) {
					dprintf("hangman: error getting thread names: %s\n",
						strerror(err));
					return B_ERROR;
				}
			}
			add_debugger_command("kdlhangman", kdlhangman, KCMD_HELP);
			return B_OK;
		case B_MODULE_UNINIT:
			remove_debugger_command("kdlhangman", kdlhangman);
			return B_OK;
	}

	return B_ERROR;
}


static struct debugger_module_info sModuleInfo = {
	{
		"debugger/hangman/v1",
		B_KEEP_LOADED,
		&std_ops
	},
	NULL,
	NULL,
	NULL,
	NULL
};

module_info *modules[] = {
	(module_info *)&sModuleInfo,
	NULL
};

#  endif /* AS_DRIVER */

#else

void
kdl_trip(void)
{
	int fd;
	fd = open("/dev/misc/hangman", O_WRONLY);
	if (fd < B_OK) {
		puts("hey, you're pissing me off, no /dev/"DEV_ENTRY" !!!");
		system("/bin/alert --stop 'It would work better with the hangman driver enabled...\nyou really deserve a forced reboot :P'");
		return;
	}
	write(fd, "hangme!", 7);
	close(fd);
}


int
main(int argc, char *argv)
{
	int score; /* how many correct guesses ? */
	/* init */
	if (init_words(FORTUNE_FILE) < B_OK) {
		fprintf(stderr, "error reading fortune file\n");
		return 1;
	}
	score = play_hangman();
	PRINTF("score %d\n", score);
	if (score > (MAX_CACHED_WORDS - MAX_FAILS_BEFORE_BSOD)) {
		PRINTF("Congrats !\n");
	}
	if (score < (MAX_CACHED_WORDS - MAX_FAILS_BEFORE_BSOD)) {
		/* too many fails... gonna kick :p */
		kdl_trip();
	}
	return 0;
}


#endif
