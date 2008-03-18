#include <stdio.h>
#include <stdbool.h>

#define uint64 long long unsigned
#define int64 long long

static void
nearest_powers(uint64 value, uint64 *lower, uint64 *upper)
{
        uint64 power = 1UL << 12;
	if (lower)
	        *lower = power;
        while (value >= power) {
		if (lower)
	             *lower = power;
             power <<= 1;
        }
	if (upper)
	       *upper = power;
}


int64 sols[5];
int solCount;
int64 props[5];

static void
find_nearest(uint64 value, int iteration)
{
	if (iteration > 4 || (iteration + 1) >= solCount)
		return;
	uint64 down, up;
	int i;
	nearest_powers(value, &down, &up);
	props[iteration] = down;
	if (value - down < 0x100000) {
		for (i=0; i<=iteration; i++)
			sols[i] = props[i];
		solCount = iteration + 1;
		return;
	}
	find_nearest(value - down, iteration + 1);
	props[iteration] = -up;
	if (up - value < 0x100000) {
		for (i=0; i<=iteration; i++)
			sols[i] = props[i];
		solCount = iteration + 1;
		return;
	}
	find_nearest(up - value, iteration + 1);
}

int
main()
{
	uint64 length = 0xbfee0000; // 0xdfee0000; // 0x9ffb0000; //0xa7f00000; //0x70000000; //0xbfee0000;
	uint64 base = 0;
	solCount = 5;
	find_nearest(length, 0);
	printf("sols ");
	int i;
	for (i=0; i<solCount; i++) {
		printf("0x%Lx ", sols[i]);
	}
	printf("\n");

	bool nextDown = false;
	for (i = 0; i < solCount; i++) {
		if (sols[i] < 0) {
			if (nextDown)
				base += sols[i];
			printf("%Lx %Lx %s\n", base, -sols[i], nextDown ? "UC" : "WB");
			if (!nextDown)
				base -= sols[i];
			nextDown = !nextDown;
		} else {
			if (nextDown)
				base -= sols[i];
			printf("%Lx %Lx %s\n", base, sols[i], nextDown ? "UC" : "WB");
			if (!nextDown)
				base += sols[i];
		}
		
	}
}
