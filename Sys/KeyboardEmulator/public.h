#ifndef _PUBLIC_H
#define _PUBLIC_H

#include "devioctl.h"

#define IOCTL_INDEX0             0x800
#define IOCTL_INDEX1             0x801
#define IOCTL_INDEX2             0x802
#define IOCTL_INDEX3             0x803
#define IOCTL_INDEX4             0x804
#define IOCTL_INDEX5             0x805
#define IOCTL_INDEX6             0x806
#define IOCTL_INDEX7             0x807
#define IOCTL_INDEX8             0x808

#define IOCTL_KEYBOARD_GET_ATTRIBUTES \
    CTL_CODE( FILE_DEVICE_KEYBOARD, IOCTL_INDEX0, METHOD_BUFFERED, FILE_READ_DATA)

#define IOCTL_KEYBOARD_INSERT_KEY \
	CTL_CODE( FILE_DEVICE_KEYBOARD, IOCTL_INDEX1, METHOD_IN_DIRECT, FILE_WRITE_DATA)

#define IOCTL_KEYBOARD_SET_DEVICE_ID \
	CTL_CODE( FILE_DEVICE_KEYBOARD, IOCTL_INDEX2, METHOD_BUFFERED, FILE_WRITE_DATA)

#define IOCTL_KEYBOARD_GET_DEVICE_ID \
	CTL_CODE( FILE_DEVICE_KEYBOARD, IOCTL_INDEX3, METHOD_BUFFERED, FILE_READ_DATA)

#define IOCTL_KEYBOARD_DETECT_DEVICE_ID \
	CTL_CODE( FILE_DEVICE_KEYBOARD, IOCTL_INDEX4, METHOD_BUFFERED, FILE_READ_DATA)

#define IOCTL_KEYBOARD_SET_FILTER \
	CTL_CODE( FILE_DEVICE_KEYBOARD, IOCTL_INDEX5, METHOD_IN_DIRECT, FILE_WRITE_DATA)

#define IOCTL_KEYBOARD_GET_FILTER \
	CTL_CODE( FILE_DEVICE_KEYBOARD, IOCTL_INDEX6, METHOD_OUT_DIRECT, FILE_READ_DATA)

#define IOCTL_KEYBOARD_SET_MODIFY \
	CTL_CODE( FILE_DEVICE_KEYBOARD, IOCTL_INDEX7, METHOD_IN_DIRECT, FILE_WRITE_DATA)

#define IOCTL_KEYBOARD_GET_MODIFY \
	CTL_CODE( FILE_DEVICE_KEYBOARD, IOCTL_INDEX8, METHOD_OUT_DIRECT, FILE_READ_DATA)

typedef struct _KEYBOARD_QUERY_RESULT {
	USHORT ActiveDeviceId; 
	USHORT NumberOfDevices;
} KEYBOARD_QUERY_RESULT, * PKEYBOARD_QUERY_RESULT;

typedef struct _KEY_FILTER_DATA {
	//The predicate flag that will be used to filter inputs
	USHORT FlagPredicates;
	//Scan code of keys which will be filtered
	USHORT ScanCode;
} KEY_FILTER_DATA, * PKEY_FILTER_DATA;

typedef enum _FILTER_MODE {
	//Filter nothing
	FILTER_KEY_NONE = 0x0000,
	//Filter all keys matches given predicate flag
	FILTER_KEY_FLAGS = 0x0001,
	//Filter all keys matches given predicate flag and scan code pairs
	FILTER_KEY_FLAG_AND_SCANCODE = 0x0002,
	//Filter all keys
	FILTER_KEY_ALL = 0xFFFF,
} FILTER_MODE, * PFILTER_MODE;

typedef struct _KEY_MODIFY_DATA {
	//If input flag matches one of this flag bits
	USHORT FlagPredicates;
	//Change scan code from
	USHORT FromScanCode;
	//To this scan code
	USHORT ToScanCode;
} KEY_MODIFY_DATA, * PKEY_MODIFY_DATA;

typedef struct _KEY_MODIFY_REQUEST {
	//
	//Number of modify data entries
	//
	USHORT ModifyCount;
	//
	//Modify data entries
	//
	PKEY_MODIFY_DATA ModifyData;

} KEY_MODIFY_REQUEST, * PKEY_MODIFY_REQUEST;

typedef struct _KEY_FILTER_REQUEST {
	//
	//FILTER_MODE
	//
	USHORT FilterMode;
	//
	//Either No. of input KEY_FILTER_DATA or a key Flag; depends on the filter mode
	//
	USHORT FilterCount;
	//
	//Filter data entries
	//
	PKEY_FILTER_DATA FilterData;

} KEY_FILTER_REQUEST, * PKEY_FILTER_REQUEST;
#endif
