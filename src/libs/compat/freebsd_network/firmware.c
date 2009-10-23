/*
 * Copyright 2009, Colin Günther, coling@gmx.de.
 * All Rights Reserved. Distributed under the terms of the MIT License.
 *
 */


#include <compat/sys/param.h>
#include <compat/sys/firmware.h>
#include <compat/sys/haiku-module.h>

#include <stdlib.h>
#include <string.h>

#include <FindDirectory.h>
#include <StorageDefs.h>
#include <SupportDefs.h>


const struct firmware*
firmware_get(const char* imageName)
{
	int					fileDescriptor;
	struct firmware*	firmware = NULL;
	int32				firmwareFileSize;
	char*				firmwareName;
	char*				firmwarePath;
	ssize_t				readCount;

	firmwarePath = (char*)malloc(B_PATH_NAME_LENGTH);
	if (firmwarePath == NULL)
		goto cleanup0;

	if (find_directory(B_SYSTEM_DATA_DIRECTORY, -1, false,
		firmwarePath, B_PATH_NAME_LENGTH) != B_OK)
		goto cleanup1;

	strlcat(firmwarePath, "/firmware/", B_PATH_NAME_LENGTH);
	strlcat(firmwarePath, gDriverName, B_PATH_NAME_LENGTH);
	strlcat(firmwarePath, "/", B_PATH_NAME_LENGTH);
	strlcat(firmwarePath, imageName, B_PATH_NAME_LENGTH);

	fileDescriptor = open(firmwarePath, B_READ_ONLY);
	if (fileDescriptor == -1)
		goto cleanup1;

	firmwareFileSize = lseek(fileDescriptor, 0, SEEK_END);
	lseek(fileDescriptor, 0, SEEK_SET);

	firmwareName = (char*)malloc(strlen(imageName) + 1);
	if (firmwareName == NULL)
		goto cleanup2;

	strncpy(firmwareName, imageName, strlen(imageName) + 1);

	firmware = (struct firmware*)malloc(sizeof(struct firmware));
	if (firmware == NULL)
		goto cleanup3;

	firmware->data = malloc(firmwareFileSize);
	if (firmware->data == NULL)
		goto cleanup4;

	readCount = read(fileDescriptor, (void*)firmware->data, firmwareFileSize);
	if (readCount == -1)
		goto cleanup5;

	firmware->datasize = firmwareFileSize;
	firmware->name = firmwareName;
	firmware->version = __haiku_firmware_version;
	goto cleanup2;

cleanup5:
	free(&firmware->data);
cleanup4:
	free(firmware);
	firmware = NULL;
cleanup3:
	free(firmwareName);
cleanup2:
	close(fileDescriptor);
cleanup1:
	free(firmwarePath);
cleanup0:
	return firmware;
}


void
firmware_put(const struct firmware* pointer, int flags)
{

}
