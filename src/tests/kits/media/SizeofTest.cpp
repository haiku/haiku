//#include "../source/headers/BufferGroup.h"
#include <MediaKit.h>
#include <stdio.h>

int main()
{
	printf("BBuffer sizeof = %ld\n",sizeof(BBuffer));
	printf("BBufferConsumer sizeof = %ld\n",sizeof(BBufferConsumer));
	printf("BBufferGroup sizeof = %ld\n",sizeof(BBufferGroup));
	printf("BBufferProducer sizeof = %ld\n",sizeof(BBufferProducer));
	printf("BContinuousParameter sizeof = %ld\n",sizeof(BContinuousParameter));
	printf("BControllable  sizeof = %ld\n",sizeof(BControllable ));
	printf("BDiscreteParameter  sizeof = %ld\n",sizeof(BDiscreteParameter ));
	printf("BFileInterface sizeof = %ld\n",sizeof(BFileInterface));
	printf("BMediaAddOn sizeof = %ld\n",sizeof(BMediaAddOn));
//	printf("BMediaBufferDecoder sizeof = %ld\n",sizeof(BMediaBufferDecoder));
//	printf("MediaBufferEncoder sizeof = %ld\n",sizeof(MediaBufferEncoder));
//	printf("BMediaDecoder sizeof = %ld\n",sizeof(BMediaDecoder));
//	printf("BMediaEncoder  sizeof = %ld\n",sizeof(BMediaEncoder));
	printf("BMediaEventLooper sizeof = %ld\n",sizeof(BMediaEventLooper));
	printf("BMediaFile sizeof = %ld\n",sizeof(BMediaFile));
	printf("BMediaFiles sizeof = %ld\n",sizeof(BMediaFiles));
	printf("BMediaFiles sizeof = %ld\n",sizeof(BMediaFiles));
	printf("BMediaFormats sizeof = %ld\n",sizeof(BMediaFormats));
	printf("BMediaNode sizeof = %ld\n",sizeof(BMediaNode));
	printf("BMediaRoster sizeof = %ld\n",sizeof(BMediaRoster));
	printf("BMediaTheme sizeof = %ld\n",sizeof(BMediaTheme));
	printf("BMediaTrack sizeof = %ld\n",sizeof(BMediaTrack));
	printf("BNullParameter sizeof = %ld\n",sizeof(BNullParameter));
	printf("BParameter sizeof = %ld\n",sizeof(BParameter));
	printf("BParameterGroup sizeof = %ld\n",sizeof(BParameterGroup));
	printf("BSound sizeof = %ld\n",sizeof(BSound));
	printf("BTimeCode sizeof = %ld\n",sizeof(BTimeCode));
	printf("BParameterWeb sizeof = %ld\n",sizeof(BParameterWeb));
	printf("BSmallBuffer sizeof = %ld\n",sizeof(BSmallBuffer));
	printf("BSoundPlayer sizeof = %ld\n",sizeof(BSoundPlayer ));
	printf("BTimedEventQueue sizeof = %ld\n",sizeof(BTimedEventQueue));
	printf("BTimeSource sizeof = %ld\n",sizeof(BTimeSource));
	return 0;
}