/* driver_settings - implements the driver settings API
**
** Initial version by Axel Dörfler, axeld@pinc-software.de
** This file may be used under the terms of the OpenBeOS License.
*/


#include <OS.h>
#include <driver_settings.h>

// this definition is currently missing from the headers above...
// just a small to let it compile fine in the OpenBeOS tree
#ifndef B_FILE_NAME_LENGTH
#	define B_FILE_NAME_LENGTH 256
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>


#define SETTINGS_DIRECTORY "/boot/home/config/settings/kernel/drivers/"
#define SETTINGS_MAGIC		'DrvS'

// Those maximum values are independent from the implementation - they
// have been chosen to make the code more robust against bad files
#define MAX_SETTINGS_SIZE	32768
#define MAX_SETTINGS_LEVEL	8

#define CONTINUE_PARAMETER	1

struct settings_handle {
	void	*first_buffer;
	int32	magic;
	struct	driver_settings settings;
	char	*text;
};

// Basic settings file access functions - since the driver settings API
// has to be able to load the settings files using the BIOS, before the
// kernel is running.
// ToDo: there is currently no way to change the settings ops - the
//	kernel should just call a function at startup.
struct settings_ops {
	int (*open)(const char *name);
	void (*close)(int handle);
	off_t (*filesize)(int handle);
	size_t (*read)(int handle, char *buffer, size_t bufferSize);
};

// The BIOS calls are currently not implemented
// We need to have a stripped-down BFS driver for the bootloader;
// this one would implement the functionality we need here

// ToDo: the code currently assumes a working malloc()/free()
//	for the bootloader code. Most probably this is okay, though

static int bios_open(const char *name);
static void bios_close(int handle);
static off_t bios_filesize(int handle);
static size_t bios_read(int handle, char *buffer, size_t bufferSize);

static const struct settings_ops kBiosOps = {
	bios_open,
	bios_close,
	bios_filesize,
	bios_read,
};

// The kernel calls are used after the kernel has been loaded and
// initialized by the bootloader.

static int kernel_open(const char *name);
static void kernel_close(int handle);
static off_t kernel_filesize(int handle);
static size_t kernel_read(int handle, char *buffer, size_t bufferSize);

static const struct settings_ops kKernelOps = {
	kernel_open,
	kernel_close,
	kernel_filesize,
	kernel_read,
};

// ToDo: the BIOS API must be used at first when we have a real bootloader
static const struct settings_ops *gSettingsOps = &kKernelOps;


static int
bios_open(const char *name)
{
	// not yet implemented
	// has to use a stripped down BFS driver to get down to the file
	return B_ERROR;
}


static void
bios_close(int handle)
{
	// not yet implemented
}


static off_t
bios_filesize(int handle)
{
	// not yet implemented
	return 0;
}


static size_t
bios_read(int handle, char *buffer, size_t bufferSize)
{
	// not yet implemented
	return B_ERROR;
}


static int
kernel_open(const char *name)
{
	char path[B_FILE_NAME_LENGTH + 64];
	strcpy(path, SETTINGS_DIRECTORY);
	strlcat(path, name, sizeof(path));

	return open(path, O_RDONLY);
}


static void
kernel_close(int handle)
{
	close(handle);
}


static off_t
kernel_filesize(int handle)
{
	struct stat stat;
	if (fstat(handle, &stat) < B_OK)
		return B_ERROR;

	return stat.st_size;
}


static size_t
kernel_read(int handle, char *buffer, size_t bufferSize)
{
	return read(handle, buffer, bufferSize);
}


//	#pragma mark -
//	Functions not part of the public API


static inline bool
check_handle(struct settings_handle *handle)
{
	if (handle == NULL
		|| handle->magic != SETTINGS_MAGIC)
		return false;

	return true;
}


static driver_parameter *
get_parameter(struct settings_handle *handle, const char *name)
{
	int32 i;
	for (i = handle->settings.parameter_count; i-- > 0;) {
		if (!strcmp(handle->settings.parameters[i].name, name))
			return &handle->settings.parameters[i];
	}
	return NULL;
}


