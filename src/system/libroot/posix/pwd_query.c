
/*
 * This is a reimplementation of the BeOS R5 query-based multi-user system.
 * (c) 2005, François Revol.
 * provided under the MIT licence
 */

//#ifdef REAL_MULTIUSER
#if 1

#include <pwd.h>
#include <grp.h>

#include <errno.h>
#include <fs_attr.h>
#include <fs_info.h>
#include <fs_query.h>
#include <string.h>
#include <TLS.h>
#include <Debug.h>
#include <TypeConstants.h>

#include <errno_private.h>

/*
 * Some notes.
 * Users are stored in an fs node (not necessarily a regular file,
 * that could be a dir, even the user's home dir,
 * though we should be careful as they are owned by users.
 */


#define GR_MAX_NAME 32

#define PW_MAX_NAME 32
#define PW_MAX_DIR B_PATH_NAME_LENGTH
#define PW_MAX_GECOS 128
#define PW_MAX_PASSWD 32
#define PW_MAX_SHELL B_PATH_NAME_LENGTH

/* should be more than enough :) */
#define GRBUFFSZ (GR_MAX_NAME + 2)
#define PWBUFFSZ (PW_MAX_NAME + PW_MAX_DIR + PW_MAX_GECOS + PW_MAX_PASSWD + PW_MAX_SHELL + 4)


/* attribute names we use to store groups & users. */
/* pets nm & strings */

static const char *B_GR_GID = "sys:group:gid";
static const char *B_GR_NAME = "sys:group:name";
//static const char *B_GR_PASSWD = "sys:group:passwd";

static const char *B_PW_DIR = "sys:user:dir";
static const char *B_PW_GECOS = "sys:user:fullname";
static const char *B_PW_GID = "sys:user:gid";
static const char *B_PW_NAME = "sys:user:name";
static const char *B_PW_PASSWD = "sys:user:passwd";
static const char *B_PW_SHELL = "sys:user:shell";
static const char *B_PW_UID = "sys:user:uid";

/* queries */
static const char *Q_GR_ALL = "sys:group:gid>-1";
static const char *QT_GR_GID = "sys:group:gid==%d";
static const char *QT_GR_NAM = "sys:group:name==\"%s\"";
static const char *Q_PW_ALL = "sys:user:uid>-1";
static const char *QT_PW_UID = "sys:user:uid==%d";
static const char *QT_PW_NAM = "sys:user:name==\"%s\"";

extern void __init_pwd_stuff(void);
extern void __fini_pwd_stuff(void);

static char *default_gr_members[] = { /*"baron",*/ NULL };

static dev_t boot_device;

/* TLS stuff */

static int32 pw_tls_id;

typedef struct pw_tls {
	DIR *grent_query;
	DIR *pwent_query;

	int gridx;
	int pwidx;

	char grfile[B_PATH_NAME_LENGTH+1]; /* current group's cached file path */
	char pwfile[B_PATH_NAME_LENGTH+1]; /* current user's cached file path */

	struct group grent;
	char grbuff[GRBUFFSZ]; /* XXX: merge with pwbuff ? */

	struct passwd pwent;
	char pwbuff[PWBUFFSZ];
} pw_tls_t;

struct pw_tls *get_pw_tls(void)
{
	pw_tls_t *p = (pw_tls_t *)tls_get(pw_tls_id);
	PRINT(("%s()\n", __FUNCTION__));

	if (!p) {
		p = (pw_tls_t *)malloc(sizeof(pw_tls_t));
		if (!p)
			return NULL;
		memset(p, 0, sizeof(pw_tls_t));
		p->grent_query = NULL;
		p->pwent_query = NULL;
		tls_set(pw_tls_id, p);
	}
	return p;
}

/* fill the path from the dirent and open() */
int dentopen(struct dirent *dent, char *path)
{
	int err;
	PRINT(("%s({%ld, %Ld, %ld, %Ld, %s}, )\n", __FUNCTION__, dent->d_pdev, dent->d_pino, dent->d_dev, dent->d_ino, dent->d_name));
	dent->d_dev = boot_device;
	err = get_path_for_dirent(dent, path, B_PATH_NAME_LENGTH);
	if ((err < 0) || (path[0] != '/')) {
		__set_errno(err);
		return -1;
	}
	PRINT(("%s: open(%s)\n", __FUNCTION__, path));
	return open(path, O_RDONLY);
}

