/*
 * Copyright 2002-2007, Axel Dörfler, axeld@pinc-software.de.
 * This file may be used under the terms of the MIT License.
 */

/*!	\brief Implements the driver settings API
	This file is used by three different components with different needs:
	  1) the boot loader
		Buffers a list of settings files to move over to the kernel - the
		actual buffering is located in the boot loader directly, though.
		Creates driver_settings structures out of those on demand only.
	  2) the kernel
		Maintains a list of settings so that no disk access is required
		for known settings (such as those passed over from the boot
		loader).
	  3) libroot.so
		Exports the parser to userland applications, so that they can
		easily make use of driver_settings styled files.

	The file has to be recompiled for every component separately, so that
	it properly exports the required functionality (which is specified by
	_BOOT_MODE for the boot loader, and _KERNEL_MODE for the kernel).
*/

// The boot loader is compiled with kernel rules, but we want to explicitely
// differentiate between the two here.
#ifdef _BOOT_MODE
#	undef _KERNEL_MODE
#endif

#include <directories.h>
#include <driver_settings.h>
#include <FindDirectory.h>
#include <OS.h>

#ifdef _KERNEL_MODE
#	include <KernelExport.h>
#	include <util/list.h>
#	include <lock.h>
#	include <kdriver_settings.h>
#	include <kernel.h>
#	include <boot/kernel_args.h>
#	include <boot_device.h>
#endif
#ifdef _BOOT_MODE
#	include <boot/kernel_args.h>
#	include <boot/stage2.h>
#endif

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

#ifndef B_BUFFER_OVERFLOW
#	define B_BUFFER_OVERFLOW B_ERROR
#endif

#define SETTINGS_DIRECTORY "/kernel/drivers/"
#define SETTINGS_MAGIC		'DrvS'

// Those maximum values are independent from the implementation - they
// have been chosen to make the code more robust against bad files
#define MAX_SETTINGS_SIZE	32768
#define MAX_SETTINGS_LEVEL	8

#define CONTINUE_PARAMETER	1
#define NO_PARAMETER 2


typedef struct settings_handle {
#ifdef _KERNEL_MODE
	list_link	link;
	char		name[B_OS_NAME_LENGTH];
	int32		ref_count;
#endif
	int32		magic;
	struct		driver_settings settings;
	char		*text;
} settings_handle;


enum assignment_mode {
	NO_ASSIGNMENT,
	ALLOW_ASSIGNMENT,
	IGNORE_ASSIGNMENT
};


#ifdef _KERNEL_MODE
static struct list sHandles;
static mutex sLock = MUTEX_INITIALIZER("driver settings");
#endif


//	#pragma mark - private functions


/*!
	Returns true for any characters that separate parameters -
	those are ignored in the input stream and won't be added
	to any words.
*/
static inline bool
is_parameter_separator(char c)
{
	return c == '\n' || c == ';';
}


/** Indicates if "c" begins a new word or not.
 */

static inline bool
is_word_break(char c)
{
	return isspace(c) || is_parameter_separator(c);
}


static inline bool
check_handle(settings_handle *handle)
{
	if (handle == NULL
		|| handle->magic != SETTINGS_MAGIC)
		return false;

	return true;
}


static driver_parameter *
get_parameter(settings_handle *handle, const char *name)
{
	int32 i;
	for (i = handle->settings.parameter_count; i-- > 0;) {
		if (!strcmp(handle->settings.parameters[i].name, name))
			return &handle->settings.parameters[i];
	}
	return NULL;
}


