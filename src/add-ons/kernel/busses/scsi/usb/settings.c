/**
 *
 * TODO: description
 * 
 * This file is a part of USB SCSI CAM for Haiku OS.
 * May be used under terms of the MIT License
 *
 * Author(s):
 * 	Siarzhuk Zharski <imker@gmx.li>
 * 	
 * 	
 */
/** driver settings support implementation */

#include "usb_scsi.h"
#include "settings.h"
//#include "proto_common.h" 

#include <stdlib.h>	/* strtoul */
#include <strings.h> /* strncpy */
#include <driver_settings.h>
#include "tracing.h"

#define DEF_DEVS	2	/**< default amount of reserved devices */
#define S_DEF_DEVS	"2" /**< default amount of reserved devices */
#define DEF_LUNS	4	/**< default amount of reserved LUNs */
#define S_DEF_LUNS	"4" /**< default amount of reserved LUNs */
/** count of device entries, to be reserved for fake devices */
int reserved_devices	= DEF_DEVS;
/** count of Logical Unit Numbers, to be reserved for fake devices */
int reserved_luns		= DEF_LUNS;

bool b_reservation_on  = true;
bool b_ignore_sysinit2 = false;
/**
	support of some kind rudimentary "quick" search. Indexes in
	settings_keys array.
*/
enum SKKeys{
 skkVendor = 0,	
 skkDevice,	 
// skkName,		 
// skkTransport, 
 skkProtocol, 
 skkCommandSet, 
 skkFakeInq, 
 skk6ByteCmd, 
 skkTransTU,
 skkNoTU,
 skkNoGetMaxLUN, 
 skkNoPreventMedia, 
 skkUseModeSense10, 
 skkForceReadOnly, 
 skkProtoBulk,	
 skkProtoCB,	
 skkProtoCBI,
// skkProtoFreecom,
 skkCmdSetSCSI,	
 skkCmdSetUFI,	
 skkCmdSetATAPI,	
 skkCmdSetRBC,	
 skkCmdSetQIC157,	
 
 skkKeysCount,
// skkTransportBase = skkSubClassSCSI, 
 skkProtoBegin = skkProtoBulk, 
 skkProtoEnd	 = skkProtoCBI, 
 skkCmdSetBegin = skkCmdSetSCSI, 
 skkCmdSetEnd	 = skkCmdSetQIC157, 
};
/**
	helper struct, used in our "quick" search algorithm 
*/
struct _settings_key{
	union _hash{
		char name[32];
		uint16 key;
	}hash;
	uint32 property;	
}settings_keys[] = {	/**< array of keys, used in our settings files */
	{{"vendor"	 }, 0}, /* MUST BE SYNCHRONISED WITH skk*** indexes!!! */
	{{"device"	 }, 0},
//	{{"name"		 }, 0},
//	{{"transport"}, 0},
	{{"protocol"}, 0},
	{{"commandset"}, 0},
	{{"fake_inquiry"	}, 0},
	{{"use_rw6_byte_cmd" }, 0},
	{{"trans_test_unit"	}, 0},
	{{"no_test_unit"	}, 0},
	{{"no_get_max_lun"}, 0},
	{{"no_prevent_media"}, 0},
	{{"use_mode_sense_10"}, 0},
	{{"force_read_only"}, 0},
	{{"BULK"	 }, PROTO_BULK_ONLY},
	{{"CB"		 }, PROTO_CB},
	{{"CBI"		}, PROTO_CBI},
//	{{"Freecom"}, PROTO_FREECOM},
	{{"SCSI"	 }, CMDSET_SCSI},
	{{"UFI"		}, CMDSET_UFI},
	{{"ATAPI"	}, CMDSET_ATAPI},
	{{"RBC"		}, CMDSET_RBC},
	{{"QIC157" }, CMDSET_QIC157},
};
/**
	\define:SK_EQUAL
	checks is the __name parameter correspond to value, pointed by __id index
	in settings_keys array. The magic of our "quick" search algorithm =-))	 
*/
#define CAST_SK(__name) (*(uint16 *)(__name))
#define SK_EQUAL(__name, __id) ((CAST_SK(__name) == (settings_keys[__id].hash.key)) && \
																 (0 == strcmp(__name, settings_keys[__id].hash.name)))
