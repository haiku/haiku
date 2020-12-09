#ifndef _FSSH_DRIVER_SETTINGS_H
#define _FSSH_DRIVER_SETTINGS_H


#include "fssh_defs.h"


typedef struct fssh_driver_parameter {
	char *name;
	int	value_count;
	char **values;
	int parameter_count;
	struct fssh_driver_parameter *parameters;
} fssh_driver_parameter;

typedef struct fssh_driver_settings {
	int parameter_count;
	struct fssh_driver_parameter *parameters;
} fssh_driver_settings;


#ifdef __cplusplus
extern "C" {
#endif


extern void*			fssh_load_driver_settings(const char *driverName);
extern fssh_status_t	fssh_unload_driver_settings(void *handle);

extern void*			fssh_parse_driver_settings_string(
							const char *settingsString);
extern fssh_status_t	fssh_get_driver_settings_string(void *_handle,
							char *buffer, fssh_size_t *_bufferSize, bool flat);

extern const char*		fssh_get_driver_parameter(void *handle, const char *key,
							const char *unknownValue,	/* key not present */
							const char *noargValue);	/* key has no value */
extern bool				fssh_get_driver_boolean_parameter(void *handle,
							const char *key, bool unknownValue,
							bool noargValue);

extern const fssh_driver_settings *fssh_get_driver_settings(void *handle);

/* Pass this in as drivername to access safe mode settings */
#define FSSH_B_SAFEMODE_DRIVER_SETTINGS	"/safemode/"

/* Pass this as the key value to check if safe mode is enabled */
#define FSSH_B_SAFEMODE_SAFE_MODE		"safemode"


#ifdef __cplusplus
}
#endif

#endif	// _FSSH_DRIVER_SETTINGS_H
