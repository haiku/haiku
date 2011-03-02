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
/** Main part of USB SIM implementation */

#include "usb_scsi.h" 

#include <KernelExport.h>
#include <module.h>
#include <malloc.h>
#include <strings.h>
#include <stdio.h>
#include <device_manager.h>
#include <bus/SCSI.h>
#include "device_info.h" 
#include "settings.h" 
#include "transform_procs.h" 
#include "tracing.h" 
#include "scsi_commands.h" 
#include "proto_common.h"
#include "proto_bulk.h" 
#include "proto_cbi.h" 
#include "usb_defs.h" 
#include "fake_device.h" 
#include "sg_buffer.h" 


#if 0
status_t device_added(const usb_device device, void **cookie);
											
status_t device_removed(void *cookie);

static long sim_action(CCB_HEADER *ccbh);
static long sim_init();

#define SIM_VERSION 1
#define HBA_VERSION 1

//#define ROUNDUP(size, seg) (((size) + (seg) - 1) & ~((seg) - 1))

#define INQ_VENDOR_LEN		0x08
#define INQ_PRODUCT_LEN		0x10
#define INQ_REVISION_LEN	0x04

#define TRANS_TIMEOUT		7500000

static long path_id		= -1;
static int32 load_count	= 0;
 
static char sim_vendor_name[]	= "Haiku";		/* who wrote this driver */
static char hba_vendor_name[]	= "USB";		/* who made the hardware */
static char controller_family[]	= "USB SCSI";	/* what family of products */
 
struct usb_support_descriptor supported_devices[] = {
	{0, 0, 0, 0, 0}
};

#define SIZEOF(array) (sizeof(array)/sizeof(array[0])) //?????

usb_device_info *usb_devices[MAX_DEVICES_COUNT];
/* main devices table locking semaphore */
sem_id usb_serial_lock = -1;
 
usb_module_info *usb;
static cam_for_sim_module_info *cam;

struct usb_notify_hooks notify_hooks = {
	device_added,
	device_removed
};

/* function prototupes */
static status_t get_interface_properties(usb_interface_info *uii, uint32 *pproperties);
static status_t load_vendor_module(char **path, const char *path_mask, const char *prop, module_info **mi);
static status_t setup_transport_modules(usb_device_info *udi, usb_device_settings *uds);
static status_t setup_endpoints(usb_interface_info *uii, usb_device_info *udi);
static status_t allocate_resources(usb_device_info *udi);
static void 	release_resources(usb_device_info *udi);
static status_t xpt_scsi_io(CCB_SCSIIO *ccbio);
static status_t xpt_path_inquiry(CCB_PATHINQ *ccbp);
static status_t xpt_extended_path_inquiry(CCB_EXTENDED_PATHINQ *ccbep);

/**
	\fn:match_device
	\param device:???
	\param uii:???
	\param pproperties:???
	\return:B_BAD_TYPE , B_ENTRY_NOT_FOUND, B_OK
	
	??
*/
status_t get_interface_properties(usb_interface_info *uii, uint32 *pproperties)
{
	status_t status = B_BAD_TYPE;
	if(uii->descr->interface_class == USB_DEV_CLASS_MASS){
		status = B_OK;
		switch(uii->descr->interface_subclass){
		case USB_DEV_SUBCLASS_RBC:		*pproperties |= CMDSET_RBC; break;
		case USB_DEV_SUBCLASS_UFI:		*pproperties |= CMDSET_UFI;	break;	 
		case USB_DEV_SUBCLASS_SFF8020I: 
		case USB_DEV_SUBCLASS_SFF8070I: *pproperties |= CMDSET_ATAPI;	break;
		case USB_DEV_SUBCLASS_SCSI:		*pproperties |= CMDSET_SCSI;	break;
		case USB_DEV_SUBCLASS_QIC157:	*pproperties |= CMDSET_QIC157;	break;
		default: 
			TRACE_ALWAYS("get_interface_properties:unknown USB subclass:%02x\n",
									 uii->descr->interface_subclass);
			/*status = B_ENTRY_NOT_FOUND;*/ /*B_BAD_TYPE assumed*/
			break;
		}
		switch(uii->descr->interface_protocol){
		case USB_DEV_PROTOCOL_CBI:	*pproperties |= PROTO_CBI;	break;
		case USB_DEV_PROTOCOL_CB:	*pproperties |= PROTO_CB;	break;
		case USB_DEV_PROTOCOL_BULK:	*pproperties |= PROTO_BULK_ONLY; break;
		default: 
			TRACE_ALWAYS("get_interface_properties:unknown USB protocol:%02x\n",
									 uii->descr->interface_protocol);
			/*status = B_ENTRY_NOT_FOUND;*/ /*B_BAD_TYPE assumed*/
			break;
		}
		if(status == B_OK){
			TRACE("get_interface_properties: standard properties:%08x\n", *pproperties);
		}
	}
	return status;
}
/**
	\fn:
	
*/
status_t load_vendor_module(char **path, const char *path_mask,
							const char *prop, module_info **mi)
{
	status_t status = B_NO_MEMORY;
	int path_len = strlen(path_mask) + strlen(prop);
	*path = malloc(path_len);
	if(*path){
		sprintf(*path, path_mask, prop);
		status = get_module(*path, mi);
		if(status != B_OK)
			TRACE_ALWAYS("load_vendor_module:get_module(%s) failed:%08x\n", *path, status);
	} else {
		TRACE_ALWAYS("load_vendor_module:couldn't allocate %d bytes\n", path_len);
	}
	return status;
}
/**
	\fn:
	
*/
status_t setup_transport_modules(usb_device_info *udi,
									 usb_device_settings *uds)
{
	status_t status = B_OK;
	switch(PROTO(udi->properties)){
	case PROTO_BULK_ONLY:
		udi->protocol_m = &bulk_only_protocol_m;
		break;
	case PROTO_CB:
	case PROTO_CBI:
		udi->protocol_m = &cbi_protocol_m;
		break;
	case PROTO_VENDOR:{
			status = load_vendor_module(&udi->protocol_m_path,
										PROTOCOL_MODULE_MASK,
										uds->vendor_protocol,
										(module_info**)&udi->protocol_m);
		}break;
	default:
		TRACE_ALWAYS("setup_transport_modules: "
					 "transport %02x is not supported\n", PROTO(udi->properties));
		status = B_ENTRY_NOT_FOUND;
	}
	if(status == B_OK){
		switch(CMDSET(udi->properties)){
		case CMDSET_SCSI:
			udi->transform_m = &scsi_transform_m;
			break;
		case CMDSET_UFI:
			udi->transform_m = &ufi_transform_m;
			udi->properties |= FIX_FORCE_MS_TO_10; /* always use 10-byte request */
			break;
		case CMDSET_ATAPI:
			udi->transform_m = &atapi_transform_m;
			udi->properties |= FIX_FORCE_MS_TO_10; /* always use 10-byte request */
			break;
		case CMDSET_RBC:
			udi->transform_m = &rbc_transform_m;
			break;
		case CMDSET_QIC157:
			udi->transform_m = &qic157_transform_m;
			udi->properties |= FIX_FORCE_MS_TO_10; /* always use 10-byte request */
			break;
		case CMDSET_VENDOR:{
				status = load_vendor_module(&udi->transform_m_path,
										TRANSFORM_MODULE_MASK,
										uds->vendor_commandset,
										(module_info**)&udi->transform_m);
			}break;
		default:
			TRACE_ALWAYS("setup_transport_modules: "
						 "protocol %02x is not supported\n", CMDSET(udi->properties));
			status = B_ENTRY_NOT_FOUND;
		}
	}
	return status;
}