static status_t
get_word(char **_pos, char **_word, bool allowNewLine)
{
	char *pos = *_pos;
	bool quoted = false, newLine = false, end = false;
	bool escaped = false;

	// Skip any white space and comments
	while (pos[0] && ((allowNewLine && (isspace(pos[0]) || pos[0] == '#'))
		|| (!allowNewLine && (pos[0] == '\t' || pos[0] == ' ')))) {
		// skip any comment lines
		if (pos[0] == '#') {
			while (*pos && *pos != '\n')
				pos++;
		}
		pos++;
	}

	if (pos[0] == '}')
		return B_NO_ERROR;

	// Read in a word - might contain escaped (\) spaces, or it
	// might also be quoted (").

	if (pos[0] == '"') {
		quoted = true;
		pos++;
	}
	*_word = pos;

	while (pos[0]) {
		if (pos[0] == '\\' && pos[1] == ' ') {
			escaped = true;
			pos++;
		} else if (quoted && pos[0] == '\\' && pos[1] == '"') {
			escaped = true;
			pos++;
		} else if (pos[0] == '\n')
			break;
		else if ((!quoted && isspace(pos[0])) || (quoted && pos[0] == '"'))
			break;

		pos++;
	}

	// "String exceeds line" - missing end quote
	if (quoted && pos[0] != '"')
		return B_BAD_DATA;

	end = pos[0] == '\0';
	newLine = pos[0] == '\n' || end;
	pos[0] = '\0';

	// Correct name if there were any escaped characters
	if (escaped) {
		char *word = *_word;
		while (word < pos) {
			if (word[0] == '\\'
				&& ((quoted && word[1] == '"')
					|| (!quoted && word[1] == ' ')))
				memmove(word, word + 1, pos - word);

			word++;
		}
	}

	if (end) {
		*_pos = pos;
		return B_OK;
	}

	// Scan for next beginning word, open brackets, or comment start

	pos++;
	while (true) {
		*_pos = pos;
		if (!pos[0])
			return B_NO_ERROR;

		if (pos[0] == '\n') {
			// an open bracket '{' could follow after the first
			// newline, but not later
			if (newLine)
				return B_NO_ERROR;

			newLine = true;
		} else if (pos[0] == '{' || pos[0] == '}' || pos[0] == '#')
			return B_NO_ERROR;
		else if (!isspace(pos[0]))
			return newLine ? B_NO_ERROR : CONTINUE_PARAMETER;

		pos++;
	}
}


static status_t
parse_parameter(struct driver_parameter *parameter, char **_pos, int32 level)
{
	char *pos = *_pos;
	status_t status;

	// initialize parameter first
	memset(parameter, 0, sizeof(struct driver_parameter));

	status = get_word(&pos, &parameter->name, true);
	if (status == CONTINUE_PARAMETER) {
		while (status == CONTINUE_PARAMETER) {
			char **newArray, *value;
			status = get_word(&pos, &value, false);
			if (status < B_OK)
				break;

			// enlarge value array and save the value

			newArray = realloc(parameter->values, (parameter->value_count + 1) * sizeof(char *));
			if (newArray == NULL)
				return B_NO_MEMORY;

			parameter->values = newArray;
			parameter->values[parameter->value_count++] = value;
		}
	}
	*_pos = pos;
	return status;
}


static status_t
parse_parameters(struct driver_parameter **parameters, int *count, char **pos, int32 level)
{
	if (level > MAX_SETTINGS_LEVEL)
		return B_LINK_LIMIT;

	while (true) {
		struct driver_parameter *newArray, *parameter;
		status_t status;
		newArray = realloc(*parameters, (*count + 1) * sizeof(struct driver_parameter));
		if (newArray == NULL)
			return B_NO_MEMORY;

		parameter = &newArray[*count];

		status = parse_parameter(parameter, pos, level);
		if (status < B_OK)
			return status;

		*parameters = newArray;
		(*count)++;

		// check for level beginning and end
		if (**pos == '{') {
			// if we go a level deeper, just start all over again...
			(*pos)++;
			status = parse_parameters(&parameter->parameters, &parameter->parameter_count, pos, level + 1);
			if (status < B_OK)
				return status;
		}
		if ((**pos == '}' && level > 0)
			|| (**pos == '\0' && level == 0)) {
			// take the closing bracket from the stack
			(*pos)++;
			return B_OK;
		}

		// obviously, something has gone wrong
		if (**pos == '}' || **pos == '\0')
			return B_ERROR;
	}
}


static status_t
parse_settings(struct settings_handle *handle)
{
	char *text = handle->text;

	memset(&handle->settings, 0, sizeof(struct driver_settings));
	return parse_parameters(&handle->settings.parameters, &handle->settings.parameter_count, &text, 0);
}


