/*
 * Copyright 2017-2019, Jérôme Duval, jerome.duval@gmail.com
 * Distributed under the terms of the MIT License.
 */


#include <spawn.h>

#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <libroot_private.h>
#include <signal_defs.h>
#include <syscalls.h>


enum action_type {
	file_action_open,
	file_action_close,
	file_action_dup2,
	file_action_chdir,
	file_action_fchdir
};

struct _file_action {
	enum action_type type;
	int fd;
	union {
		struct {
			char* path;
			int	oflag;
			mode_t mode;
		} open_action;
		struct {
			int srcfd;
		} dup2_action;
		struct {
			char* path;
		} chdir_action;
	} action;
};

struct _posix_spawnattr {
	short	flags;
	pid_t	pgroup;
	sigset_t	sigdefault;
	sigset_t	sigmask;
};

struct _posix_spawn_file_actions {
	int		size;
	int		count;
	_file_action *actions;
};


static int
posix_spawn_file_actions_extend(struct _posix_spawn_file_actions *actions)
{
	int newsize = actions->size + 4;
	void *newactions = realloc(actions->actions,
		newsize * sizeof(struct _file_action));
	if (newactions == NULL)
		return ENOMEM;
	actions->actions = (struct _file_action*)newactions;
	actions->size = newsize;
	return 0;
}


int
posix_spawn_file_actions_init(posix_spawn_file_actions_t *_file_actions)
{
	posix_spawn_file_actions_t actions = (posix_spawn_file_actions_t)malloc(
		sizeof(struct _posix_spawn_file_actions));

	if (actions == NULL)
		return ENOMEM;

	memset(actions, 0, sizeof(*actions));
	*_file_actions = actions;

	return 0;
}


int
posix_spawn_file_actions_destroy(posix_spawn_file_actions_t *_actions)
{
	struct _posix_spawn_file_actions* actions = _actions != NULL ? *_actions : NULL;

	if (actions == NULL)
		return EINVAL;

	for (int i = 0; i < actions->count; i++) {
		struct _file_action *action = &actions->actions[i];

		if (action->type == file_action_open)
			free(action->action.open_action.path);
		else if (action->type == file_action_chdir)
			free(action->action.chdir_action.path);
	}

	free(actions);

	return 0;
}


int
posix_spawn_file_actions_addopen(posix_spawn_file_actions_t *_actions,
	int fildes, const char *path, int oflag, mode_t mode)
{
	struct _posix_spawn_file_actions* actions = _actions != NULL ? *_actions : NULL;

	if (actions == NULL)
		return EINVAL;

	if (fildes < 0 || fildes >= sysconf(_SC_OPEN_MAX))
		return EBADF;

	char* npath = strdup(path);
	if (npath == NULL)
		return ENOMEM;
	if (actions->count == actions->size
		&& posix_spawn_file_actions_extend(actions) != 0) {
		free(npath);
		return ENOMEM;
	}

	struct _file_action *action = &actions->actions[actions->count];
	action->type = file_action_open;
	action->fd = fildes;
	action->action.open_action.path = npath;
	action->action.open_action.oflag = oflag;
	action->action.open_action.mode = mode;
	actions->count++;
	return 0;
}


int
posix_spawn_file_actions_addclose(posix_spawn_file_actions_t *_actions,
	int fildes)
{
	struct _posix_spawn_file_actions* actions = _actions != NULL ? *_actions : NULL;

	if (actions == NULL)
		return EINVAL;

	if (fildes < 0 || fildes >= sysconf(_SC_OPEN_MAX))
		return EBADF;

	if (actions->count == actions->size
		&& posix_spawn_file_actions_extend(actions) != 0) {
		return ENOMEM;
	}

	struct _file_action *action = &actions->actions[actions->count];
	action->type = file_action_close;
	action->fd = fildes;
	actions->count++;
	return 0;
}


