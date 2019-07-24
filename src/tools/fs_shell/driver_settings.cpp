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

#include "fssh_driver_settings.h"

#include <ctype.h>
#include <stdlib.h>

#include "fssh_fcntl.h"
#include "fssh_lock.h"
#include "fssh_os.h"
#include "fssh_stat.h"
#include "fssh_string.h"
#include "fssh_unistd.h"

#include "list.h"


using namespace FSShell;

#define SETTINGS_DIRECTORY "/kernel/drivers/"
#define SETTINGS_MAGIC		'DrvS'

// Those maximum values are independent from the implementation - they
// have been chosen to make the code more robust against bad files
#define MAX_SETTINGS_SIZE	32768
#define MAX_SETTINGS_LEVEL	8

#define CONTINUE_PARAMETER	1
#define NO_PARAMETER 2


typedef struct settings_handle {
	list_link	link;
	char		name[FSSH_B_OS_NAME_LENGTH];
	int32_t		ref_count;
	int32_t		magic;
	struct		fssh_driver_settings settings;
	char		*text;
} settings_handle;


enum assignment_mode {
	NO_ASSIGNMENT,
	ALLOW_ASSIGNMENT,
	IGNORE_ASSIGNMENT
};


static struct list sHandles;
static fssh_mutex sLock;


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
check_handle(void *_handle)
{
	settings_handle *handle = (settings_handle *)_handle;
	if (handle == NULL
		|| handle->magic != SETTINGS_MAGIC)
		return false;

	return true;
}


static fssh_driver_parameter *
get_parameter(settings_handle *handle, const char *name)
{
	int32_t i;
	for (i = handle->settings.parameter_count; i-- > 0;) {
		if (!fssh_strcmp(handle->settings.parameters[i].name, name))
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
	If the word is followed by a newline it will return FSSH_B_OK, if white
	spaces follows, it will return CONTINUE_PARAMETER.
*/
static fssh_status_t
get_word(char **_pos, char **_word, int32_t assignmentMode, bool allowNewLine)
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
		return FSSH_B_BAD_DATA;

	// last character is a backslash
	if (charEscaped)
		return FSSH_B_BAD_DATA;

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
		return FSSH_B_OK;
	}

	// Scan for next beginning word, open brackets, or comment start

	pos++;
	while (true) {
		*_pos = pos;
		if (!pos[0])
			return FSSH_B_NO_ERROR;

		if (is_parameter_separator(pos[0])) {
			// an open bracket '{' could follow after the first
			// newline, but not later
			if (newLine)
				return FSSH_B_NO_ERROR;

			newLine = true;
		} else if (pos[0] == '{' || pos[0] == '}' || pos[0] == '#')
			return FSSH_B_NO_ERROR;
		else if (!isspace(pos[0]))
			return newLine ? FSSH_B_NO_ERROR : CONTINUE_PARAMETER;

		pos++;
	}
}


static fssh_status_t
parse_parameter(struct fssh_driver_parameter *parameter, char **_pos, int32_t level)
{
	char *pos = *_pos;
	fssh_status_t status;

	// initialize parameter first
	fssh_memset(parameter, 0, sizeof(struct fssh_driver_parameter));

	status = get_word(&pos, &parameter->name, NO_ASSIGNMENT, true);
	if (status == CONTINUE_PARAMETER) {
		while (status == CONTINUE_PARAMETER) {
			char **newArray, *value;
			status = get_word(&pos, &value, parameter->value_count == 0
				? ALLOW_ASSIGNMENT : IGNORE_ASSIGNMENT, false);
			if (status < FSSH_B_OK)
				break;

			// enlarge value array and save the value

			newArray = (char**)realloc(parameter->values,
				(parameter->value_count + 1) * sizeof(char *));
			if (newArray == NULL)
				return FSSH_B_NO_MEMORY;

			parameter->values = newArray;
			parameter->values[parameter->value_count++] = value;
		}
	}

	*_pos = pos;
	return status;
}


