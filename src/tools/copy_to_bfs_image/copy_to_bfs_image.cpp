/*
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "kprotos.h"
#include "argv.h"

#include <fs_attr.h>


static status_t
copy_file(const char *from, const char *to)
{
    size_t bufferSize = 4 * 1024;
    char buffer[bufferSize];

	int fd = open(from, O_RDONLY);
	if (fd < 0) {
		printf("can't open host file: %s\n", from);
		return fd;
	}

	int bfd = sys_open(1, -1, to, O_RDWR | O_CREAT, S_IFREG | S_IRWXU, 0);
	if (bfd < 0) {
        close(fd);
        printf("error opening: %s\n", to);
        return bfd;
    }

	status_t err = B_OK;
	DIR *attrDirectory;
	dirent_t *attr;

	// copy attributes first!
	if ((attrDirectory = fs_fopen_attr_dir(fd)) != NULL) {
		while ((attr = fs_read_attr_dir(attrDirectory)) != NULL) {
			struct attr_info attrInfo;
			int32 size = bufferSize, bytesRead;

			if (fs_stat_attr(fd, attr->d_name, &attrInfo) != 0)
				continue;

			if (attrInfo.size <= size)
				size = attrInfo.size - 1;
			else
				printf("truncating attribute: %s\n", attr->d_name);

			bytesRead = fs_read_attr(fd, attr->d_name, attrInfo.type, 0, buffer, size);
			if (bytesRead < size) {
				printf("could not read attribute %s: %s\n", attr->d_name, strerror(bytesRead));
				continue;
			}
			buffer[size] = '\0';

		    err = sys_write_attr(1, bfd, attr->d_name, attrInfo.type, buffer, size, 0);
			if (err < B_OK) {
				printf("write attr (\"%s\", type = %p, %p, %ld bytes) failed: %s\n",
					attr->d_name, (void *)attrInfo.type, buffer, size, strerror(err));
				continue;
			}
		}
		fs_close_attr_dir(attrDirectory);
	} else
		puts("could not open attr-dir");

	ssize_t amount;
	while ((amount = read(fd, buffer, bufferSize)) == (ssize_t)bufferSize) {
		err = sys_write(1, bfd, buffer, amount);
		if (err < 0)
			break;
	}

	if (amount && err >= 0)
		err = sys_write(1, bfd, buffer, amount);

	if (err < 0)
		printf("write error: err == %ld, amount == %ld\n", err, amount);

	sys_close(1, bfd);
	close(fd);

	return err;
}


static status_t
copy_dir(const char *fromPath, const char *toPath)
{
    int err = 0;
    char fromName[B_PATH_NAME_LENGTH];
	DIR *from;
	dirent_t *dirent;

	from = opendir(fromPath);
	if (from == NULL) {
		printf("could not open %s\n", fromPath);
		return B_ENTRY_NOT_FOUND;
	}

	while ((dirent = readdir(from)) != NULL) {
		struct stat st;

		if (!strcmp(dirent->d_name, ".") || !strcmp(dirent->d_name, ".."))
			continue;

		strlcpy(fromName, fromPath, B_PATH_NAME_LENGTH);
		if (fromName[strlen(fromName) - 1] != '/')
			strlcat(fromName, "/", B_PATH_NAME_LENGTH);
		strlcat(fromName, dirent->d_name, B_PATH_NAME_LENGTH);

		if (stat(fromName, &st) != 0)
			continue;

		char path[B_PATH_NAME_LENGTH];
		strcpy(path, toPath);
		strcat(path, "/");
		strcat(path, dirent->d_name);

		if (st.st_mode & S_IFDIR) {
			char path[1024];

			if ((err = sys_mkdir(1, -1, path, MY_S_IRWXU)) == B_OK)
				copy_dir(fromName, path);
			else
				printf("Could not create directory %s: (%s)\n", path, strerror(err));
		} else {
			copy_file(fromName, path);
		}
	}
	closedir(from);

	return B_OK;
}


status_t
copy(const char *from, const char *to)
{
	// normalize target path

	char toPath[B_PATH_NAME_LENGTH];
	snprintf(toPath, sizeof(toPath), "/myfs/%s", to);
	
	// choose the correct copy mechanism

	struct stat fromStat;
	if (stat(from, &fromStat) != 0) {
		fprintf(stderr, "Could not stat \"%s\": %s\n", from, strerror(errno));
		return errno;
	}

	struct stat toStat;
	status_t status = sys_rstat(1, -1, toPath, &toStat, 1);
	if (status != B_OK) {
		fprintf(stderr, "Could not stat \"%s\": %s\n", to, strerror(status));
		return status;
	}
	
	if (S_ISDIR(fromStat.st_mode)) {
		if (S_ISREG(toStat.st_mode)) {
			fprintf(stderr, "Target \"%s\" is not a directory\n.", to);
			return B_NOT_A_DIRECTORY;
		}

		return copy_dir(from, toPath);
	} else {
		if (S_ISREG(toStat.st_mode)) {
			// overwrite target file
			return copy_file(from, toPath);
		} else {
			// copy into target directory
			strlcat(toPath, "/", B_PATH_NAME_LENGTH);
			
			const char *fileName = strrchr(from, '/');
			if (fileName == NULL)
				fileName = from;
			else
				fileName++;

			strlcat(toPath, fileName, B_PATH_NAME_LENGTH);

			return copy_file(from, toPath);
		}
	}
}


void
usage(const char *programName)
{
	printf("usage: %s <bfs-image> <source-files ...> <target>\n", programName);
	exit(-1);
}


int
main(int argc, char **argv)
{
	const char *programName = strrchr(argv[0], '/');
	if (programName == NULL)
		programName = argv[0];
	else
		programName++;

	if (argc < 4)
		usage(programName);

	char *image = argv[1];
	const char *target = argv[argc - 1];

	init_fs(image);

	for (int32 i = 2; i < argc - 1; i++) {
		printf("copy \"%s\" to \"%s\"\n", argv[i], target);
		if (copy(argv[i], target) < B_OK)
			break;
	}

    if (sys_unmount(1, -1, "/myfs") != 0) {
        printf("could not un-mount /myfs\n");
        return 5;
    }

    shutdown_block_cache();

    return 0;
}

