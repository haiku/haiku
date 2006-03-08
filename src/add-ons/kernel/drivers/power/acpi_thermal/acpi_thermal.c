/* ++++++++++
    ACPI Generic Thermal Zone Driver. 
    Obtains general status of passive devices, monitors / sets critical temperatures
    Controls active devices.
+++++ */
#include "acpi_thermal_dev.h"
#include "acpi_thermal.h"

acpi_module_info *acpi;

static thermal_dev *device_list = NULL;
static sem_id dev_list_lock = -1;
static int device_count = 0;

static char **device_names = NULL;

/* Enumerates the devices of ACPI_THERMAL_TYPE 
 * looking for devices with _CRT and _TMP methods.
 * Those devices, be they passive or active 
 * are usefull things to monitor / control.
 */
void enumerate_devices (char* parent)
{
	char result[255];
	void *counter = NULL;
	thermal_dev *td = NULL;
	
	acpi_object_type buf;
	size_t bufsize = sizeof(buf);
	
	while (acpi->get_next_entry(ACPI_TYPE_THERMAL, parent, result, 255, &counter) == B_OK) {
		if ((acpi->evaluate_method(result, "_TMP", &buf, bufsize, NULL, 0) == B_OK) && 
		    (acpi->evaluate_method(result, "_CRT", &buf, bufsize, NULL, 0) == B_OK))
		{
			td = (thermal_dev *) malloc(sizeof(thermal_dev));
			if (td != NULL) {
				td->open = 0;
				td->num = device_count;
				
				dprintf("acpi_thermal: Adding thermal device from: %s\n", result);
				td->path = malloc(sizeof(char) * strlen(result) + 1);
				strcpy(td->path, result);
				
				acquire_sem(dev_list_lock);
				td->next = device_list;
				device_list = td;
				device_count++;
				release_sem(dev_list_lock);
			}
		}
		enumerate_devices(result);
	}
}

void
cleanup_published_names(void)
{
	int i;
	
	dprintf("acpi_thermal: cleanup_published_names()\n");
	if (device_names) {
		for (i = 0; device_names[i]; i++){
			free(device_names[i]);
		}
		free(device_names);
	}
}

status_t
init_hardware (void)
{
	return B_OK;
}

status_t
init_driver (void)
{
	if (get_module(B_ACPI_MODULE_NAME, (module_info **)&acpi) < 0) {
		dprintf("Failed to get ACPI module\n");
		return B_ERROR;
	}
	
	if ((dev_list_lock = create_sem(1, "dev_list_lock")) < 0) {
		put_module(B_ACPI_MODULE_NAME);
		return dev_list_lock;
	}
	
	enumerate_devices("\\");
	return B_OK;
}


void
uninit_driver (void)
{
	thermal_dev *td;
	acquire_sem(dev_list_lock);
	td = device_list;
	
	while (td != NULL) {
		device_list = td->next;
		device_count--;
		
		free(td->path);
		free(td);
		td = device_list;
	}
	
	delete_sem(dev_list_lock);
	
	put_module(B_ACPI_MODULE_NAME);
	
	cleanup_published_names();
}


static status_t
acpi_thermal_open (const char *name, uint32 flags, void** cookie)
{
	thermal_dev *td;
	int devnum = atoi(name + strlen(basename));
	
	acquire_sem(dev_list_lock);
	for (td = device_list; td; td = td->next) {
		if (td->num == devnum) {
			dprintf("acpi_thermal: opening ACPI path %s\n", td->path);
			td->open++;
			*cookie = td;
			
			release_sem(dev_list_lock);
			return B_OK;
		}
	}
	release_sem(dev_list_lock);
	
	*cookie = NULL;
	return B_ERROR;
}

