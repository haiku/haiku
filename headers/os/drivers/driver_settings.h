#ifndef _DRIVER_SETTINGS_H
#define _DRIVER_SETTINGS_H

#include <OS.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct driver_parameter {
	char *name;
	int	value_count;
	char **values;
	int parameter_count;
	struct driver_parameter *parameters;
} driver_parameter;

typedef struct driver_settings {
	int parameter_count;
	struct driver_parameter *parameters;
} driver_settings;

extern void			*load_driver_settings(const char *drivername);
extern status_t		unload_driver_settings(void *handle);
extern const char	*get_driver_parameter(
							void *handle,
							const char *keyname,
							const char *unknownvalue,	/* key not present */
							const char *noargvalue);	/* key has no value */
extern bool			get_driver_boolean_parameter(
							void *handle,
							const char *keyname,
							bool unknownvalue,
							bool noargvalue);

extern const driver_settings   *get_driver_settings(void *handle);

/* Pass this in as drivername to access safe mode settings */
#define B_SAFEMODE_DRIVER_SETTINGS	"/safemode/"

/* Pass this as the key value to check if safe mode is enabled */
#define B_SAFEMODE_SAFE_MODE		"safemode"

#ifdef __cplusplus
}
#endif

#endif	/* _DRIVER_SETTINGS_H */
