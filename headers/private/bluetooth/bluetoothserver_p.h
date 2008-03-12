#ifndef _BLUETOOTH_SERVER_PRIVATE_H
#define _BLUETOOTH_SERVER_PRIVATE_H


#define BLUETOOTH_SIGNATURE "application/x-vnd.Be-bluetooth_server"

/* Kit Comunication */

// LocalDevice 
#define BT_MSG_COUNT_LOCAL_DEVICES		'btCd'
#define BT_MSG_ADQUIRE_LOCAL_DEVICE     'btAd'
#define BT_MSG_GET_FRIENDLY_NAME        'btFn'
#define BT_MSG_GET_ADDRESS              'btGa'
#define BT_MSG_HANDLE_SIMPLE_REQUEST    'btsR'
#define BT_MSG_ADD_DEVICE               'btDD'

// Discovery
#define BT_MSG_INQUIRY_STARTED    'IqSt'
#define BT_MSG_INQUIRY_COMPLETED  'IqCM'
#define BT_MSG_INQUIRY_TERMINATED 'IqTR'
#define BT_MSG_INQUIRY_ERROR      'IqER'
#define BT_MSG_INQUIRY_DEVICE     'IqDE'


#endif