static void
release_transport_modules(usb_device_info *udi)
{
	if(PROTO(udi->properties) == PROTO_VENDOR && 0 != udi->protocol_m_path){
		put_module(udi->protocol_m_path);
		udi->protocol_m = 0;
		free(udi->protocol_m_path);
	}
	if(CMDSET(udi->properties) == CMDSET_VENDOR && 0 != udi->transform_m_path){
		put_module(udi->transform_m_path);
		udi->transform_m = 0;
		free(udi->transform_m_path);
	}
}

/**
	\fn:
*/
status_t setup_endpoints(usb_interface_info *uii, usb_device_info *udi)
{
	status_t status = B_OK;
	int16 idx = 0;
	enum{ epIn = 0, epOut, epIntr, epCount };
	size_t epts[epCount] = { -1, -1, -1 };
	char	*epnames[epCount] = {"input", "output", "interrupt"};
	size_t ep = 0;
	for(; ep < uii->endpoint_count; ep++){
		usb_endpoint_descriptor *ed = uii->endpoint[ep].descr;
		TRACE("try endpoint:%d %x %x %x\n", ep, (int32)ed->attributes, (int32)ed->endpoint_address, uii->endpoint[ep].handle);
		if((ed->attributes & USB_EP_ATTR_MASK) == USB_EP_ATTR_BULK){
			if((ed->endpoint_address & USB_EP_ADDR_DIR_IN) == USB_EP_ADDR_DIR_IN){
				epts[epIn]	= ep;
			}else{
				epts[epOut] = ep;
			}
		}else{
			if((ed->attributes & USB_EP_ATTR_MASK) == USB_EP_ATTR_INTERRUPT)
				epts[epIntr] = ep;
		}
	}
	switch(PROTO(udi->properties)){
	case PROTO_CB:
	case PROTO_BULK_ONLY:
		if(epts[epIntr] == -1)
			epts[epIntr] = 0; /* not required for this transports - set it to default*/
		break;
	case PROTO_CBI:
	default:
	}
	for(idx = 0; idx < epCount; idx++){
		if(epts[idx] == -1 && PROTO(udi->properties) != PROTO_VENDOR){
			TRACE_ALWAYS("setup_endpoints: required %s endpoint not found. "
						 "ignore this interface\n", epnames[idx]);
			// DEBUG!!!!!!!!!!
			status = B_ERROR;
		}
	}
	if(status == B_OK){
		udi->pipe_in	= uii->endpoint[epts[epIn]].handle;
		udi->pipe_out	= uii->endpoint[epts[epOut]].handle;
		udi->pipe_intr	= uii->endpoint[epts[epIntr]].handle;
		//TRACE("setup_endpoints: input:%d output:%d "
		//		 "interrupt:%d\n", epts[epIn], epts[epOut], epts[epIntr]);
		TRACE("endpoint:%x %x %x\n", udi->pipe_in, udi->pipe_out, udi->pipe_intr);
	}
	return status;
}
/**
	\fn:create_device_info
	\param device:???
	\param ppudi:???
	\return:???
	
	???
*/
status_t allocate_resources(usb_device_info *udi)
{
	char name[32];
	status_t status = B_NO_MEMORY;
	uint16 dev = 0;
	acquire_sem(usb_serial_lock);
	for(; dev < MAX_DEVICES_COUNT; dev++){
		if(!usb_devices[dev])
			break;
	}
	if(dev < MAX_DEVICES_COUNT){
		usb_devices[dev] = udi;
		udi->lock_sem =
		udi->trans_sem = -1;
		sprintf(name, "usb_scsi lock_sem:%d", dev);
		if((udi->lock_sem = create_sem(0, name)) >= 0){
			sprintf(name, "usb_scsi trans_sem:%d", dev);
			if((udi->trans_sem = create_sem(0, name))>=0){
				udi->dev_num = dev;
				release_sem(udi->lock_sem);
				status = B_OK;
			}else 
				status = udi->trans_sem;
		} else 
			status = udi->lock_sem;
			
		if(status != B_OK){
			TRACE_ALWAYS("allocate_resources:error:%s", strerror(status));
			if(udi->lock_sem >= 0)
				delete_sem(udi->lock_sem);
			if(udi->trans_sem >= 0)
				delete_sem(udi->trans_sem);
			usb_devices[dev] = NULL;
			free(udi);
		}
	}else{
		TRACE_ALWAYS("allocate_resources:reserved devices space exhausted."
								 "Unplug unnesesary devices or reconfigure this driver.\n");
	}
	release_sem(usb_serial_lock);
	return status;
}
/**
	\fn:
	
*/
void release_resources(usb_device_info *udi)
{
	release_transport_modules(udi);
	udi->usb_m = 0;
	(*usb->cancel_queued_transfers)(udi->pipe_in);
	(*usb->cancel_queued_transfers)(udi->pipe_out);
	delete_sem(udi->lock_sem);
	delete_sem(udi->trans_sem);
	acquire_sem(usb_serial_lock);
	usb_devices[udi->dev_num] = NULL;
	release_sem(usb_serial_lock);
}
/**
	\fn:device_added
	\param device:??
	\param cookie:??
	\return:??
	
	??
*/
status_t device_added(const usb_device device, void **cookie){
	status_t status = B_NO_MEMORY;
	const usb_configuration_info *uci = NULL;
	uint16 cfg = 0;
	bool b_found = false;
	bool b_has_extra_settings = false;
	const usb_device_descriptor *udd = (*usb->get_device_descriptor)(device);
	usb_device_info *udi = (usb_device_info *)malloc(sizeof(usb_device_info));
	TRACE("device_added: probing canidate: "
		"%04x/%04x %02d/%02d/%02d\n", 
			udd->vendor_id, udd->product_id,
			udd->device_class, 
			udd->device_subclass, 
			udd->device_protocol);
	if(udi){
		status = B_NO_INIT;
		while(!b_found && (uci = (*usb->get_nth_configuration)(device, cfg++))){
			uint16 itf = 0;
			for(; itf < uci->interface_count && !b_found; itf++){
				usb_interface_list *ifl = &uci->interface[itf];
				uint16 alt = 0;
				for(; alt < ifl->alt_count; alt++){
					usb_interface_info *uii = &ifl->alt[alt];
					usb_device_settings ud_settings;
					memset(udi, 0, sizeof(usb_device_info));
					memset(&ud_settings, 0, sizeof(usb_device_settings));
					udi->device = device;
					udi->interface	= itf;
					b_has_extra_settings = lookup_device_settings(udd, &ud_settings);
					switch(get_interface_properties(uii, &udi->properties)){
					case B_BAD_TYPE: /* non-standard USB class*/
						if(!b_has_extra_settings){
							continue; /* skip to next interface */
						} /* no break - fall through */
					case B_OK:
						if(b_has_extra_settings){
							if(PROTO(ud_settings.properties) != PROTO_NONE){
								udi->properties &= ~PROTO_MASK;
								udi->properties |= PROTO(ud_settings.properties);
							}
							if(CMDSET(ud_settings.properties) != CMDSET_NONE){
								udi->properties &= ~CMDSET_MASK;
								udi->properties |= CMDSET(ud_settings.properties);
							}
							udi->properties |= ud_settings.properties & FIX_MASK;
							TRACE("device_added: properties merged:%08x\n", udi->properties);
						}
						if( B_OK == setup_transport_modules(udi, &ud_settings)){
							break;
						} /* else - no break - fall through */
					default: 
						continue; /* skip to next interface */
					}
					if(alt != 0){ /*TODO: are we need this ???*/
						if((status = (*usb->set_alt_interface)(device, uii)) != B_OK){
							TRACE_ALWAYS("device_added:setting alt interface failed:%s",
												strerror(status));
							goto Failed;/* Break - is it right?*/
						}	
					}
					if((*usb->get_configuration)(device) != uci){
						if((status = (*usb->set_configuration)(device, uci)) != B_OK){
								TRACE_ALWAYS("device_added:setting configuration failed:%08x uci: %08x\n",
											 (*usb->get_configuration)(device), uci);
							TRACE_ALWAYS("device_added:setting configuration failed:%s\n",
													strerror(status));
			
							goto Failed;/* Break - is it right?*/
						}
					}
					if(B_OK != setup_endpoints(uii, udi)){
						continue; /* skip to next interface */
					}
					if((status = allocate_resources(udi)) == B_OK){
						udi->b_trace = b_log_protocol;
						udi->trace = usb_scsi_trace;
						udi->trace_bytes = usb_scsi_trace_bytes;
						udi->trans_timeout = TRANS_TIMEOUT; 
						udi->usb_m = usb;
						udi->not_ready_luns = 0xff; /*assume all LUNs initially not ready */
						if((status = (*udi->protocol_m->init)(udi)) == B_OK){
							TRACE("device_added[%d]: SUCCESS! Enjoy using!\n", udi->dev_num);
							*cookie = udi;
							b_found = true; /* we have found something useful - time to go out! */
							break;					/* ... now break alternatives iteration.*/
						} else { 
							release_resources(udi);
						}
					}
					/* go to next iteration - check all configurations for possible devices */
				}/* for(...) iterate interface alternates*/
			}/* for(...) iterate interfaces*/
		}/* while(...) iterate configurations */
		if(status == B_OK){
			(*cam->minfo.rescan)();
		} else {
			free(udi);
		}
	} /* if(udi){ */
	if(status != B_OK){
		TRACE("device_added: probing failed (%s) for: %04x/%04x\n",
				strerror(status), udd->vendor_id, udd->product_id);
	}	
Failed:	
	return status;
}
/**
	\fn:device_removed
	\param cookie:???
	\return:???
	
	???
*/
status_t device_removed(void *cookie)
{
	status_t status = B_OK;
	usb_device_info *udi = (usb_device_info *)cookie;
	acquire_sem(udi->lock_sem); /* wait for possible I/O operation complete */	
	release_resources(udi);
	/* no corresponding call of release_sem(udi->lock_sem);
		 - semaphore was deleted in release_resources, any waiting thread
		   was failed with BAD_SEM_ID. 
	*/
	TRACE_ALWAYS("device_removed[%d]:All The Best !!!\n", udi->dev_num);
	free(udi);
	(*cam->minfo.rescan)();
	return status;
}
/**
	\fn:sim_init
	\return: ???
	
	called on SIM init
*/
static long sim_init(void)
{
	status_t status = B_OK;
	TRACE("sim_init\n");
	return status;
}

