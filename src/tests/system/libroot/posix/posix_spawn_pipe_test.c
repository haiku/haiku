#include "posix_spawn_pipe_test.h"

#include <errno.h>
#include <unistd.h>
#include <spawn.h>
#include <stdio.h>
#include <string.h>

#define panic(n, str) if (n != 0) { perror(str); return 1; }
#define readIdx 0
#define writeIdx 1

int main() {
  int out[2], err[2];
  posix_spawn_file_actions_t fdops;
  pid_t pid;
  char* const argv[] = { "./posix_spawn_pipe_err", NULL };

  panic(pipe(out), "pipe stdout");
  panic(pipe(err), "pipe stderr");

  errno = posix_spawn_file_actions_init(&fdops);
  panic(errno, "init");
  errno = posix_spawn_file_actions_addclose(&fdops, out[readIdx]);
  panic(errno, "close stdout read");
  errno = posix_spawn_file_actions_adddup2(&fdops, out[writeIdx], 1);
  panic(errno, "dup2 stdout write");
  errno = posix_spawn_file_actions_addclose(&fdops, err[readIdx]);
  panic(errno, "close stderr read");
  errno = posix_spawn_file_actions_adddup2(&fdops, err[writeIdx], 2);
  panic(errno, "dup2 stderr write");
  errno = posix_spawn(&pid, "./posix_spawn_pipe_err", &fdops, NULL, argv, NULL);
  panic(errno, "spawn");

  FILE *cOut = fdopen(out[readIdx], "r");
  if (cOut == NULL) panic(-1, "cOut");
  FILE *cErr = fdopen(err[readIdx], "r");
  if (cErr == NULL) panic(-1, "cErr");

  char *buf = NULL;
  size_t bufsize = 0;
  getline(&buf, &bufsize, cOut);
  panic(ferror(cOut), "getline cOut");
  if (strcmp(buf, testOut) != 0) {
    printf("stdout got: %s", buf);
    printf("stdout exp: %s", testOut);
  }
  getline(&buf, &bufsize, cErr);
  panic(ferror(cErr), "getline cErr");
  if (strcmp(buf, testErr) != 0) {
    printf("stderr got: %s", buf);
    printf("stderr exp: %s", testErr);
  }

  return 0;
}