int
posix_spawn_file_actions_adddup2(posix_spawn_file_actions_t *_actions,
	int fildes, int newfildes)
{
	struct _posix_spawn_file_actions* actions = _actions != NULL ? *_actions : NULL;

	if (actions == NULL)
		return EINVAL;

	if (fildes < 0 || fildes >= sysconf(_SC_OPEN_MAX))
		return EBADF;

	if (actions->count == actions->size
		&& posix_spawn_file_actions_extend(actions) != 0) {
		return ENOMEM;
	}

	struct _file_action *action = &actions->actions[actions->count];
	action->type = file_action_dup2;
	action->fd = newfildes;
	action->action.dup2_action.srcfd = fildes;
	actions->count++;
	return 0;
}


int
posix_spawn_file_actions_addchdir_np(posix_spawn_file_actions_t *_actions,
	const char *path)
{
	struct _posix_spawn_file_actions* actions = _actions != NULL ? *_actions : NULL;

	if (actions == NULL)
		return EINVAL;

	char* npath = strdup(path);
	if (npath == NULL)
		return ENOMEM;
	if (actions->count == actions->size
		&& posix_spawn_file_actions_extend(actions) != 0) {
		free(npath);
		return ENOMEM;
	}

	struct _file_action *action = &actions->actions[actions->count];
	action->type = file_action_chdir;
	action->fd = -1;
	action->action.chdir_action.path = npath;
	actions->count++;
	return 0;
}


int
posix_spawn_file_actions_addfchdir_np(posix_spawn_file_actions_t *_actions,
	int fildes)
{
	struct _posix_spawn_file_actions* actions = _actions != NULL ? *_actions : NULL;

	if (actions == NULL)
		return EINVAL;

	if (fildes < 0 || fildes >= sysconf(_SC_OPEN_MAX))
		return EBADF;

	if (actions->count == actions->size
		&& posix_spawn_file_actions_extend(actions) != 0) {
		return ENOMEM;
	}

	struct _file_action *action = &actions->actions[actions->count];
	action->type = file_action_fchdir;
	action->fd = fildes;
	actions->count++;
	return 0;
}


int
posix_spawnattr_init(posix_spawnattr_t *_attr)
{
	posix_spawnattr_t attr = (posix_spawnattr_t)malloc(
		sizeof(struct _posix_spawnattr));

	if (attr == NULL)
		return ENOMEM;

	memset(attr, 0, sizeof(*attr));
	*_attr = attr;

	return 0;
}


int
posix_spawnattr_destroy(posix_spawnattr_t *_attr)
{
	struct _posix_spawnattr* attr = _attr != NULL ? *_attr : NULL;

	if (attr == NULL)
		return EINVAL;

	free(attr);

	return 0;
}


int
posix_spawnattr_getflags(const posix_spawnattr_t *_attr, short *flags)
{
	struct _posix_spawnattr *attr;

	if (_attr == NULL || (attr = *_attr) == NULL || flags == NULL)
		return EINVAL;

	*flags = attr->flags;

	return 0;
}


int
posix_spawnattr_setflags(posix_spawnattr_t *_attr, short flags)
{
	struct _posix_spawnattr *attr;

	if (_attr == NULL || (attr = *_attr) == NULL)
		return EINVAL;

	if ((flags & ~(POSIX_SPAWN_RESETIDS | POSIX_SPAWN_SETPGROUP
			| POSIX_SPAWN_SETSIGDEF | POSIX_SPAWN_SETSIGMASK)) != 0) {
		return EINVAL;
	}

	attr->flags = flags;

	return 0;
}


int
posix_spawnattr_getpgroup(const posix_spawnattr_t *_attr, pid_t *pgroup)
{
	struct _posix_spawnattr *attr;

	if (_attr == NULL || (attr = *_attr) == NULL || pgroup == NULL)
		return EINVAL;

	*pgroup = attr->pgroup;

	return 0;
}


int
posix_spawnattr_setpgroup(posix_spawnattr_t *_attr, pid_t pgroup)
{
	struct _posix_spawnattr *attr;

	if (_attr == NULL || (attr = *_attr) == NULL)
		return EINVAL;

	attr->pgroup = pgroup;

	return 0;
}


