/*
 * Copyright 2001-2005, Haiku, Inc. All Rights Reserved.
 * Distributed under the terms of the MIT License.
 */
#ifndef INPUT_SERVER_TYPES_H
#define INPUT_SERVER_TYPES_H


#define IS_GET_MOUSE_TYPE				'Igmt'
#define IS_SET_MOUSE_TYPE 				'Ismt'
#define IS_GET_MOUSE_ACCELERATION 		'Igma'
#define IS_SET_MOUSE_ACCELERATION 		'Isma'
#define IS_GET_KEY_REPEAT_DELAY 		'Igrd'
#define IS_SET_KEY_REPEAT_DELAY 		'Isrd'
#define IS_GET_KEY_INFO 				'Igki'
#define IS_GET_MODIFIERS 				'Igmd'
#define IS_GET_MODIFIER_KEY			'Igmk'
#define IS_SET_MODIFIER_KEY 			'Ismk'
#define IS_SET_KEYBOARD_LOCKS 			'Iskl'
#define IS_GET_MOUSE_SPEED 				'Igms'
#define IS_SET_MOUSE_SPEED 				'Isms'
#define IS_SET_MOUSE_POSITION 			'Ismp'
#define IS_GET_MOUSE_MAP 				'Igmm'
#define IS_SET_MOUSE_MAP 				'Ismm'
#define IS_GET_KEYBOARD_ID 				'Igid'
#define IS_SET_KEYBOARD_ID 				'Isid'
#define IS_GET_CLICK_SPEED 				'Igcs'
#define IS_SET_CLICK_SPEED 				'Iscs'
#define IS_GET_KEY_REPEAT_RATE 			'Igrr'
#define IS_SET_KEY_REPEAT_RATE 			'Isrr'
#define IS_GET_KEY_MAP					'Igkm'
#define IS_RESTORE_KEY_MAP				'Iskm'
#define IS_FOCUS_IM_AWARE_VIEW 			'Ifim'
#define IS_UNFOCUS_IM_AWARE_VIEW 		'Iuim'

#define IS_FIND_DEVICES					'Ifdv'
#define IS_WATCH_DEVICES				'Iwdv'
#define IS_NOTIFY_DEVICE				'Intf'
#define IS_IS_DEVICE_RUNNING			'Idvr'
#define IS_START_DEVICE					'Istd'
#define IS_STOP_DEVICE					'Ispd'
#define IS_CONTROL_DEVICES				'Icnd'
#define SYSTEM_SHUTTING_DOWN			'SSDn'

#define IS_SAVE_SETTINGS			'Isst'
#define IS_SAVE_KEYMAP				'Iskp'

// app_server communication
#define IS_ACQUIRE_INPUT				'Iaqi'
#define IS_RELEASE_INPUT				'Irli'

// Method Replicant
#define IS_SET_METHOD					'MRsm'
#define IS_METHOD_REGISTER				'MRmr'
#define IS_UPDATE_NAME					'MRun'
#define IS_UPDATE_ICON					'MRui'
#define IS_UPDATE_MENU					'MRum'
#define IS_UPDATE_METHOD				'MRu!'
#define IS_ADD_METHOD					'MRa!'
#define IS_REMOVE_METHOD				'MRr!'

// Change screen resolution
#define IS_SCREEN_BOUNDS_UPDATED		'_FMM'
	// R5 compatible definition

#endif	/* INPUT_SERVER_TYPES_H */