/* group stuff */

int fill_grent_default(struct group *gbuf)
{
	PRINT(("%s()\n", __FUNCTION__));
	gbuf->gr_gid = 1000;
	gbuf->gr_name = gbuf->gr_gid?"users":"wheel";
	gbuf->gr_passwd = "";
	gbuf->gr_mem = default_gr_members;
	return 0;
}

int fill_grent_from_fd(int fd, struct group *gbuf, char *buf, size_t buflen)
{
	size_t left;
	ssize_t len;
	left = buflen;
	len = fs_read_attr(fd, B_GR_GID, B_INT32_TYPE, 0LL, &gbuf->gr_gid, sizeof(gid_t));
	if (len < 0)
		return fill_grent_default(gbuf);
	PRINT(("%s: got gid\n", __FUNCTION__));
	gbuf->gr_passwd = "";
	gbuf->gr_mem = default_gr_members;

	if (left < GR_MAX_NAME + 1)
		return ERANGE;
	len = fs_read_attr(fd, B_GR_NAME, B_STRING_TYPE, 0LL, buf, GR_MAX_NAME);
	if (len < 0)
		return fill_grent_default(gbuf);
	gbuf->gr_name = buf;
	buf[len] = '\0';
	left -= len + 1;
	buf += len + 1;
	PRINT(("%s: got name\n", __FUNCTION__));
	return 0;
}

void setgrent(void)
{
	pw_tls_t *p;
	p = get_pw_tls();
	PRINT(("%s()\n", __FUNCTION__));
	if (p->grent_query) /* clumsy apps */
		fs_close_query(p->grent_query);
	p->grent_query = fs_open_query(boot_device, Q_GR_ALL, 0);
	PRINT(("pwq: %p\n", p->grent_query));
	p->gridx = 0;
}

void endgrent(void)
{
	pw_tls_t *p;
	PRINT(("%s()\n", __FUNCTION__));
	p = get_pw_tls();

	if (p->grent_query)
		fs_close_query(p->grent_query);
	p->grent_query = NULL;
	p->gridx = -1;
}


/* this conforms to the linux getgrent_r (there are several protos for that one... crap) */
/* note the FILE * based version is not supported as it makes no sense here */
int getgrent_r(struct group *gbuf, char *buf, size_t buflen, struct group **gbufp)
{
	pw_tls_t *p;
	int err;
	int fd;
	struct dirent *dent;
	PRINT(("%s()\n", __FUNCTION__));
	p = get_pw_tls();
	if (!p)
		return ENOMEM;
PRINT(("getgrent_r: grq = %p, idx = %d\n", p->grent_query, p->gridx));
	if (!p->grent_query)
		setgrent(); /* y0u clumsy app! */
	if (!p->grent_query)
		return EIO; /* something happened... */
	__set_errno(0);
	dent = fs_read_query(p->grent_query);
	*gbufp = NULL;
	if (!dent) {
		/* found nothing on first iteration ? */
		if (p->gridx == 0) {
			if (fill_grent_default(gbuf) < 0)
				return -1;
			*gbufp = gbuf;
			p->gridx++;
		}
		return 0;
	}
	fd = dentopen(dent, p->grfile);
	if (fd < B_OK)
		return errno?errno:-1;
	err = fill_grent_from_fd(fd, gbuf, buf, buflen);
	PRINT(("%s: fill_grent_from_fd = %d\n", __FUNCTION__, err));
	close(fd);
	if (err)
		return err;
	p->gridx++;
	*gbufp = gbuf;
	return 0;
}