int
posix_spawnattr_getsigdefault(const posix_spawnattr_t *_attr, sigset_t *sigdefault)
{
	struct _posix_spawnattr *attr;

	if (_attr == NULL || (attr = *_attr) == NULL || sigdefault == NULL)
		return EINVAL;

	memcpy(sigdefault, &attr->sigdefault, sizeof(sigset_t));

	return 0;
}


int
posix_spawnattr_setsigdefault(posix_spawnattr_t *_attr,
	const sigset_t *sigdefault)
{
	struct _posix_spawnattr *attr;

	if (_attr == NULL || (attr = *_attr) == NULL || sigdefault == NULL)
		return EINVAL;

	memcpy(&attr->sigdefault, sigdefault, sizeof(sigset_t));

	return 0;
}


int
posix_spawnattr_getsigmask(const posix_spawnattr_t *_attr, sigset_t *sigmask)
{
	struct _posix_spawnattr *attr;

	if (_attr == NULL || (attr = *_attr) == NULL || sigmask == NULL)
		return EINVAL;

	memcpy(sigmask, &attr->sigmask, sizeof(sigset_t));

	return 0;
}


int
posix_spawnattr_setsigmask(posix_spawnattr_t *_attr, const sigset_t *sigmask)
{
	struct _posix_spawnattr *attr;

	if (_attr == NULL || (attr = *_attr) == NULL || sigmask == NULL)
		return EINVAL;

	memcpy(&attr->sigmask, sigmask, sizeof(sigset_t));

	return 0;
}


static int
process_spawnattr(const posix_spawnattr_t *_attr)
{
	if (_attr == NULL)
		return 0;

	struct _posix_spawnattr *attr = *_attr;
	if (attr == NULL)
		return EINVAL;

	if ((attr->flags & POSIX_SPAWN_SETSIGMASK) != 0)
		sigprocmask(SIG_SETMASK, &attr->sigmask, NULL);

	if ((attr->flags & POSIX_SPAWN_SETSIGDEF) != 0) {
		struct sigaction action;
		action.sa_handler = SIG_DFL;
		action.sa_flags = 0;
		action.sa_userdata = NULL;
		sigemptyset(&action.sa_mask);
		for (int i = 1; i <= MAX_SIGNAL_NUMBER; i++) {
			if (sigismember(&attr->sigdefault, i) == 1
				&& sigaction(i, &action, NULL) != 0) {
				return errno;
			}
		}
	}

	if ((attr->flags & POSIX_SPAWN_RESETIDS) != 0) {
		if (setegid(getgid()) != 0)
			return errno;
		if (seteuid(getuid()) != 0)
			return errno;
	}

	if ((attr->flags & POSIX_SPAWN_SETSID) != 0) {
		if (setsid() != 0)
			return errno;
	}

	if ((attr->flags & POSIX_SPAWN_SETPGROUP) != 0) {
		if (setpgid(0, attr->pgroup) != 0)
			return errno;
	}

	return 0;
}


static int
process_file_actions(const posix_spawn_file_actions_t *_actions, int *errfd)
{
	if (_actions == NULL)
		return 0;

	struct _posix_spawn_file_actions* actions = *_actions;
	if (actions == NULL)
		return EINVAL;

	for (int i = 0; i < actions->count; i++) {
		struct _file_action *action = &actions->actions[i];

		if (action->fd == *errfd) {
			int newfd = dup(action->fd);
			if (newfd == -1)
				return errno;
			close(action->fd);
			fcntl(newfd, F_SETFD, FD_CLOEXEC);
			*errfd = newfd;
		}

		if (action->type == file_action_close) {
			if (close(action->fd) != 0)
				return errno;
		} else if (action->type == file_action_open) {
			int fd = open(action->action.open_action.path,
				action->action.open_action.oflag,
				action->action.open_action.mode);
			if (fd == -1)
				return errno;
			if (fd != action->fd) {
				if (dup2(fd, action->fd) == -1)
					return errno;
				if (close(fd) != 0)
					return errno;
			}
		} else if (action->type == file_action_dup2) {
			if (dup2(action->action.dup2_action.srcfd, action->fd) == -1)
				return errno;
		} else if (action->type == file_action_chdir) {
			if (chdir(action->action.chdir_action.path) == -1)
				return errno;
		} else if (action->type == file_action_fchdir) {
			if (fchdir(action->fd) == -1)
				return errno;
		}
	}

	return 0;
}