/**
*/
static bool
pre_check_scsi_io_request(usb_device_info *udi, CCB_SCSIIO *ccbio,
												status_t *ret_status)
{
	int target_id	 = ccbio->cam_ch.cam_target_id;
	uint8 target_lun = ccbio->cam_ch.cam_target_lun;
	*ret_status 	 = B_OK;
	/* handle reserved device and luns entries */
	if(b_reservation_on && udi == NULL && 
		 target_id < reserved_devices &&
		 target_lun < reserved_luns)
	{
		*ret_status = fake_scsi_io(ccbio);
		return false;
	}
	/* no device for this target | LUN */
	if(udi == NULL || target_lun > udi->max_lun){
		ccbio->cam_ch.cam_status = CAM_DEV_NOT_THERE;
		*ret_status = B_DEV_BAD_DRIVE_NUM;
		return false;
	}
	/* check command length */
	if(ccbio->cam_cdb_len != 6	&&
		 ccbio->cam_cdb_len != 10 &&
		 ccbio->cam_cdb_len != 12)
	{
		TRACE("Bad SCSI command length:%d.Ignore\n", ccbio->cam_cdb_len);
		ccbio->cam_ch.cam_status = CAM_REQ_INVALID;
		*ret_status = B_BAD_VALUE;
		return false;
	}
	/* Clean up auto sense buffer.
		 To avoid misunderstanding in not ready luns logic detection.*/
	if(NULL == ccbio->cam_sense_ptr){
		memset(&udi->autosense_data, 0, sizeof(udi->autosense_data));
	} else {
		memset(ccbio->cam_sense_ptr, 0, ccbio->cam_sense_len);
	} 
	return true;
}
/**
*/
static bool
pre_handle_features(usb_device_info *udi, CCB_SCSIIO *ccbio,
					scsi_cmd_generic *command, sg_buffer *sgb,
											status_t *ret_status)
{
	uint8 target_lun = ccbio->cam_ch.cam_target_lun;
	*ret_status = B_OK;
	udi->trans_timeout = TRANS_TIMEOUT; 
	switch(command->opcode){
	case MODE_SELECT_6:
	case MODE_SENSE_6:{
		bool b_select = (MODE_SELECT_6 == command->opcode);
		const char*cmd_name = b_select ? "MODE_SELECT" : "MODE_SENSE";
		if(udi->not_ready_luns & (1 << target_lun)){
			TRACE("pre_handle_features:%s_6 bypassed for LUN:%d\n", cmd_name, target_lun);
			goto set_REQ_INVALID_and_return;
		}
		if(HAS_FIXES(udi->properties, FIX_FORCE_MS_TO_10)){
			if( B_OK != realloc_sg_buffer(sgb, ccbio->cam_dxfer_len + 4)){ /* TODO: 4 - hardcoded? */
				TRACE_ALWAYS("pre_handle_features:error allocating %d bytes for %s_10\n",
								 ccbio->cam_dxfer_len + 4, cmd_name);
				goto set_REQ_INVALID_and_return;
			}
			if(b_select){
				sg_buffer sgb_sense_6;
				/*TODO: implemenet and try refragment_sg_buffer - to check handling of real scatter/gather!!*/
				if(B_OK != init_sg_buffer(&sgb_sense_6, ccbio)){
					/* In case of error TRACE-ing was already performed in sg_ functions */
					goto set_REQ_INVALID_and_return;
				}
				TRACE_MODE_SENSE_SGB("MODE SELECT 6:", &sgb_sense_6);
				if(B_OK != sg_memcpy(sgb, 1, &sgb_sense_6, 0, 3) ||
					 B_OK != sg_memcpy(sgb, 7, &sgb_sense_6, 3, 1) ||	 
					 B_OK != sg_memcpy(sgb, 8, &sgb_sense_6, 4, ccbio->cam_dxfer_len - sizeof(scsi_mode_param_header_6)))
				{
					/* In case of error TRACE-ing was already performed in sg_ functions */
					goto set_REQ_INVALID_and_return;
				}
				TRACE_MODE_SENSE_SGB("MODE SELECT 10:", sgb);
			} 
		} /*else {
			if(b_select){ / * trace data if configured in settings * /
				TRACE_MODE_SENSE_DATA("MODE_SELECT:", ccbio->cam_data_ptr, ccbio->cam_dxfer_len);
			}	
		}*/
	}break;
	case INQUIRY: /* fake INQUIRY request */
		if(HAS_FIXES(udi->properties, FIX_NO_INQUIRY)){
			fake_inquiry_request(udi, ccbio);
			goto set_REQ_CMP_and_return;
		} break;
	case TEST_UNIT_READY: /* fake INQUIRY request */
		if(HAS_FIXES(udi->properties, FIX_NO_TEST_UNIT)){
			goto set_REQ_CMP_and_return;
		} break;
	case PREVENT_ALLOW_MEDIA_REMOVAL: /* fake PREVENT_ALLOW_MEDIA_REMOVAL request */
		if(HAS_FIXES(udi->properties, FIX_NO_PREVENT_MEDIA)){
			goto set_REQ_CMP_and_return;
		} break;
	case FORMAT_UNIT:
		udi->trans_timeout = B_INFINITE_TIMEOUT;
		break;
	default: break;
	}
	return true;
	
set_REQ_CMP_and_return:
	ccbio->cam_ch.cam_status = CAM_REQ_CMP;
	return false;//*ret_status = B_OK;

set_REQ_INVALID_and_return:
	ccbio->cam_ch.cam_status = CAM_REQ_INVALID;
	*ret_status = B_BAD_VALUE;
	return false;
}
/**
*/
static bool
post_handle_features(usb_device_info *udi, CCB_SCSIIO *ccbio,
					scsi_cmd_generic *command, sg_buffer *sgb,
												status_t *ret_status)
{
	bool b_cmd_ok = (ccbio->cam_ch.cam_status == CAM_REQ_CMP);
	uint8 target_lun = ccbio->cam_ch.cam_target_lun;
	switch(command->opcode){
	case READ_6: 
	case WRITE_6:
#if 0	 
/* Disabled - single problem can switch to 6-bytes mode. If device doesn't
	 support 6-bytes command all goes totally wrong. That's bad. */	
		if(!b_cmd_ok && !HAS_FEATURES(udi->descr.properties, PROP_FORCE_RW_TO_6)){
			TRACE("post_handle_features:READ(10)/WRITE(10) failed - retry 6-byte one\n");
			udi->descr.properties |= PROP_FORCE_RW_TO_6;
			ccbio->cam_scsi_status = SCSI_STATUS_OK; /* clear the scsi_status. */
			ccbio->cam_ch.cam_status = CAM_REQ_INPROG; /* set status in progress again. */
			b_retry = true; /* inform caller about retry */
		}
#endif
		break;
	case MODE_SENSE_6:
		if(!b_cmd_ok && !HAS_FIXES(udi->properties, FIX_FORCE_MS_TO_10)){
			TRACE("post_handle_features:MODE SENSE(6) failed - retry 10-byte one\n");
			udi->properties |= FIX_FORCE_MS_TO_10;
			ccbio->cam_scsi_status = SCSI_STATUS_OK; /* clear the scsi_status. */
			ccbio->cam_ch.cam_status = CAM_REQ_INPROG; /* set status in progress again. */
			return true; /* inform caller about retry */
		}	/* no break, - fallthrough! */
	case MODE_SELECT_6:{
		if(MODE_SENSE_6 == command->opcode){
			sg_buffer sgb_sense_6;
			if(B_OK != init_sg_buffer(&sgb_sense_6, ccbio)){
				TRACE_ALWAYS("post_hanlde_features: initialize sgb failed\n");
				goto set_REQ_INVALID_and_return;
			}
			/* convert sense information from 10-byte request result to 6-byte one */
			if(HAS_FIXES(udi->properties, FIX_FORCE_MS_TO_10)){
				uchar mode_data_len_msb = 0, block_descr_len_msb = 0;
				if( B_OK != sg_access_byte(sgb, 0, &mode_data_len_msb, false) ||
						B_OK != sg_access_byte(sgb, 6, &block_descr_len_msb, false) ||
							 0 != mode_data_len_msb || 0 != block_descr_len_msb)
				{
					/* In case of error TRACE-ing was already performed in sg_ functions */
					TRACE_ALWAYS("MODE_SENSE 10->6 conversion overflow: %d, %d\n",
								 mode_data_len_msb, block_descr_len_msb);
					goto set_REQ_INVALID_and_return;
				}
				TRACE_MODE_SENSE_SGB("MODE SENSE 10:", sgb);
				if( B_OK != sg_memcpy(&sgb_sense_6, 0, sgb, 1, 3) ||
						B_OK != sg_memcpy(&sgb_sense_6, 3, sgb, 7, 1) ||	 
						B_OK != sg_memcpy(&sgb_sense_6, 4, sgb, 8, ccbio->cam_dxfer_len - sizeof(scsi_mode_param_header_6)))
				{
					/* In case of error TRACE-ing was already performed in sg_ functions */
					TRACE_ALWAYS("MODE_SENSE 10->6 conversion failed\n");
					goto set_REQ_INVALID_and_return;
				}		
			}
			/* set write-protected flag if required by user */
			if(HAS_FIXES(udi->properties, FIX_FORCE_READ_ONLY)){
				status_t status = B_OK;
				uchar device_spec_params = 0;
				if(B_OK == (status = sg_access_byte(sgb, 2, &device_spec_params, false))){
					device_spec_params |= 0x80;
					status = sg_access_byte(sgb, 2, &device_spec_params, true);
				}
				if(B_OK != status){
					TRACE_ALWAYS("MODE_SENSE set READ-ONLY mode failed. Writing ALLOWED!\n");
					/*goto set_req_invalid_and_return;*/ /* not urgent. do not fail processing...	*/
				}
			}
			TRACE_MODE_SENSE_SGB("MODE SENSE 6:", &sgb_sense_6);
		}
		if(HAS_FIXES(udi->properties, FIX_FORCE_MS_TO_10)){
			free_sg_buffer(sgb);
		}
	} break;
	case TEST_UNIT_READY: { /* set the not ready luns flag */
			scsi_sense_data *sense_data = (ccbio->cam_sense_ptr == NULL) ?
				&udi->autosense_data : (scsi_sense_data *)ccbio->cam_sense_ptr;
			if((sense_data->flags & SSD_KEY) == SSD_KEY_NOT_READY){
				udi->not_ready_luns |= (1 << target_lun);
			} else {
				udi->not_ready_luns &= ~(1 << target_lun);
			}
			usb_scsi_trace_bytes("NOT_READY_LUNS:", &udi->not_ready_luns, 1);
		} break;
	case READ_CAPACITY:{
		/*uint8 *bts = sgb->piov->iov_base;
		uint32 capacity = (bts[0]<<24) + (bts[1]<<16) + (bts[2]<<8) + (bts[3]);
		TRACE_ALWAYS("CAPAC:%d\n", capacity);
		//bts[3] -= 3;*/
		TRACE_CAPACITY("READ_CAPACITY:", sgb);
	} break;
	default: break;
	}
	return false; /* do not retry - all is OK */ //b_retry;

set_REQ_INVALID_and_return:
	ccbio->cam_ch.cam_status = CAM_REQ_INVALID;
	*ret_status = B_BAD_VALUE;
	return false; /* do not retry - fatal error */	
}
/**
	\fn:xpt_scsi_io
	\param ccbio: ????
	\return: ???
		
	xpt_scsi_io - handle XPT_SCSI_IO sim action 
*/
status_t xpt_scsi_io(CCB_SCSIIO *ccbio)
{
	status_t status			= B_OK;
	usb_device_info *udi = usb_devices[ccbio->cam_ch.cam_target_id/*target_id*/];
	uint8 *cmd;
	sg_buffer sgb;

	/* clear the scsi_status. It can come from system with some garbage value ...*/
	ccbio->cam_scsi_status = SCSI_STATUS_OK;
	/* check the request for correct parameters, valid targets, luns etc ... */
	if(false == pre_check_scsi_io_request(udi, ccbio, &status)){
		return status;
	}

#if 0 /* activate if you need detailed logging of CCB_SCSI request*/
	usb_scsi_trace_CCB_SCSIIO(ccbio); 
#endif


	/* acquire semaphore - avoid re-enters */
/*	if((status = acquire_sem_etc(udi->lock_sem, 1,	
								B_RELATIVE_TIMEOUT,
								udi->trans_timeout)) != B_OK) */
//	TRACE_ALWAYS("sem before acq:%08x\n",udi->lock_sem);
	if((status = acquire_sem(udi->lock_sem)) != B_OK){
		/* disabled - CAM_BUSY flag is not recognized by BeOS ... :-(
		if(status == B_WOULD_BLOCK){
			TRACE("locked sema bypass OK\n");
			ccbio->cam_ch.cam_status = CAM_BUSY;	
			return B_OK;
		}*/ 
		TRACE("xpt_scsi_io:acquire_sem_etc() failed:%08x\n", status);
		ccbio->cam_ch.cam_status = CAM_DEV_NOT_THERE;
		return B_DEV_BAD_DRIVE_NUM;
	}
	/* set the command data pointer */		
	if(ccbio->cam_ch.cam_flags & CAM_CDB_POINTER){
		cmd = ccbio->cam_cdb_io.cam_cdb_ptr;
	}else{
		cmd = ccbio->cam_cdb_io.cam_cdb_bytes;
	}
	/* NOTE: using stack copy of sg_buffer sgb. It can be modified/freed in
					 *_handle_features() functions! Twice reallocation is also not awaited!
					 Note this on refactoring!!! */
	/*TODO returns!*/
	init_sg_buffer(&sgb, ccbio); 
	do{ /* <-- will be repeated if 6-byte RW/MS commands failed */
		uint8 *rcmd;
		uint8 rcmdlen;
		uint32 transfer_len = 0;
		EDirection dir = eDirNone;
		/* handle various features for this device */
		if(!pre_handle_features(udi, ccbio, (scsi_cmd_generic *)cmd, &sgb, &status)){
			release_sem(udi->lock_sem);
			return status;
		}
		/* transform command as required by protocol */
		rcmd		= udi->scsi_command_buf;
		rcmdlen = sizeof(udi->scsi_command_buf);
		if((status = (*udi->transform_m->transform)(udi, cmd, ccbio->cam_cdb_len & 0x1f,
													 &rcmd, &rcmdlen)) != B_OK)
		{
			TRACE_ALWAYS("xpt_scsi_io: transform failed: %08x\n", status);
			ccbio->cam_ch.cam_status = CAM_REQ_INVALID;
			release_sem(udi->lock_sem);
			return B_BAD_VALUE;
		}
		/* set correct direction flag */
		switch(CAM_DIR_MASK & ccbio->cam_ch.cam_flags){
		case CAM_DIR_IN:	dir = eDirIn;	break;
		case CAM_DIR_OUT:	dir = eDirOut;	break;
		default:			dir = eDirNone;	break;
		}
		
		TRACE_DATA_IO_SG(sgb.piov, sgb.count);
		
		/*TODO: return!*/
		sg_buffer_len(&sgb, &transfer_len);			 
		/* transfer command to device. SCSI status will be handled in callback */
		(*udi->protocol_m->transfer)(udi, rcmd, rcmdlen, sgb.piov, sgb.count,
									 transfer_len/*ccbio->cam_dxfer_len*/,
										dir, ccbio, transfer_callback);
		/* perform some post-tranfer features handling
			 and automatic 6-10 bytes command support detection */
	} while(post_handle_features(udi, ccbio, (scsi_cmd_generic *)cmd, &sgb, &status));
	release_sem(udi->lock_sem);
	return status;
}
/**
	\fn:xpt_path_inquiry
	\param ccbp:
	\return:
	
	xpt_path_inquiry - handle XPT_PATH_INQ sim action 
*/
status_t xpt_path_inquiry(CCB_PATHINQ *ccbp)
{
	status_t status			= B_OK;
	
	ccbp->cam_version_num	= SIM_VERSION;
	ccbp->cam_target_sprt	= 0;
	ccbp->cam_hba_eng_cnt	= 0;
	memset (ccbp->cam_vuhba_flags, 0, VUHBA);
	ccbp->cam_sim_priv		= SIM_PRIV;
	ccbp->cam_async_flags	= 0;
	ccbp->cam_initiator_id	= CONTROLLER_SCSI_ID; 
	ccbp->cam_hba_inquiry	= CONTROLLER_SCSI_BUS; /* Narrow SCSI bus */
	ccbp->cam_hba_misc		= PIM_NOINQUIRY;
	ccbp->cam_osd_usage		= 0;
	/* There should be special handling of path_id == 0xff
		 but looks like it's not used by BeOS now */
	/*ccbp->cam_hpath_id = path_id;*/ 
	strncpy(ccbp->cam_sim_vid, sim_vendor_name, SIM_ID);
	strncpy(ccbp->cam_hba_vid, hba_vendor_name, HBA_ID);
	ccbp->cam_ch.cam_status = CAM_REQ_CMP;
	return status;
}
/**
	\fn:xpt_extended_path_inquiry
	\param ccbep: ???
	\return:???
	
	xpt_extended_path_inquiry - handle XPT_EXTENDED_PATH_INQ sim action 
*/
status_t xpt_extended_path_inquiry(CCB_EXTENDED_PATHINQ *ccbep)
{
	status_t status = B_OK;
	xpt_path_inquiry((CCB_PATHINQ *)ccbep); 
	sprintf(ccbep->cam_sim_version, "%d.0", SIM_VERSION);
	sprintf(ccbep->cam_hba_version, "%d.0", HBA_VERSION);
	strncpy(ccbep->cam_controller_family, controller_family, FAM_ID);
	strncpy(ccbep->cam_controller_type, "USB-SCSI", TYPE_ID);
	return status;
}
/**
	\fn:sim_action
	\param ccbh: ????
	\return: ????
	
	This fucntion performs SCSI interface module actions -
	calls corresponding xpt_* - functions.
*/
static long sim_action(CCB_HEADER *ccbh)
{
	status_t status = B_ERROR;
	if(path_id != ccbh->cam_path_id){
		TRACE_ALWAYS("sim_action:path_id mismatch of func:%d:our:%d,requested:%d\n",
					 ccbh->cam_func_code, path_id, ccbh->cam_path_id);
		ccbh->cam_status = CAM_PATH_INVALID;
	} else {
		ccbh->cam_status = CAM_REQ_INPROG;
		switch(ccbh->cam_func_code){
			case XPT_SCSI_IO:
				status = xpt_scsi_io((CCB_SCSIIO *)ccbh);
				break;
			case XPT_PATH_INQ:
				status = xpt_path_inquiry((CCB_PATHINQ *)ccbh);
				break;
			case XPT_EXTENDED_PATH_INQ:
				status = xpt_extended_path_inquiry((CCB_EXTENDED_PATHINQ *)ccbh);
				break;
			default:
				TRACE_ALWAYS("sim_action: unsupported function: %x\n", ccbh->cam_func_code);
				ccbh->cam_status = CAM_REQ_INVALID;
				break;
		}
	}
	return status;
}
#endif