struct group *getgrent(void)
{
	pw_tls_t *p;
	struct group *ent;
	int err;
	PRINT(("%s()\n", __FUNCTION__));
	p = get_pw_tls();
	if (!p) {
		/* we are really bork */
		__set_errno(ENOMEM);
		return NULL;
	}
	err = getgrent_r(&p->grent, p->grbuff, GRBUFFSZ, &ent);
	if (err < 0) {
		__set_errno(err);
		return NULL;
	}
	if (!ent)
		return NULL;
	PRINT(("getgrent(); returning entry for %s\n", ent->gr_name));
	return ent;
}

/* by gid */
struct group *getgrgid(gid_t gid)
{
	struct dirent *dent;
	pw_tls_t *p;
	DIR *query;
	int err;
	int fd;

	PRINT(("%s()\n", __FUNCTION__));
	p = get_pw_tls();
	if (!p) {
		/* we are really bork */
		__set_errno(ENOMEM);
		return NULL;
	}

	/* reusing path */
	sprintf(p->grfile, QT_GR_GID, gid);
	query = fs_open_query(boot_device, p->grfile, 0);
	PRINT(("q: %p\n", query));
	if (!query)
		return NULL;

	dent = fs_read_query(query);
	if (!dent) {
		fs_close_query(query);
		return NULL;
	}
	fd = dentopen(dent, p->grfile);
	fs_close_query(query);
	if (fd < B_OK)
		return NULL;
	err = fill_grent_from_fd(fd, &p->grent, p->grbuff, GRBUFFSZ);
	PRINT(("%s: fill_grent_from_fd = %d\n", __FUNCTION__, err));
	close(fd);
	if (err)
		return NULL;
	return &p->grent;

}

/* by name */
struct group *getgrnam(const char *name)
{
	struct dirent *dent;
	pw_tls_t *p;
	DIR *query;
	int err;
	int fd;

	PRINT(("%s()\n", __FUNCTION__));
	p = get_pw_tls();
	if (!p) {
		/* we are really bork */
		__set_errno(ENOMEM);
		return NULL;
	}

	if (!name || strlen(name) > GR_MAX_NAME) {
		__set_errno(EINVAL);
		return NULL;
	}
	/* reusing path */
	sprintf(p->grfile, QT_GR_NAM, name);
	query = fs_open_query(boot_device, p->grfile, 0);
	PRINT(("q: %p\n", query));
	if (!query)
		return NULL;

	dent = fs_read_query(query);
	if (!dent) {
		fs_close_query(query);
		return NULL;
	}
	fd = dentopen(dent, p->grfile);
	fs_close_query(query);
	if (fd < B_OK)
		return NULL;
	err = fill_grent_from_fd(fd, &p->grent, p->grbuff, GRBUFFSZ);
	PRINT(("%s: fill_grent_from_fd = %d\n", __FUNCTION__, err));
	close(fd);
	if (err)
		return NULL;
	return &p->grent;

}


/* user stuff */

int fill_pwent_default(struct passwd *pwbuf)
{
	PRINT(("%s()\n", __FUNCTION__));
	return ENOENT; /* hmm no we don't exist! */
	pwbuf->pw_gid = 1000;
	pwbuf->pw_uid = 1000;
	pwbuf->pw_name = "baron";
	pwbuf->pw_passwd = "*";
	pwbuf->pw_dir = "/var/tmp";
	pwbuf->pw_shell = "/bin/false";
	pwbuf->pw_gecos = "Unknown User";
	return 0;
}

