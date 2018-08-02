#include <unistd.h>

int main() {
  const char msg[] = "something";
  write(2, msg, sizeof(msg));
  return 0;
}