/**
	\fn:std_ops
	\param op: operation to be performed on this module
	\param ...: possible additional arguments
	\return: B_OK on success, error status on failure
	
	This function deals with standard module operations. Currently, the only
	two things that entails are initialization and uninitialization. 
	- get the SCSI Bus Manager Module and USB Manager Module
	- put them when we're finished
*/
static status_t std_ops(int32 op, ...)
{
	//int i;
	status_t status = B_OK;//B_ERROR;
	//CAM_SIM_ENTRY entry;
	switch(op) {
	case B_MODULE_INIT:
		TRACE_ALWAYS("std_ops: B_MODULE_INIT called!\n");
	/*	if(0 == atomic_add(&load_count, 1)){
			thread_info tinfo = {0}; */
			load_module_settings();		 
/*			get_thread_info(find_thread(0), &tinfo);
			if(!b_ignore_sysinit2 || (0 != strcmp(tinfo.name, "sysinit2"))){
				create_log();							
				if(get_module(B_USB_MODULE_NAME, (module_info **)&usb) == B_OK){ 
					if(get_module(B_CAM_FOR_SIM_MODULE_NAME, (module_info **)&cam) == B_OK){ 
						for(i = 0; i < MAX_DEVICES_COUNT; i++)
							usb_devices[i] = NULL; 
	
						if((*usb->register_driver)(MODULE_NAME, supported_devices, SIZEOF(supported_devices), "usb_dsk") == B_OK){
							if((*usb->install_notify)(MODULE_NAME, &notify_hooks) == B_OK){
								entry.sim_init = sim_init;
								entry.sim_action = sim_action;
								path_id =(*cam->xpt_bus_register)(&entry);
								usb_serial_lock = create_sem(1, MODULE_NAME"_devices_table_lock");
								status = B_OK;
								break; 
							} 
						} 
						put_module(B_CAM_FOR_SIM_MODULE_NAME);
					}
					put_module(B_USB_MODULE_NAME);
				}
			} else {
				TRACE_ALWAYS("std_ops INIT call was ignored for thread:%s\n", tinfo.name);
			}
		} else {
			atomic_add(&load_count, -1);
		}*/	
		break;
	case B_MODULE_UNINIT:
		TRACE_ALWAYS("std_ops: B_MODULE_UNINIT called!\n");
	/*	if(1 == atomic_add(&load_count, -1)){
			(*usb->uninstall_notify)(MODULE_NAME);
			status = B_OK;
			if(path_id != -1){
				(*cam->xpt_bus_deregister)(path_id);
				path_id = -1;
			}
			delete_sem(usb_serial_lock);
			put_module(B_USB_MODULE_NAME);
			put_module(B_CAM_FOR_SIM_MODULE_NAME); 
		} else {
			atomic_add(&load_count, 1);
		}*/
		break;
	}
	return status;
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static float 
supports_device(device_node_handle parent, bool *_noConnection)
{
	TRACE_ALWAYS("supports_device\n");
	return 0.f;
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static status_t
register_device(device_node_handle parent)
{
	TRACE_ALWAYS("register_device\n");
	return B_OK;
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static status_t
init_module(device_node_handle node, void *user_cookie, void **_cookie)
{
	TRACE_ALWAYS("inti_driver\n");
	return B_OK;
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static status_t
uninit_module(void *cookie)
{
	TRACE_ALWAYS("uninit_driver\n");
	return B_OK;
}

/**
 * \fn: 
 * \param :
 *  TODO
 */
static void
device_removed(device_node_handle node, void *cookie)
{
	TRACE_ALWAYS("device_removed\n");
}

/**
 * \fn: 
 * \param :
 *  TODO
 */
static void
device_cleanup(device_node_handle node)
{
	TRACE_ALWAYS("device_cleanup\n");
}

/**
 * \fn: 
 * \param :
 *  TODO
 */
static void
get_supported_paths(const char ***_busses, const char ***_devices)
{
	TRACE_ALWAYS("get_supported_path\n");
}

/**
 * \fn: 
 * \param :
 *  TODO
 */
static void
scsi_io( scsi_sim_cookie cookie, scsi_ccb *ccb )
{
	TRACE_ALWAYS("scsi_io\n");
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static uchar
abort( scsi_sim_cookie cookie, scsi_ccb *ccb_to_abort )
{
	TRACE_ALWAYS("scsi_sim\n");
	return 0;
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static uchar
reset_device( scsi_sim_cookie cookie, uchar target_id, uchar target_lun )
{
	TRACE_ALWAYS("supports_device\n");
	return 0;
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static uchar
term_io( scsi_sim_cookie cookie, scsi_ccb *ccb_to_terminate )
{
	TRACE_ALWAYS("term_io\n");
	return 0;
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static uchar
path_inquiry( scsi_sim_cookie cookie, scsi_path_inquiry *inquiry_data )
{
	TRACE_ALWAYS("path_inquiry\n");
	return 0;
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static uchar
scan_bus( scsi_sim_cookie cookie )
{
	TRACE_ALWAYS("scan_bus\n");
	return 0;
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static uchar
reset_bus( scsi_sim_cookie cookie )
{
	TRACE_ALWAYS("reset_bus\n");
	return 0;
}

/**
 * \fn: 
 * \param :
 *  TODO
 */
static void
get_restrictions(scsi_sim_cookie cookie, uchar target_id, bool *is_atapi, bool *no_autosense, uint32 *max_blocks )
{
	TRACE_ALWAYS("get_restrictions\n");
}

/**
 * \fn: 
 * \param :
 * \return:
 *  TODO
 */
static status_t
module_ioctl(scsi_sim_cookie cookie, uint8 targetID, uint32 op, void *buffer, size_t length)
{
	TRACE_ALWAYS("ioctl\n");
	return B_DEV_INVALID_IOCTL;
}


/**
	Declare our module_info so we can be loaded as a kernel module
*/
static scsi_sim_interface usb_scsi_sim = {
	{	//driver_module_info 
		{ // module_info
			"busses/scsi/usb/device_v1", // is device_v1 really required? or v1 is enough?
			0, 
			&std_ops
		},
		
		supports_device,
		register_device,

		init_module,	// init_driver, 
		uninit_module,	// uninit_driver,
		
		device_removed,
		device_cleanup,

		get_supported_paths,
	},
	
	scsi_io,
	abort,
	reset_device,
	term_io,
	
	path_inquiry,
	scan_bus,
	reset_bus,
	
	get_restrictions,
	
	module_ioctl //ioctl	
};

/**
	Export module_info-s list 
*/
_EXPORT module_info *modules[] = {
	(module_info *) &usb_scsi_sim,
	NULL
};