static status_t
acpi_thermal_read (void* cookie, off_t position, void *buf, size_t* num_bytes)
{
	int i;
	acpi_thermal_type therm_info;
	if (*num_bytes < 1)
		return B_IO_ERROR;
	
	if (position == 0) {
		dprintf("acpi_thermal: read()\n");
		acpi_thermal_control(cookie, drvOpGetThermalType, &therm_info, 0);
		sprintf((char *)buf, "ACPI Thermal Device %u\n", therm_info.devnum);
		*num_bytes = strlen((char *)buf);
	
		sprintf((char *)buf + *num_bytes, "  Critical Temperature: %lu.%lu K\n", 
				(therm_info.critical_temp / 10), (therm_info.critical_temp % 10));
		*num_bytes = strlen((char *)buf);
		
		sprintf((char *)buf + *num_bytes, "  Current Temperature: %lu.%lu K\n", 
				(therm_info.current_temp / 10), (therm_info.current_temp % 10));
		*num_bytes = strlen((char *)buf);
	
		if (therm_info.hot_temp > 0) {
			sprintf((char *)buf + *num_bytes, "  Hot Temperature: %lu.%lu K\n", 
					(therm_info.hot_temp / 10), (therm_info.hot_temp % 10));
			*num_bytes = strlen((char *)buf);
		}

		if (therm_info.passive_package) {
/* Incomplete. 
     Needs to obtain acpi global lock.
     acpi_object_type needs Reference entry (with Handle that can be resolved)
     what you see here is _highly_ unreliable.
*/
/*      		if (therm_info.passive_package->data.package.count > 0) {
				sprintf((char *)buf + *num_bytes, "  Passive Devices\n");
				*num_bytes = strlen((char *)buf);
				for (i = 0; i < therm_info.passive_package->data.package.count; i++) {
					sprintf((char *)buf + *num_bytes, "    Processor: %lu\n", therm_info.passive_package->data.package.objects[i].data.processor.cpu_id);
					*num_bytes = strlen((char *)buf);
				}
			}
*/
			free(therm_info.passive_package);
		}
	} else {
		*num_bytes = 0;
	}
	
	return B_OK;
}

static status_t
acpi_thermal_write (void* cookie, off_t position, const void* buffer, size_t* num_bytes)
{
	return B_ERROR;
}

static status_t
acpi_thermal_control (void* cookie, uint32 op, void* arg, size_t len)
{
	status_t err = B_ERROR;
	
	thermal_dev *td = (thermal_dev *)cookie;
	char objname[255];
	acpi_thermal_type *att = NULL;
	
	size_t bufsize = sizeof(acpi_object_type);
	acpi_object_type buf;
	
	switch (op) {
		case drvOpGetThermalType: {
			dprintf("acpi_thermal: GetThermalType()\n");
			att = (acpi_thermal_type *)arg;
			
			// Read basic temperature thresholds.
			att->devnum = td->num;
			err = acpi->evaluate_method(td->path, "_CRT", &buf, bufsize, NULL, 0);
			att->critical_temp = buf.data.integer;
			err = acpi->evaluate_method(td->path, "_TMP", &buf, bufsize, NULL, 0);
			att->current_temp = buf.data.integer;
			err = acpi->evaluate_method(td->path, "_HOT", &buf, bufsize, NULL, 0);
			if (err == B_OK) {
				att->hot_temp = buf.data.integer;
			} else {
				att->hot_temp = 0;
			}
			dprintf("acpi_thermal: GotBasicTemperatures()\n");
			
			// Read Passive Cooling devices
			att->passive_package = NULL;
			sprintf(objname, "%s._PSL", td->path);
			err = acpi->get_object(objname, &(att->passive_package));
			
			att->active_count = 0;
			att->active_devices = NULL;
			
			err = B_OK;
			break;
		}
	}
	return err;
}

static status_t
acpi_thermal_close (void* cookie)
{
	return B_OK;
}

static status_t
acpi_thermal_free (void* cookie)
{
	thermal_dev *td = (thermal_dev *)cookie;
	
	acquire_sem(dev_list_lock);
	td->open--;
	release_sem(dev_list_lock);
	
	return B_OK;
}

device_hooks acpi_thermal_hooks = {
	acpi_thermal_open, 			/* -> open entry point */
	acpi_thermal_close, 		/* -> close entry point */
	acpi_thermal_free,			/* -> free cookie */
	acpi_thermal_control, 		/* -> control entry point */
	acpi_thermal_read,			/* -> read entry point */
	acpi_thermal_write,			/* -> write entry point */
	NULL, NULL, NULL, NULL
};

const char**
publish_devices()
{
	thermal_dev *td;
	int i;
	
	dprintf("acpi_thermal: publish_devices()\n");
	
	cleanup_published_names();
	
	acquire_sem(dev_list_lock);
	device_names = (char **) malloc(sizeof(char*) * (device_count + 1));
	if (device_names) {
		for (i = 0, td = device_list; td; td = td->next) {
			if ((device_names[i] = (char *) malloc(strlen(basename) + 4))) {
				sprintf(device_names[i], "%s%d", basename, td->num);
				dprintf("acpi_thermal: Publishing \"/dev/%s\"\n", device_names[i]);
				i++;
			}
		}
		device_names[i] = NULL;
	}
	release_sem(dev_list_lock);
	
	return (const char**) device_names;
}

device_hooks*
find_device(const char* name)
{
	return &acpi_thermal_hooks;
}
