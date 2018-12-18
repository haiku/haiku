#include <errno.h>
#include <pthread.h>
#include <stddef.h>
#include <stdio.h>

#define panic(n, str) if (n != 0) { perror(str); return 1; }

#define numThreads 8
#define loopCount 10000

__thread int th = 0;

void *thread(void *ignored) {
  th = 1;
  return NULL;
}

int main() {
  pthread_t thr[numThreads];

  for (int i = 0; i < loopCount; i++) {
    for (int j = 0; j < numThreads; j++) {
      errno = pthread_create(&thr[j], NULL, thread, NULL);
      panic(errno, "pthread_create");
    }

    for (int j = 0; j < numThreads; j++) {
      errno = pthread_join(thr[j], NULL);
      panic(errno, "pthread_join");
    }
  }

  return 0;
}
