#include <stdio.h>
#include <fcntl.h>
#include <spawn.h>
#include <errno.h>

#define panic(str) if (ret != 0) { errno = ret; perror(str); return 1; }

int main() {
        int ret;
        pid_t child;
        char* const av[] = { "posix_spawn_redir_err", NULL };
        posix_spawn_file_actions_t child_fd_acts;
        ret = posix_spawn_file_actions_init(&child_fd_acts);
        panic("init");
        ret = posix_spawn_file_actions_addopen(&child_fd_acts, 1, "errlog",
                O_WRONLY | O_CREAT | O_TRUNC, 0644);
        panic("addopen");
        ret = posix_spawn_file_actions_adddup2(&child_fd_acts, 1, 2);
        panic("adddup2");
        ret = posix_spawn(&child, "./posix_spawn_redir_err", &child_fd_acts, NULL, av, NULL);
        panic("spawn");
        return 0;
}