/*!
	Returns the next word in the input buffer passed in via "_pos" - if
	this function returns, it will bump the input position after the word.
	It automatically cares about quoted strings and escaped characters.
	If "allowNewLine" is true, it reads over comments to get to the next
	word.
	Depending on the "assignmentMode" parameter, the '=' sign is either
	used as a work break, or not.
	The input buffer will be changed to contain the word without quotes
	or escaped characters and adds a terminating NULL byte. The "_word"
	parameter will be set to the beginning of the word.
	If the word is followed by a newline it will return B_OK, if white
	spaces follows, it will return CONTINUE_PARAMETER.
*/
static status_t
get_word(char **_pos, char **_word, int32 assignmentMode, bool allowNewLine)
{
	char *pos = *_pos;
	char quoted = 0;
	bool newLine = false, end = false;
	int escaped = 0;
	bool charEscaped = false;

	// Skip any white space and comments
	while (pos[0]
		&& ((allowNewLine && (isspace(pos[0]) || is_parameter_separator(pos[0])
				|| pos[0] == '#'))
			|| (!allowNewLine && (pos[0] == '\t' || pos[0] == ' '))
			|| (assignmentMode == ALLOW_ASSIGNMENT && pos[0] == '='))) {
		// skip any comment lines
		if (pos[0] == '#') {
			while (pos[0] && pos[0] != '\n')
				pos++;
		}
		pos++;
	}

	if (pos[0] == '}' || pos[0] == '\0') {
		// if we just read some white space before an end of a
		// parameter, this is just no parameter at all
		*_pos = pos;
		return NO_PARAMETER;
	}

	// Read in a word - might contain escaped (\) spaces, or it
	// might also be quoted (" or ').

	if (pos[0] == '"' || pos[0] == '\'') {
		quoted = pos[0];
		pos++;
	}
	*_word = pos;

	while (pos[0]) {
		if (charEscaped)
			charEscaped = false;
		else if (pos[0] == '\\') {
			charEscaped = true;
			escaped++;
		} else if ((!quoted && (is_word_break(pos[0])
				|| (assignmentMode != IGNORE_ASSIGNMENT && pos[0] == '=')))
			|| (quoted && pos[0] == quoted))
			break;

		pos++;
	}

	// "String exceeds line" - missing end quote
	if (quoted && pos[0] != quoted)
		return B_BAD_DATA;

	// last character is a backslash
	if (charEscaped)
		return B_BAD_DATA;

	end = pos[0] == '\0';
	newLine = is_parameter_separator(pos[0]) || end;
	pos[0] = '\0';

	// Correct name if there were any escaped characters
	if (escaped) {
		char *word = *_word;
		int offset = 0;
		while (word <= pos) {
			if (word[0] == '\\') {
				offset--;
				word++;
			}
			word[offset] = word[0];
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

		if (is_parameter_separator(pos[0])) {
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

	status = get_word(&pos, &parameter->name, NO_ASSIGNMENT, true);
	if (status == CONTINUE_PARAMETER) {
		while (status == CONTINUE_PARAMETER) {
			char **newArray, *value;
			status = get_word(&pos, &value, parameter->value_count == 0
				? ALLOW_ASSIGNMENT : IGNORE_ASSIGNMENT, false);
			if (status < B_OK)
				break;

			// enlarge value array and save the value

			newArray = (char**)realloc(parameter->values,
				(parameter->value_count + 1) * sizeof(char *));
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
parse_parameters(struct driver_parameter **_parameters, int *_count,
	char **_pos, int32 level)
{
	if (level > MAX_SETTINGS_LEVEL)
		return B_LINK_LIMIT;

	while (true) {
		struct driver_parameter parameter;
		struct driver_parameter *newArray;
		status_t status;

		status = parse_parameter(&parameter, _pos, level);
		if (status < B_OK)
			return status;

		if (status != NO_PARAMETER) {
			driver_parameter *newParameter;

			newArray = (driver_parameter*)realloc(*_parameters, (*_count + 1)
				* sizeof(struct driver_parameter));
			if (newArray == NULL)
				return B_NO_MEMORY;

			memcpy(&newArray[*_count], &parameter, sizeof(struct driver_parameter));
			newParameter = &newArray[*_count];

			*_parameters = newArray;
			(*_count)++;

			// check for level beginning and end
			if (**_pos == '{') {
				// if we go a level deeper, just start all over again...
				(*_pos)++;
				status = parse_parameters(&newParameter->parameters,
							&newParameter->parameter_count, _pos, level + 1);
				if (status < B_OK)
					return status;
			}
		}

		if ((**_pos == '}' && level > 0)
			|| (**_pos == '\0' && level == 0)) {
			// take the closing bracket from the stack
			(*_pos)++;
			return B_OK;
		}

		// obviously, something has gone wrong
		if (**_pos == '}' || **_pos == '\0')
			return B_ERROR;
	}
}


static status_t
parse_settings(settings_handle *handle)
{
	char *text = handle->text;

	memset(&handle->settings, 0, sizeof(struct driver_settings));

	// empty settings are allowed
	if (text == NULL)
		return B_OK;

	return parse_parameters(&handle->settings.parameters,
		&handle->settings.parameter_count, &text, 0);
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
free_settings(settings_handle *handle)
{
	int32 i;
	for (i = handle->settings.parameter_count; i-- > 0;)
		free_parameter(&handle->settings.parameters[i]);

	free(handle->settings.parameters);
	free(handle->text);
	free(handle);
}


static settings_handle *
new_settings(char *buffer, const char *driverName)
{
	settings_handle *handle = (settings_handle*)malloc(sizeof(settings_handle));
	if (handle == NULL)
		return NULL;

	handle->magic = SETTINGS_MAGIC;
	handle->text = buffer;

#ifdef _KERNEL_MODE
	handle->ref_count = 1;
	strlcpy(handle->name, driverName, sizeof(handle->name));
#endif

	if (parse_settings(handle) == B_OK)
		return handle;

	free(handle);
	return NULL;
}


static settings_handle *
load_driver_settings_from_file(int file, const char *driverName)
{
	struct stat stat;

	// Allocate a buffer and read the whole file into it.
	// We will keep this buffer in memory, until the settings
	// are unloaded.
	// The driver_parameter::name field will point directly
	// to this buffer.

	if (fstat(file, &stat) < B_OK)
		return NULL;

	if (stat.st_size > B_OK && stat.st_size < MAX_SETTINGS_SIZE) {
		char *text = (char *)malloc(stat.st_size + 1);
		if (text != NULL && read(file, text, stat.st_size) == stat.st_size) {
			settings_handle *handle;

			text[stat.st_size] = '\0';
				// make sure the string is null terminated
				// to avoid misbehaviour

			handle = new_settings(text, driverName);
			if (handle != NULL) {
				// everything went fine!
				return handle;
			}

			free(handle);
		}
		// "text" might be NULL here, but that's allowed
		free(text);
	}

	return NULL;
}


static bool
put_string(char **_buffer, ssize_t *_bufferSize, char *string)
{
	size_t length, reserved, quotes;
	char *buffer = *_buffer, c;
	bool quoted;

	if (string == NULL)
		return true;

	for (length = reserved = quotes = 0; (c = string[length]) != '\0'; length++) {
		if (c == '"')
			quotes++;
		else if (is_word_break(c))
			reserved++;
	}
	quoted = reserved || quotes;

	// update _bufferSize in any way, so that we can chain several
	// of these calls without having to check the return value
	// everytime
	*_bufferSize -= length + (quoted ? 2 + quotes : 0);

	if (*_bufferSize <= 0)
		return false;

	if (quoted)
		*(buffer++) = '"';

	for (;(c = string[0]) != '\0'; string++) {
		if (c == '"')
			*(buffer++) = '\\';

		*(buffer++) = c;
	}

	if (quoted)
		*(buffer++) = '"';

	buffer[0] = '\0';

	// update the buffer position
	*_buffer = buffer;

	return true;
}


static bool
put_chars(char **_buffer, ssize_t *_bufferSize, const char *chars)
{
	char *buffer = *_buffer;
	size_t length;

	if (chars == NULL)
		return true;

	length = strlen(chars);
	*_bufferSize -= length;

	if (*_bufferSize <= 0)
		return false;

	memcpy(buffer, chars, length);
	buffer += length;
	buffer[0] = '\0';

	// update the buffer position
	*_buffer = buffer;

	return true;
}


static bool
put_char(char **_buffer, ssize_t *_bufferSize, char c)
{
	char *buffer = *_buffer;

	*_bufferSize -= 1;

	if (*_bufferSize <= 0)
		return false;

	buffer[0] = c;
	buffer[1] = '\0';

	// update the buffer position
	*_buffer = buffer + 1;

	return true;
}


static void
put_level_space(char **_buffer, ssize_t *_bufferSize, int32 level)
{
	while (level-- > 0)
		put_char(_buffer, _bufferSize, '\t');
}


static void
put_parameter(char **_buffer, ssize_t *_bufferSize,
	struct driver_parameter *parameter, int32 level, bool flat)
{
	int32 i;

	if (!flat)
		put_level_space(_buffer, _bufferSize, level);

	put_string(_buffer, _bufferSize, parameter->name);
	if (flat && parameter->value_count > 0)
		put_chars(_buffer, _bufferSize, " =");

	for (i = 0; i < parameter->value_count; i++) {
		put_char(_buffer, _bufferSize, ' ');
		put_string(_buffer, _bufferSize, parameter->values[i]);
	}

	if (parameter->parameter_count > 0) {
		put_chars(_buffer, _bufferSize, " {");
		if (!flat)
			put_char(_buffer, _bufferSize, '\n');

		for (i = 0; i < parameter->parameter_count; i++) {
			put_parameter(_buffer, _bufferSize, &parameter->parameters[i],
				level + 1, flat);

			if (parameter->parameters[i].parameter_count == 0)
				put_chars(_buffer, _bufferSize, flat ? "; " : "\n");
		}

		if (!flat)
			put_level_space(_buffer, _bufferSize, level);
		put_chars(_buffer, _bufferSize, flat ? "}" : "}\n");
	}
}


//	#pragma mark - Kernel only functions


#ifdef _KERNEL_MODE
static settings_handle *
find_driver_settings(const char *name)
{
	settings_handle *handle = NULL;

	ASSERT_LOCKED_MUTEX(&sLock);

	while ((handle = (settings_handle*)list_get_next_item(&sHandles, handle))
			!= NULL) {
		if (!strcmp(handle->name, name))
			return handle;
	}

	return NULL;
}


status_t
driver_settings_init(kernel_args *args)
{
	struct driver_settings_file *settings = args->driver_settings;

	// Move the preloaded driver settings over to the kernel

	list_init(&sHandles);

	while (settings != NULL) {
		settings_handle *handle
			= (settings_handle*)malloc(sizeof(settings_handle));
		if (handle == NULL)
			return B_NO_MEMORY;

		if (settings->size != 0) {
			handle->text = (char*)malloc(settings->size + 1);
			if (handle->text == NULL) {
				free(handle);
				return B_NO_MEMORY;
			}

			memcpy(handle->text, settings->buffer, settings->size);
			handle->text[settings->size] = '\0';
				// null terminate the buffer
		} else
			handle->text = NULL;

		strlcpy(handle->name, settings->name, sizeof(handle->name));
		handle->settings.parameters = NULL;
		handle->settings.parameter_count = 0;
		handle->magic = 0;
			// this triggers parsing the settings when they are actually used

		if (!strcmp(handle->name, B_SAFEMODE_DRIVER_SETTINGS)) {
			// These settings cannot be reloaded, so we better don't throw
			// them away.
			handle->ref_count = 1;
		} else
			handle->ref_count = 0;

		list_add_item(&sHandles, handle);

		settings = settings->next;
	}

	return B_OK;
}
#endif


//	#pragma mark - public API


status_t
unload_driver_settings(void *_handle)
{
	settings_handle *handle = (settings_handle *)_handle;
	if (!check_handle(handle))
		return B_BAD_VALUE;

#ifdef _KERNEL_MODE
	mutex_lock(&sLock);
	if (--handle->ref_count == 0 && gBootDevice > 0) {
		// don't unload an handle when /boot is not available
		list_remove_link(&handle->link);
	} else
		handle = NULL;
	mutex_unlock(&sLock);
#endif

	if (handle != NULL)
		free_settings(handle);

	return B_OK;
}


void *
load_driver_settings(const char *driverName)
{
	settings_handle *handle;
	int file = -1;

	if (driverName == NULL)
		return NULL;

#ifdef _KERNEL_MODE
	// see if we already have these settings loaded
	mutex_lock(&sLock);
	handle = find_driver_settings(driverName);
	if (handle != NULL && handle->ref_count == 0 && gBootDevice > 0) {
		// A handle with a zero ref_count should be unloaded if /boot is
		// available.
		list_remove_link(&handle->link);
		free_settings(handle);
	} else if (handle != NULL) {
		handle->ref_count++;

		// we got it, now let's see if it already has been parsed
		if (handle->magic != SETTINGS_MAGIC) {
			handle->magic = SETTINGS_MAGIC;

			if (parse_settings(handle) != B_OK) {
				// no valid settings, let's cut down its memory requirements
				free(handle->text);
				handle->text = NULL;
				handle = NULL;
			}
		}
		mutex_unlock(&sLock);
		return handle;
	}

	// we are allowed to call the driver settings pretty early in the boot process
	if (gKernelStartup) {
		mutex_unlock(&sLock);
		return NULL;
	}
#endif	// _KERNEL_MODE
#ifdef _BOOT_MODE
	// see if we already have these settings loaded
	{
		struct driver_settings_file *settings = gKernelArgs.driver_settings;
		while (settings != NULL) {
			if (!strcmp(settings->name, driverName)) {
				// we have it - since the buffer is clobbered, we have to
				// copy its contents, though
				char *text = (char*)malloc(settings->size + 1);
				if (text == NULL)
					return NULL;

				memcpy(text, settings->buffer, settings->size + 1);
				return new_settings(text, driverName);
			}
			settings = settings->next;
		}
	}
#endif	// _BOOT_MODE

	// open the settings from the standardized location
	if (driverName[0] != '/') {
		char path[B_FILE_NAME_LENGTH + 64];

#ifdef _BOOT_MODE
		strcpy(path, kUserSettingsDirectory);
#else
		// TODO: use B_COMMON_SETTINGS_DIRECTORY instead!
		if (find_directory(B_USER_SETTINGS_DIRECTORY, -1, false, path,
				sizeof(path)) == B_OK)
#endif
		{
			strlcat(path, SETTINGS_DIRECTORY, sizeof(path));
			strlcat(path, driverName, sizeof(path));
		}

		file = open(path, O_RDONLY);
	} else
		file = open(driverName, O_RDONLY);

	if (file < B_OK) {
#ifdef _KERNEL_MODE
		mutex_unlock(&sLock);
#endif
		return NULL;
	}

	handle = load_driver_settings_from_file(file, driverName);

#ifdef _KERNEL_MODE
	if (handle != NULL)
		list_add_item(&sHandles, handle);
	mutex_unlock(&sLock);
#endif

	close(file);
	return (void *)handle;
}


/** Loads a driver settings file using the full path, instead of
 *	only defining the leaf name (as load_driver_settings() does).
 *	I am not sure if this function is really necessary - I would
 *	probably prefer something like a search order (if it's not
 *	an absolute path):
 *		~/config/settings/kernel/driver
 *		current directory
 *	That would render this function useless.
 */

#if 0
void *
load_driver_settings_from_path(const char *path)
{
	settings_handle *handle;
	int file;

	if (path == NULL)
		return NULL;

	file = open(path, O_RDONLY);
	if (file < B_OK)
		return NULL;

	handle = load_driver_settings_from_file(file);

	close(file);
	return (void *)handle;
}
#endif


/*!
	Returns a new driver_settings handle that has the parsed contents
	of the passed string.
	You can get an empty driver_settings object when you pass NULL as
	the "settingsString" parameter.
*/
void *
parse_driver_settings_string(const char *settingsString)
{
	// we simply copy the whole string to use it as our internal buffer
	char *text = strdup(settingsString);
	if (settingsString == NULL || text != NULL) {
		settings_handle *handle
			= (settings_handle*)malloc(sizeof(settings_handle));
		if (handle != NULL) {
			handle->magic = SETTINGS_MAGIC;
			handle->text = text;

			if (parse_settings(handle) == B_OK)
				return handle;

			free(handle);
		}
		free(text);
	}

	return NULL;
}


/*!
	This function prints out a driver settings structure to a human
	readable string.
	It's either in standard style or the single line style speficied
	by the "flat" parameter.
	If the buffer is too small to hold the string, B_BUFFER_OVERFLOW
	is returned, and the needed amount of bytes if placed in the
	"_bufferSize" parameter.
	If the "handle" parameter is not a valid driver settings handle, or
	the "buffer" parameter is NULL, B_BAD_VALUE is returned.
*/
status_t
get_driver_settings_string(void *_handle, char *buffer, ssize_t *_bufferSize,
	bool flat)
{
	settings_handle *handle = (settings_handle *)_handle;
	ssize_t bufferSize = *_bufferSize;
	int32 i;

	if (!check_handle(handle) || !buffer || *_bufferSize == 0)
		return B_BAD_VALUE;

	for (i = 0; i < handle->settings.parameter_count; i++) {
		put_parameter(&buffer, &bufferSize, &handle->settings.parameters[i],
			0, flat);
	}

	*_bufferSize -= bufferSize;
	return bufferSize >= 0 ? B_OK : B_BUFFER_OVERFLOW;
}


/*!
	Matches the first value of the parameter matching "keyName" with a set
	of boolean values like 1/true/yes/on/enabled/...
	Returns "unknownValue" if the parameter could not be found or doesn't
	have any valid boolean setting, and "noArgValue" if the parameter
	doesn't have any values.
	Also returns "unknownValue" if the handle passed in was not valid.
*/
bool
get_driver_boolean_parameter(void *_handle, const char *keyName,
	bool unknownValue, bool noArgValue)
{
	settings_handle *handle = (settings_handle*)_handle;
	driver_parameter *parameter;
	char *boolean;

	if (!check_handle(handle))
		return unknownValue;

	// check for the parameter
	if ((parameter = get_parameter(handle, keyName)) == NULL)
		return unknownValue;

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
get_driver_parameter(void *_handle, const char *keyName,
	const char *unknownValue, const char *noArgValue)
{
	settings_handle* handle = (settings_handle*)_handle;
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
	if (!check_handle((settings_handle*)handle))
		return NULL;

	return &((settings_handle *)handle)->settings;
}


// this creates an alias of the above function
// unload_driver_settings() is the same as delete_driver_settings()
extern "C" __typeof(unload_driver_settings) delete_driver_settings
	__attribute__((alias ("unload_driver_settings")));

