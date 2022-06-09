#include "atomic.h"

int ffs(int i);
int ffs(int i)
{
	return i ? a_ctz_l(i)+1 : 0;
}