/**
	\fn:load_module_settings
	loads driver settings from extarnal settings file through BeOS driver
	settings API. Called on initialization of the module
*/	
void load_module_settings(void)
{
	void *sh = load_driver_settings(MODULE_NAME);
	if(sh){
		load_log_settings(sh);
		/* devices "reservation". Workaround for plug-n-play device attaching*/
		reserved_devices = strtoul(get_driver_parameter(sh, "reserve_devices",
								S_DEF_DEVS, S_DEF_DEVS), NULL, 0);
		reserved_luns	 = strtoul(get_driver_parameter(sh, "reserve_luns",
								S_DEF_LUNS, S_DEF_LUNS), NULL, 0);
		b_ignore_sysinit2 = get_driver_boolean_parameter(sh, "ignore_sysinit2",
								b_ignore_sysinit2, false);
		if(reserved_devices > MAX_DEVICES_COUNT)
			reserved_devices = MAX_DEVICES_COUNT; 
		if(reserved_luns > MAX_LUNS_COUNT)
			reserved_luns = MAX_LUNS_COUNT;
		b_reservation_on = (reserved_devices != 0);
		
		unload_driver_settings(sh);
	} else {
		TRACE("settings:load:file '%s' was not found. Using default setting...\n",
												 MODULE_NAME);
	}	
}
/**
	\fn:strncpy_value
	\param to: buffer for copied string
	\param dp: driver_parameter, from wich copied string come
	\param size: maximal size of copied string 
	copies a string, containing value[0] of this parameter, from driver_parameter,
	pointed by dp, to buffer pointed by to. Semantic of this function is similar
	to standard strncpy() one.		
*/
/*static void
strncpy_value(char *to, driver_parameter *dp, size_t size)
{
	to[0] = 0;
	if(dp->value_count > 0){
		strncpy(to, dp->values[0], size);
	}
}*/
/**
	\fn:parse_transport
	\param dp: driver_parameter, containing device transport information
	\return: a bitmasked value from PROP_-defined flags for USB subclass and \
					 protocol
	parse the transport driver_parameter for known USB subclasses, protocols and
	compose a bitmasked value from those settings	 
*/
static uint32
parse_transport(driver_parameter *dp, int skkBase, int skkEnd,
				uint32 vendor_prop, char *vendor_prop_name)
{
	uint32 ret = 0;
	if(dp->value_count > 0){
		char *value = dp->values[0];
		int skkIdx = skkBase;
		for(; skkIdx <= skkEnd; skkIdx++){
			if(SK_EQUAL(value, skkIdx)){
				ret |= settings_keys[skkIdx].property;
				break;
			}
		} /* for(...) enumerate protocol and commandset keys */
		if(skkIdx > skkEnd){ /* not found - assume vendor prop */
			ret |= vendor_prop;
			strncpy(vendor_prop_name, value, VENDOR_PROP_NAME_LEN);
		}
		if(dp->value_count > 1){
			TRACE("settings:parse_transport:accept '%s', ignore extra...\n", value);
		}
	}	 
	return ret;
}
/**
	\fn:lookup_device_info
	\param product_id: product id of device to be checked for private settings
	\param dp: driver_parameter, containing device information
	\param udd: on return contains name,protocol etc. information about device 
	\return: "true" if private settings for device found - "false" otherwise
	looks through device parameter, pointed by dp, obtains the name and other
	parameters of private device settings if available
*/
static bool
lookup_device_info(uint16 product_id, driver_parameter *dp,
									 usb_device_settings *uds)
{
	bool b_found = false;
	if(dp){
		int i = 0;
		for(; i < dp->value_count; i++){
			uint16 id = strtoul(dp->values[0], NULL, 0) & 0xffff;
			if(product_id == id){
				int prm = 0;
				uds->product_id = product_id;
				for(; prm < dp->parameter_count; prm++){
/*					if(SK_EQUAL(dp->parameters[prm].name, skkName)){
						strncpy_value(udd->product_name, &dp->parameters[prm], INQ_PRODUCT_LEN);
					} else*/
/*					if(SK_EQUAL(dp->parameters[prm].name, skkTransport)){
						udd->properties |= parse_transport(&dp->parameters[prm]);
					} else*/
					if(SK_EQUAL(dp->parameters[prm].name, skkProtocol)){
						uds->properties |= parse_transport(&dp->parameters[prm],
							 skkProtoBegin, skkProtoEnd, 
							 PROTO_VENDOR, uds->vendor_protocol);
					} else
					if(SK_EQUAL(dp->parameters[prm].name, skkCommandSet)){
						uds->properties |= parse_transport(&dp->parameters[prm],
							 skkCmdSetBegin, skkCmdSetEnd, 
							 CMDSET_VENDOR, uds->vendor_commandset);
					} else
					if(SK_EQUAL(dp->parameters[prm].name, skkFakeInq)){
						uds->properties |= FIX_NO_INQUIRY;
					} else
					if(SK_EQUAL(dp->parameters[prm].name, skk6ByteCmd)){
						uds->properties |= FIX_FORCE_RW_TO_6;
					} else
					if(SK_EQUAL(dp->parameters[prm].name, skkTransTU)){
						uds->properties |= FIX_TRANS_TEST_UNIT;
					} else 
					if(SK_EQUAL(dp->parameters[prm].name, skkNoTU)){
						uds->properties |= FIX_NO_TEST_UNIT;
					} else 
					if(SK_EQUAL(dp->parameters[prm].name, skkNoPreventMedia)){
						uds->properties |= FIX_NO_PREVENT_MEDIA;
					} else
					if(SK_EQUAL(dp->parameters[prm].name, skkUseModeSense10)){
						uds->properties |= FIX_FORCE_MS_TO_10;
					} else
					if(SK_EQUAL(dp->parameters[prm].name, skkForceReadOnly)){
						uds->properties |= FIX_FORCE_READ_ONLY;
					} else
					if(SK_EQUAL(dp->parameters[prm].name, skkNoGetMaxLUN)){
						uds->properties |= FIX_NO_GETMAXLUN;
					} else {
						TRACE("settings:device:ignore unknown parameter:%s\n",
																												dp->parameters[prm].name);
					}
				} /* for(...) enumerate device parameters */
				b_found = true;
				break;
			} /* if(product_id == id){ */
		} /*enumerate parameter values (product ids) */
	} /* if(dp) */
	return b_found;
}
/**
	\fn:lookup_vendor_info
	\param vendor_id: vendor id of device to be checked for private settings
	\param product_id: product id of device to be checked for private settings
	\param dp: driver_parameter, containing vendor information
	\param udd: on return contains name, protocol etc. information about device
	\return: "true" if private settings for device found - "false" otherwise
	looks through vendor parameter, pointed by dp, obtains the name of vendor and
	device information if available
*/
static bool
lookup_vendor_info(uint16 vendor_id, uint16 product_id,
				 driver_parameter *dp, usb_device_settings *uds)
{
	bool b_found = false;
	if(dp && dp->value_count > 0 && dp->values[0]){ 
		uint16 id = strtoul(dp->values[0], NULL, 0) & 0xffff;
		if(vendor_id == id){
			int i = 0;
			for( i = 0; i < dp->parameter_count; i++){
				if(!b_found && SK_EQUAL(dp->parameters[i].name, skkDevice)){
					b_found = lookup_device_info(product_id, &dp->parameters[i], uds);
				} /*else
				if(SK_EQUAL(dp->parameters[i].name, skkName)){
					strncpy_value(udd->vendor_name, &dp->parameters[i], INQ_VENDOR_LEN);
				} */else {
					TRACE("settings:vendor:ignore unknown parameter:%s\n",
																											 dp->parameters[i].name);
				}
			} /*for(...) enumerate "vendor" parameters*/
		} /*if(vendor_id == id){*/
	} /* if(dp && ... etc */
	return b_found;
}
/**
	\fn:lookup_device_settings
	\param vendor_id: vendor id of device to be checked for private settings
	\param product_id: product id of device to be checked for private settings
	\param udd: on return contains name,protocol etc. information about device
	\return: "true" if private settings for device found - "false" otherwise
	looks through driver settings file for private device description and load it
	if available into struct pointed by udd	 
*/
bool lookup_device_settings(const usb_device_descriptor *udd,
								usb_device_settings *uds)
{
	bool b_found = false;
	if(uds){
		void *sh = load_driver_settings(MODULE_NAME);
		if(sh){
			const driver_settings *ds = get_driver_settings(sh);
			if(ds){
				int i = 0;
				for(i = 0; i < ds->parameter_count; i++){
					if(SK_EQUAL(ds->parameters[i].name, skkVendor)){
						b_found = lookup_vendor_info(udd->vendor_id,
													 udd->product_id,
											 &ds->parameters[i], uds);
						if(b_found){
							uds->vendor_id = udd->vendor_id;
							break; //we've got it - stop enumeration.
						}	
					}
				} /*for(...) - enumerate "root" parameters*/
			} /* if(ds) */ 
			unload_driver_settings(sh);
		} /* if(sh) */
		if(b_found){
			//TRACE("settings:loaded settings:'%s(%04x)/%s(%04x)/%08x'\n",
			TRACE("settings:loaded settings:'%04x/%04x/%08x'\n",
				/*descr->vendor_name,*/ uds->vendor_id,
				/*descr->product_name,*/
				uds->product_id, uds->properties);
		}
	} /* if(descr)*/
	return b_found;
}