int fill_pwent_from_fd(int fd, struct passwd *pwbuf, char *buf, size_t buflen)
{
	ssize_t left;
	ssize_t len;
	PRINT(("%s()\n", __FUNCTION__));
	left = buflen;
	if (left <= 0)
		return ERANGE;

	len = fs_read_attr(fd, B_PW_GID, B_INT32_TYPE, 0LL, &pwbuf->pw_gid, sizeof(gid_t));
	if (len < 0)
		return fill_pwent_default(pwbuf);
	PRINT(("%s: got gid\n", __FUNCTION__));

	len = fs_read_attr(fd, B_PW_UID, B_INT32_TYPE, 0LL, &pwbuf->pw_uid, sizeof(uid_t));
	if (len < 0)
		return fill_pwent_default(pwbuf);
	PRINT(("%s: got uid\n", __FUNCTION__));

	if (left < PW_MAX_NAME + 1)
		return ERANGE;
	len = fs_read_attr(fd, B_PW_NAME, B_STRING_TYPE, 0LL, buf, PW_MAX_NAME);
	if (len < 0)
		return fill_pwent_default(pwbuf);
	pwbuf->pw_name = buf;
	buf[len] = '\0';
	left -= len + 1;
	buf += len + 1;
	PRINT(("%s: got name\n", __FUNCTION__));

	if (left < PW_MAX_DIR + 1)
		return ERANGE;
	len = fs_read_attr(fd, B_PW_DIR, B_STRING_TYPE, 0LL, buf, PW_MAX_DIR);
	if (len < 0)
		return fill_pwent_default(pwbuf);
	pwbuf->pw_dir = buf;
	buf[len] = '\0';
	left -= len + 1;
	buf += len + 1;
	PRINT(("%s: got dir\n", __FUNCTION__));

	if (left < PW_MAX_SHELL + 1)
		return ERANGE;
	len = fs_read_attr(fd, B_PW_SHELL, B_STRING_TYPE, 0LL, buf, PW_MAX_SHELL);
	if (len < 0)
		return fill_pwent_default(pwbuf);
	pwbuf->pw_shell = buf;
	buf[len] = '\0';
	left -= len + 1;
	buf += len + 1;
	PRINT(("%s: got shell\n", __FUNCTION__));

	if (left < PW_MAX_GECOS + 1)
		return ERANGE;
	len = fs_read_attr(fd, B_PW_GECOS, B_STRING_TYPE, 0LL, buf, PW_MAX_GECOS);
	if (len < 0)
		return fill_pwent_default(pwbuf);
	pwbuf->pw_gecos = buf;
	buf[len] = '\0';
	left -= len + 1;
	buf += len + 1;
	PRINT(("%s: got gecos\n", __FUNCTION__));

	if (left < PW_MAX_PASSWD + 1)
		return ERANGE;
	len = fs_read_attr(fd, B_PW_PASSWD, B_STRING_TYPE, 0LL, buf, PW_MAX_PASSWD);
	if (len < 0) {
		buf[0] = '*'; /* no pass set */
		len = 1;
	}
	pwbuf->pw_passwd = buf;
	buf[len] = '\0';
	left -= len + 1;
	buf += len + 1;
	PRINT(("%s: got passwd\n", __FUNCTION__));

	return 0;
}



void setpwent(void)
{
	pw_tls_t *p;
	p = get_pw_tls();
	PRINT(("%s()\n", __FUNCTION__));
	if (p->pwent_query) /* clumsy apps */
		fs_close_query(p->pwent_query);
	p->pwent_query = fs_open_query(boot_device, Q_PW_ALL, 0);
	PRINT(("pwq: %p\n", p->pwent_query));
	p->pwidx = 0;
}

void endpwent(void)
{
	pw_tls_t *p;
	PRINT(("%s()\n", __FUNCTION__));
	p = get_pw_tls();

	if (p->pwent_query)
		fs_close_query(p->pwent_query);
	p->pwent_query = NULL;
	p->pwidx = -1;
}


/* this conforms to the linux getpwent_r (there are several protos for that one... crap) */
/* note the FILE * based version is not supported as it makes no sense here */
int getpwent_r(struct passwd *pwbuf, char *buf, size_t buflen, struct passwd **pwbufp)
{
	pw_tls_t *p;
	int err;
	int fd;
	struct dirent *dent;
	PRINT(("%s()\n", __FUNCTION__));
	p = get_pw_tls();
	if (!p)
		return ENOMEM;
PRINT(("getpwent_r: pwq = %p, idx = %d\n", p->pwent_query, p->pwidx));
	if (!p->pwent_query)
		setpwent(); /* y0u clumsy app! */
	if (!p->pwent_query)
		return EIO; /* something happened... */
	__set_errno(0);
	dent = fs_read_query(p->pwent_query);
	*pwbufp = NULL;
	if (!dent) {
		/* found nothing on first iteration ? */
		if (p->pwidx == 0) {
			if (fill_pwent_default(pwbuf) < 0)
				return -1;
			*pwbufp = pwbuf;
			p->pwidx++;
		}
		return 0;
	}
	fd = dentopen(dent, p->pwfile);
	if (fd < B_OK)
		return errno?errno:-1;
	err = fill_pwent_from_fd(fd, pwbuf, buf, buflen);
	PRINT(("%s: fill_pwent_from_fd = %d\n", __FUNCTION__, err));
	close(fd);
	if (err)
		return err;
	p->pwidx++;
	*pwbufp = pwbuf;
	return 0;
}