static fssh_status_t
parse_parameters(struct fssh_driver_parameter **_parameters, int *_count,
	char **_pos, int32_t level)
{
	if (level > MAX_SETTINGS_LEVEL)
		return FSSH_B_LINK_LIMIT;

	while (true) {
		struct fssh_driver_parameter parameter;
		struct fssh_driver_parameter *newArray;
		fssh_status_t status;

		status = parse_parameter(&parameter, _pos, level);
		if (status < FSSH_B_OK)
			return status;

		if (status != NO_PARAMETER) {
			fssh_driver_parameter *newParameter;

			newArray = (fssh_driver_parameter*)realloc(*_parameters, (*_count + 1)
				* sizeof(struct fssh_driver_parameter));
			if (newArray == NULL)
				return FSSH_B_NO_MEMORY;
	
			fssh_memcpy(&newArray[*_count], &parameter, sizeof(struct fssh_driver_parameter));
			newParameter = &newArray[*_count];

			*_parameters = newArray;
			(*_count)++;

			// check for level beginning and end
			if (**_pos == '{') {
				// if we go a level deeper, just start all over again...
				(*_pos)++;
				status = parse_parameters(&newParameter->parameters,
							&newParameter->parameter_count, _pos, level + 1);
				if (status < FSSH_B_OK)
					return status;
			}
		}

		if ((**_pos == '}' && level > 0)
			|| (**_pos == '\0' && level == 0)) {
			// take the closing bracket from the stack
			(*_pos)++;
			return FSSH_B_OK;
		}

		// obviously, something has gone wrong
		if (**_pos == '}' || **_pos == '\0')
			return FSSH_B_ERROR;
	}
}


static fssh_status_t
parse_settings(settings_handle *handle)
{
	char *text = handle->text;

	fssh_memset(&handle->settings, 0, sizeof(struct fssh_driver_settings));

	// empty settings are allowed
	if (text == NULL)
		return FSSH_B_OK;

	return parse_parameters(&handle->settings.parameters,
		&handle->settings.parameter_count, &text, 0);
}


static void
free_parameter(struct fssh_driver_parameter *parameter)
{
	int32_t i;
	for (i = parameter->parameter_count; i-- > 0;)
		free_parameter(&parameter->parameters[i]);

	free(parameter->parameters);
	free(parameter->values);
}


static void
free_settings(settings_handle *handle)
{
	int32_t i;
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

	fssh_strlcpy(handle->name, driverName, sizeof(handle->name));

	if (parse_settings(handle) == FSSH_B_OK)
		return handle;

	free(handle);
	return NULL;
}


static settings_handle *
load_driver_settings_from_file(int file, const char *driverName)
{
	struct fssh_stat stat;

	// Allocate a buffer and read the whole file into it.
	// We will keep this buffer in memory, until the settings
	// are unloaded.
	// The fssh_driver_parameter::name field will point directly
	// to this buffer.

	if (fssh_fstat(file, &stat) < FSSH_B_OK)
		return NULL;

	if (stat.fssh_st_size > FSSH_B_OK && stat.fssh_st_size < MAX_SETTINGS_SIZE) {
		char *text = (char *)malloc(stat.fssh_st_size + 1);
		if (text != NULL && fssh_read(file, text, stat.fssh_st_size) == stat.fssh_st_size) {
			settings_handle *handle;

			text[stat.fssh_st_size] = '\0';
				// make sure the string is null terminated
				// to avoid misbehaviour

			handle = new_settings(text, driverName);
			if (handle != NULL) {
				// everything went fine!
				return handle;
			}
		}
		// "text" might be NULL here, but that's allowed
		free(text);
	}

	return NULL;
}


