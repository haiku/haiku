/*
 * Copyright 2016, Dario Casalinuovo
 * Distributed under the terms of the MIT License.
 */


#include <MediaFile.h>
#include <Url.h>

#include <stdio.h>


int main(int argc, char *argv[])
{
	if (argv[1] == NULL) {
		printf("Specify an URL or a PATH\n");
		return 0;
	}

	printf("Instantiating the BMediaFile\n");

	BUrl url = BUrl(argv[1]);
	if (!url.IsValid()) {
		printf("Invalid URL\n");
		return 0;
	}

	BMediaFile* mediaFile = new BMediaFile(url);
	if (mediaFile->InitCheck() != B_OK) {
		printf("Failed creation of BMediaFile!\n");
		printf("Error: %s\n", strerror(mediaFile->InitCheck()));
	} else {
		printf("Sniffing Success!\n");

		printf("Tracks Detected: %d\n", mediaFile->CountTracks());

		sleep(5);
	}
	delete mediaFile;
	return 0;
}