struct passwd *getpwent(void)
{
	pw_tls_t *p;
	struct passwd *ent;
	int err;
	PRINT(("%s()\n", __FUNCTION__));
	p = get_pw_tls();
	if (!p) {
		/* we are really bork */
		__set_errno(ENOMEM);
		return NULL;
	}
	err = getpwent_r(&p->pwent, p->pwbuff, PWBUFFSZ, &ent);
	if (err < 0) {
		__set_errno(err);
		return NULL;
	}
	if (!ent)
		return NULL;
	PRINT(("getpwent(); returning entry for %s\n", ent->pw_name));
	return ent;
}

/* by gid */
struct passwd *getpwuid(uid_t uid)
{
	struct dirent *dent;
	pw_tls_t *p;
	DIR *query;
	int err;
	int fd;

	PRINT(("%s(%d)\n", __FUNCTION__, uid));
	p = get_pw_tls();
	if (!p) {
		/* we are really bork */
		__set_errno(ENOMEM);
		return NULL;
	}

	/* reusing path */
	sprintf(p->pwfile, QT_PW_UID, uid);
	PRINT(("%s: query(%s)\n", __FUNCTION__, p->pwfile));
	query = fs_open_query(boot_device, p->pwfile, 0);
	PRINT(("q: %p\n", query));
	if (!query)
		return NULL;

	dent = fs_read_query(query);
	if (!dent) {
		fs_close_query(query);
		return NULL;
	}
	fd = dentopen(dent, p->pwfile);
	fs_close_query(query);
	if (fd < B_OK)
		return NULL;
	err = fill_pwent_from_fd(fd, &p->pwent, p->pwbuff, PWBUFFSZ);
	PRINT(("%s: fill_pwent_from_fd = %d\n", __FUNCTION__, err));
	close(fd);
	if (err)
		return NULL;
	return &p->pwent;

}

/* by name */
struct passwd *getpwnam(const char *name)
{
	struct dirent *dent;
	pw_tls_t *p;
	DIR *query;
	int err;
	int fd;

	PRINT(("%s(%s)\n", __FUNCTION__, name));
	p = get_pw_tls();
	if (!p) {
		/* we are really bork */
		__set_errno(ENOMEM);
		return NULL;
	}

	if (!name || strlen(name) > PW_MAX_NAME) {
		__set_errno(EINVAL);
		return NULL;
	}
	/* reusing path */
	sprintf(p->pwfile, QT_PW_NAM, name);
	PRINT(("%s: query(%s)\n", __FUNCTION__, p->pwfile));
	query = fs_open_query(boot_device, p->pwfile, 0);
	PRINT(("q: %p\n", query));
	if (!query)
		return NULL;

	dent = fs_read_query(query);
	if (!dent) {
		fs_close_query(query);
		return NULL;
	}
	PRINT(("%s: dentopen()\n", __FUNCTION__));
	fd = dentopen(dent, p->pwfile);
	fs_close_query(query);
	if (fd < B_OK)
		return NULL;
	err = fill_pwent_from_fd(fd, &p->pwent, p->pwbuff, PWBUFFSZ);
	PRINT(("%s: fill_pwent_from_fd = %d\n", __FUNCTION__, err));
	close(fd);
	if (err)
		return NULL;
	return &p->pwent;

}

void __init_pwd_backend(void)
{
	/* dev_t for the boot volume */
	boot_device = dev_for_path("/boot");
	/* get us an id for holding thread-specific data */
	pw_tls_id = tls_allocate();
}

/*
void __fini_pwd_backend(void)
{

}
*/

#endif /* REAL_MULTIUSER */