static void
free_parameter(struct driver_parameter *parameter)
{
	int32 i;
	for (i = parameter->parameter_count; i-- > 0;)
		free_parameter(&parameter->parameters[i]);

	free(parameter->parameters);
	free(parameter->values);
}


static void
free_settings(struct settings_handle *handle)
{
	int32 i;
	for (i = handle->settings.parameter_count; i-- > 0;)
		free_parameter(&handle->settings.parameters[i]);

	free(handle->settings.parameters);

	free(handle->text);
	free(handle);
}


// ToDo: the API to add an item to the driver_settings is obviously accessable
//	to the kernel, so we should provide it, too (in BeOS this is used to add
//	driver settings at boot time, using the safe boot menu).

//static status_t
//add_driver_parameter(const char *name, )
//{
//}


// ToDo: make this function available to the kernel

//static void
//driver_settings_kernel_init(void)
//{
//	// switch the disk access functions to use those from the
//	// kernel rather than those from the BIOS
//	gSettingsOps = &kKernelOps;
//}


//	#pragma mark -
//	The public API implementation


status_t
unload_driver_settings(void *handle)
{
	if (!check_handle(handle))
		return B_BAD_VALUE;

	free_settings(handle);
	return B_OK;
}


void *
load_driver_settings(const char *driverName)
{
	off_t size;
	int file;

	if (driverName == NULL)
		return NULL;

	file = gSettingsOps->open(driverName);
	if (file < B_OK)
		return NULL;

	// Allocate a buffer and read the whole file into it.
	// We will keep this buffer in memory, until the settings
	// are unloaded.
	// The driver_parameter::name field will point directly
	// to this buffer.

	size = gSettingsOps->filesize(file);
	if (size > B_OK && size < MAX_SETTINGS_SIZE) {
		char *text = (char *)malloc(size + 1);
		if (text != NULL && gSettingsOps->read(file, text, size) == size) {
			struct settings_handle *handle = malloc(sizeof(struct settings_handle));
			if (handle != NULL) {
				text[size] = '\0';
				
				handle->magic = SETTINGS_MAGIC;
				handle->text = text;

				if (parse_settings(handle) == B_OK) {
					gSettingsOps->close(file);
					return handle;
				}

				free(handle);
			}
		}
		// "text" might be NULL here, but that's allowed
		free(text);
	}

	gSettingsOps->close(file);
	return NULL;
}


bool
get_driver_boolean_parameter(void *handle, const char *keyName, bool unknownValue, bool noArgValue)
{
	driver_parameter *parameter;
	char *boolean;

	if (!check_handle(handle))
		return unknownValue;

	// check for the parameter
	if ((parameter = get_parameter(handle, keyName)) == NULL)
		return unknownValue;

	// ToDo: This takes just the first argument/value, and checks that one;
	// I don't know if they are used to work that way in BeOS, though.

	// check for the argument
	if (parameter->value_count <= 0)
		return noArgValue;

	boolean = parameter->values[0];
	if (!strcmp(boolean, "1")
		|| !strcasecmp(boolean, "true")
		|| !strcasecmp(boolean, "yes")
		|| !strcasecmp(boolean, "on")
		|| !strcasecmp(boolean, "enable")
		|| !strcasecmp(boolean, "enabled"))
		return true;

	if (!strcmp(boolean, "0")
		|| !strcasecmp(boolean, "false")
		|| !strcasecmp(boolean, "no")
		|| !strcasecmp(boolean, "off")
		|| !strcasecmp(boolean, "disable")
		|| !strcasecmp(boolean, "disabled"))
		return false;

	// if no known keyword is found, "unknownValue" is returned
	return unknownValue;
}


const char *
get_driver_parameter(void *handle, const char *keyName, const char *unknownValue, const char *noArgValue)
{
	struct driver_parameter *parameter;

	if (!check_handle(handle))
		return unknownValue;

	// check for the parameter
	if ((parameter = get_parameter(handle, keyName)) == NULL)
		return unknownValue;

	// check for the argument
	if (parameter->value_count <= 0)
		return noArgValue;

	return parameter->values[0];
}


const driver_settings *
get_driver_settings(void *handle)
{
	if (!check_handle(handle))
		return NULL;

	return &((struct settings_handle *)handle)->settings;
}