static bool
put_string(char **_buffer, fssh_ssize_t *_bufferSize, char *string)
{
	fssh_size_t length, reserved, quotes;
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
put_chars(char **_buffer, fssh_ssize_t *_bufferSize, const char *chars)
{
	char *buffer = *_buffer;
	fssh_size_t length;

	if (chars == NULL)
		return true;

	length = fssh_strlen(chars);
	*_bufferSize -= length;

	if (*_bufferSize <= 0)
		return false;

	fssh_memcpy(buffer, chars, length);
	buffer += length;
	buffer[0] = '\0';

	// update the buffer position
	*_buffer = buffer;

	return true;
}


static bool
put_char(char **_buffer, fssh_ssize_t *_bufferSize, char c)
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
put_level_space(char **_buffer, fssh_ssize_t *_bufferSize, int32_t level)
{
	while (level-- > 0)
		put_char(_buffer, _bufferSize, '\t');
}


static void
put_parameter(char **_buffer, fssh_ssize_t *_bufferSize,
	struct fssh_driver_parameter *parameter, int32_t level, bool flat)
{
	int32_t i;

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


static settings_handle *
find_driver_settings(const char *name)
{
	settings_handle *handle = NULL;

	FSSH_ASSERT_LOCKED_MUTEX(&sLock);

	while ((handle = (settings_handle*)list_get_next_item(&sHandles, handle)) != NULL) {
		if (!fssh_strcmp(handle->name, name))
			return handle;
	}

	return NULL;
}


namespace FSShell {

fssh_status_t
driver_settings_init()
{
	fssh_mutex_init(&sLock, "driver settings");
	return FSSH_B_OK;
}

}	// namespace FSShell


//	#pragma mark - public API


fssh_status_t
fssh_unload_driver_settings(void *handle)
{
	if (!check_handle(handle))
		return FSSH_B_BAD_VALUE;

#if 0
	fssh_mutex_lock(&sLock);
	// ToDo: as soon as "/boot" is accessible, we should start throwing away settings
	if (--handle->ref_count == 0) {
		list_remove_link(&handle->link);
	} else
		handle = NULL;
	fssh_mutex_unlock(&sLock);
#endif

	if (handle != NULL)
		free_settings((settings_handle*)handle);

	return FSSH_B_OK;
}


void *
fssh_load_driver_settings(const char *driverName)
{
	settings_handle *handle;
	int file = -1;
	
	if (driverName == NULL)
		return NULL;

	// see if we already have these settings loaded
	fssh_mutex_lock(&sLock);
	handle = find_driver_settings(driverName);
	if (handle != NULL) {
		handle->ref_count++;

		// we got it, now let's see if it already has been parsed
		if (handle->magic != SETTINGS_MAGIC) {
			handle->magic = SETTINGS_MAGIC;

			if (parse_settings(handle) != FSSH_B_OK) {
				// no valid settings, let's cut down its memory requirements
				free(handle->text);
				handle->text = NULL;
				handle = NULL;
			}
		}
		fssh_mutex_unlock(&sLock);
		return handle;
	}

	// open the settings from the standardized location
	if (driverName[0] != '/') {
		char path[FSSH_B_FILE_NAME_LENGTH + 64];

		// This location makes at least a bit sense under BeOS compatible
		// systems.
		fssh_strcpy(path, "/boot/home/config/settings/fs_shell");

		{
			fssh_strlcat(path, SETTINGS_DIRECTORY, sizeof(path));
			fssh_strlcat(path, driverName, sizeof(path));
		}

		file = fssh_open(path, FSSH_O_RDONLY);
	} else
		file = fssh_open(driverName, FSSH_O_RDONLY);

	if (file < FSSH_B_OK) {
		fssh_mutex_unlock(&sLock);
		return NULL;
	}

	handle = load_driver_settings_from_file(file, driverName);

	if (handle != NULL)
		list_add_item(&sHandles, handle);
	fssh_mutex_unlock(&sLock);

	fssh_close(file);
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
fssh_load_driver_settings_from_path(const char *path)
{
	settings_handle *handle;
	int file;

	if (path == NULL)
		return NULL;

	file = fssh_open(path, FSSH_O_RDONLY);
	if (file < FSSH_B_OK)
		return NULL;

	handle = load_driver_settings_from_file(file);

	fssh_close(file);
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
fssh_parse_driver_settings_string(const char *settingsString)
{
	// we simply copy the whole string to use it as our internal buffer
	char *text = fssh_strdup(settingsString);
	if (settingsString == NULL || text != NULL) {
		settings_handle *handle = (settings_handle*)malloc(sizeof(settings_handle));
		if (handle != NULL) {
			handle->magic = SETTINGS_MAGIC;
			handle->text = text;

			if (parse_settings(handle) == FSSH_B_OK)
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
	If the buffer is too small to hold the string, FSSH_B_BUFFER_OVERFLOW
	is returned, and the needed amount of bytes if placed in the
	"_bufferSize" parameter.
	If the "handle" parameter is not a valid driver settings handle, or
	the "buffer" parameter is NULL, FSSH_B_BAD_VALUE is returned.
*/
fssh_status_t
fssh_get_driver_settings_string(void *_handle, char *buffer,
	fssh_ssize_t *_bufferSize, bool flat)
{
	settings_handle *handle = (settings_handle *)_handle;
	fssh_ssize_t bufferSize = *_bufferSize;
	int32_t i;

	if (!check_handle(handle) || !buffer || *_bufferSize == 0)
		return FSSH_B_BAD_VALUE;

	for (i = 0; i < handle->settings.parameter_count; i++) {
		put_parameter(&buffer, &bufferSize, &handle->settings.parameters[i],
			0, flat);
	}

	*_bufferSize -= bufferSize;
	return bufferSize >= 0 ? FSSH_B_OK : FSSH_B_BUFFER_OVERFLOW;
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
fssh_get_driver_boolean_parameter(void *handle, const char *keyName,
	bool unknownValue, bool noArgValue)
{
	fssh_driver_parameter *parameter;
	char *boolean;

	if (!check_handle(handle))
		return unknownValue;

	// check for the parameter
	if ((parameter = get_parameter((settings_handle*)handle, keyName)) == NULL)
		return unknownValue;

	// check for the argument
	if (parameter->value_count <= 0)
		return noArgValue;

	boolean = parameter->values[0];
	if (!fssh_strcmp(boolean, "1")
		|| !fssh_strcasecmp(boolean, "true")
		|| !fssh_strcasecmp(boolean, "yes")
		|| !fssh_strcasecmp(boolean, "on")
		|| !fssh_strcasecmp(boolean, "enable")
		|| !fssh_strcasecmp(boolean, "enabled"))
		return true;

	if (!fssh_strcmp(boolean, "0")
		|| !fssh_strcasecmp(boolean, "false")
		|| !fssh_strcasecmp(boolean, "no")
		|| !fssh_strcasecmp(boolean, "off")
		|| !fssh_strcasecmp(boolean, "disable")
		|| !fssh_strcasecmp(boolean, "disabled"))
		return false;

	// if no known keyword is found, "unknownValue" is returned
	return unknownValue;
}


const char *
fssh_get_driver_parameter(void *handle, const char *keyName,
	const char *unknownValue, const char *noArgValue)
{
	struct fssh_driver_parameter *parameter;

	if (!check_handle(handle))
		return unknownValue;

	// check for the parameter
	if ((parameter = get_parameter((settings_handle*)handle, keyName)) == NULL)
		return unknownValue;

	// check for the argument
	if (parameter->value_count <= 0)
		return noArgValue;

	return parameter->values[0];
}


const fssh_driver_settings *
fssh_get_driver_settings(void *handle)
{
	if (!check_handle(handle))
		return NULL;

	return &((settings_handle *)handle)->settings;
}


fssh_status_t
fssh_delete_driver_settings(void *handle)
{
	return fssh_unload_driver_settings(handle);
}