static int
spawn_using_fork(pid_t *_pid, const char *path,
	const posix_spawn_file_actions_t *actions,
	const posix_spawnattr_t *attrp, char *const argv[], char *const envp[],
	bool envpath)
{
	int err = 0;
	int fds[2];
	pid_t pid;

	if (pipe(fds) != 0)
		return errno;
	if (fcntl(fds[0], F_SETFD, FD_CLOEXEC) != 0
		|| fcntl(fds[1], F_SETFD, FD_CLOEXEC) != 0) {
		goto fail;
	}
	pid = vfork();
	if (pid == -1)
		goto fail;

	if (pid != 0) {
		// parent
		if (_pid != NULL)
			*_pid = pid;
		close(fds[1]);
		read(fds[0], &err, sizeof(err));
		if (err != 0)
			waitpid(pid, NULL, WNOHANG);
		close(fds[0]);
		return err;
	}

	// child
	close(fds[0]);

	err = process_spawnattr(attrp);
	if (err == 0)
		err = process_file_actions(actions, &fds[1]);
	if (err != 0)
		goto fail_child;

	if (envpath)
		execvpe(path, argv, envp != NULL ? envp : environ);
	else
		execve(path, argv, envp != NULL ? envp : environ);

	err = errno;

fail_child:
	write(fds[1], &err, sizeof(err));
	close(fds[1]);
	_exit(127);

fail:
	err = errno;
	close(fds[0]);
	close(fds[1]);
	return err;
}


static int
spawn_using_load_image(pid_t *_pid, const char *_path,
	char *const argv[], char *const envp[], bool envpath)
{
	const char* path;
	// if envpath is specified but the path contains '/', don't search PATH
	if (!envpath || strchr(_path, '/') != NULL) {
		path = _path;
	} else {
		char* buffer = (char*)alloca(B_PATH_NAME_LENGTH);
		status_t status = __look_up_in_path(_path, buffer);
		if (status != B_OK)
			return status;
		path = buffer;
	}

	// count arguments
	int32 argCount = 0;
	while (argv[argCount] != NULL)
		argCount++;

	thread_id thread = __load_image_at_path(path, argCount, (const char**)argv,
		(const char**)(envp != NULL ? envp : environ));
	if (thread < 0)
		return thread;

	*_pid = thread;
	return resume_thread(thread);
}


static int
do_posix_spawn(pid_t *_pid, const char *path,
	const posix_spawn_file_actions_t *actions,
	const posix_spawnattr_t *attrp, char *const argv[], char *const envp[],
	bool envpath)
{
	if (actions == NULL && attrp == NULL) {
		return spawn_using_load_image(_pid, path, argv, envp, envpath);
	} else {
		return spawn_using_fork(_pid, path, actions, attrp, argv, envp,
			envpath);
	}
}


int
posix_spawn(pid_t *pid, const char *path,
	const posix_spawn_file_actions_t *file_actions,
	const posix_spawnattr_t *attrp, char *const argv[], char *const envp[])
{
	return do_posix_spawn(pid, path, file_actions, attrp, argv, envp, false);
}


int
posix_spawnp(pid_t *pid, const char *file,
	const posix_spawn_file_actions_t *file_actions,
	const posix_spawnattr_t *attrp, char *const argv[],
	char *const envp[])
{
	return do_posix_spawn(pid, file, file_actions, attrp, argv, envp, true);
}

