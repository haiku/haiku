#ifndef _BLUETOOTH_SERVER_PRIVATE_H
#define _BLUETOOTH_SERVER_PRIVATE_H


#define BLUETOOTH_SIGNATURE "application/x-vnd.Haiku-bluetooth_server"
#define BLUETOOTH_APP_SIGNATURE "application/x-vnd.Haiku-BluetoothPrefs"

/* Kit Comunication */
// Watching
#define BT_START_WATCHING_CONNECTIONS               'wtST'
#define BT_STOP_WATCHING_CONNECTIONS                'wtSP'

// LocalDevice
#define BT_MSG_COUNT_LOCAL_DEVICES		'btCd'
#define BT_MSG_ACQUIRE_LOCAL_DEVICE     'btAd'
#define BT_MSG_HANDLE_SIMPLE_REQUEST    'btsR'
#define BT_MSG_ADD_DEVICE               'btDD'
#define BT_MSG_REMOVE_DEVICE            'btrD'
#define BT_MSG_GET_PROPERTY             'btgP'
#define BT_MSG_GET_REMOTE_DEVICES       'btgD'

// Discovery
#define BT_MSG_INQUIRY_STARTED          'IqSt'
#define BT_MSG_INQUIRY_COMPLETED        'IqCM'
#define BT_MSG_INQUIRY_TERMINATED       'IqTR'
#define BT_MSG_INQUIRY_ERROR            'IqER'
#define BT_MSG_INQUIRY_DEVICE           'IqDE'

// Pairing
#define BT_MSG_CONN_FAILED              'CnFL'
#define BT_MSG_CONN_COMPLETED           'CnCM'
#define BT_MSG_DISCONN_COMPLETED        'DcCM'

#define BT_REQ_CREATE_CONN              'rdCN'
#define BT_REQ_CANCEL_CONN              'rdCC'
#define BT_REQ_DISCONNECT               'rdDC'
#define BT_REQ_REMOVE_DEVICE            'rdRD'

#define BT_REQ_CONN_STATE               'rdCS'


#endif
