
#include "TimeCode.h"

#include <stdio.h>

int main(int argc, char *argv[]) {

BTimeCode *aBTimeCode;
int32 i,j,k;
char outStr[30];

aBTimeCode = new BTimeCode();

aBTimeCode->SetType(B_TIMECODE_30_DROP_2);

// Test us -> TimeCode -> us
for (i=59000;i<=61000;i++) {
	aBTimeCode->SetMicroseconds(i);
	aBTimeCode->GetString(outStr);
	j = aBTimeCode->Microseconds();
	k = aBTimeCode->LinearFrames();
	printf("%ld = %s = %ld = %ld\n",i,outStr,j,k);
}

// Test frames -> TimeCode -> frames
for (i=8990;i<=8995;i++) {
	aBTimeCode->SetLinearFrames(i);
	aBTimeCode->GetString(outStr);
	j = aBTimeCode->LinearFrames();
	printf("%ld = %s = %ld\n",i,outStr,j);
}

for (i=17981;i<=17990;i++) {
	aBTimeCode->SetLinearFrames(i);
	aBTimeCode->GetString(outStr);
	j = aBTimeCode->LinearFrames();
	printf("%ld = %s = %ld\n",i,outStr,j);
}

for (i=26971;i<=26980;i++) {
	aBTimeCode->SetLinearFrames(i);
	aBTimeCode->GetString(outStr);
	j = aBTimeCode->LinearFrames();
	printf("%ld = %s = %ld\n",i,outStr,j);
}

}
