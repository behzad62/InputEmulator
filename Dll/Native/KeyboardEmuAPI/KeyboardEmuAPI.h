#pragma once
#include "pch.h"

#ifdef KEYBOARDEMUAPI_EXPORTS
#define Public __declspec(dllexport)
#else 
#define Public __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif

enum KBD_KEY_STATE
{
	KBD_KEY_DOWN = 0x00,
	KBD_KEY_UP = 0x01,
	KBD_KEY_E0 = 0x02,
	KBD_KEY_E1 = 0x04,
	KBD_KEY_TERMSRV_SET_LED = 0x08,
	KBD_KEY_TERMSRV_SHADOW = 0x10,
	KBD_KEY_TERMSRV_VKPACKET = 0x20
};

enum KBD_FILTER_FLAG
{
	FLAG_KEY_NONE = 0x0000,
	FLAG_KEY_ALL = 0xFFFF,
	FLAG_KEY_DOWN = KBD_KEY_UP,
	FLAG_KEY_UP = KBD_KEY_UP << 1,
	FLAG_KEY_PRESS = FLAG_KEY_UP | FLAG_KEY_DOWN,
	FLAG_KEY_E0 = KBD_KEY_E0 << 1,
	FLAG_KEY_E1 = KBD_KEY_E1 << 1,
	FLAG_KEY_TERMSRV_SET_LED = KBD_KEY_TERMSRV_SET_LED << 1,
	FLAG_KEY_TERMSRV_SHADOW = KBD_KEY_TERMSRV_SHADOW << 1,
	FLAG_KEY_TERMSRV_VKPACKET = KBD_KEY_TERMSRV_VKPACKET << 1
};
/*++

Function Description:

	Creates a handle to the driver control object.

Return Value:

	Handle to the driver if successful,
	NULL otherwise.

--*/
Public HANDLE CreateDriverHandle(void);


/*++

Function Description:

	Closes the handle previously got from the 'CreateDriverHandle'.


Arguments:

	driverHandle - Handle to the driver control object


--*/
Public void DisposeHandle(HANDLE driverHandle);


/*++

Function Description:

	Queries the number of installed keyboard devices and the current active device Id. 


Arguments:

	driverHandle - Handle to the driver control object

	devices - Pointer to a 'KEYBOARD_QUERY_RESULT' structure that will contain the result.
			  Pointer should not be NULL.

Return Value:

	TRUE if successful,
	FALSE otherwise.

--*/
Public BOOL KeyboardGetDevices(IN HANDLE driverHandle, OUT PKEYBOARD_QUERY_RESULT devices);


/*++

Function Description:

	Sets the active device to the given Id.
	Active device is a device that this API operates on.

Arguments:

	driverHandle - Handle to the driver control object

	deviceId - Target keyboard device id.
			   This value should be in valied range returned by the 'KeyboardGetDevices'.

Return Value:

	TRUE if successful,
	FALSE otherwise.

--*/
Public BOOL KeyboardSetActiveDevice(IN HANDLE driverHandle, IN USHORT deviceId);


/*++

Function Description:

	When called then next key press on any keyboard will set the active device Id to
	the device Id of the detected keyboard. The calling thread will be blocked until
	input detected.

Arguments:

	driverHandle - Handle to the driver control object

	deviceId - Detected keyboard device id.

Return Value:

	TRUE if successful,
	FALSE otherwise.

--*/
Public BOOL KeyboardDetectDeviceId(IN HANDLE driverHandle, OUT PUSHORT deviceId);


/*++

Function Description:

	Sets the key filtering for the active device.

Arguments:

	driverHandle - Handle to the driver control object

	filterRequest - Pointer to a 'KEY_FILTER_REQUEST' structure that contains the filtering data.
					If 'FilterMode' is set to 'FILTER_KEY_FLAGS', then the 'FilterCount' member 
					should contains the actual filter flag value. Ex. one of 'KBD_FILTER_FLAG' values.
					If 'FilterMode' is set to 'FILTER_KEY_FLAG_SCANCODE', then the 'FilterData' member
					should point a location that contains 'FilterCount' of 'KEY_FILTER_DATA's.
			  
Return Value:

	TRUE if successful,
	FALSE otherwise.

--*/
Public BOOL KeyboardSetKeyFiltering(IN HANDLE driverHandle, IN PKEY_FILTER_REQUEST filterRequest);


/*++

Function Description:

	Gets the key filtering info of the active device.

Arguments:

	driverHandle - Handle to the driver control object

	filterBuffer - Pointer to a 'KEY_FILTER_REQUEST' structure that will contains the key filtering data.
					The 'FilterData' member should point a location that contains 'FilterCount' of 'KEY_FILTER_DATA' buffers.
					If the buffer is not large enough, the result will only contain 'FilterCount' amount of filter datas.


Return Value:

	TRUE if successful,
	FALSE otherwise.

--*/
Public BOOL KeyboardGetKeyFiltering(IN HANDLE driverHandle, IN OUT PKEY_FILTER_REQUEST filterBuffer);


/*++

Function Description:

	Adds a key filtering data to the collection of key filters of the active device.
	If the key filtering mode of the active device is not currently set to
					'FILTER_KEY_FLAG_SCANCODE', it will be set to this mode.

Arguments:

	driverHandle - Handle to the driver control object

	filterData - Pointer to a 'KEY_FILTER_DATA' structure that contains the key filtering data to be added.


Return Value:

	TRUE if successfully added or already existed,
	FALSE otherwise.

--*/
Public BOOL KeyboardAddKeyFiltering(IN HANDLE driverHandle, IN PKEY_FILTER_DATA filterData);


/*++

Function Description:

	Removes a key filtering data from the collection of key filters of the active device.

Arguments:

	driverHandle - Handle to the driver control object

	filterData - Pointer to a 'KEY_FILTER_DATA' structure that contains the key filtering data to be removed.
				


Return Value:

	TRUE if successfully removed or not found,
	FALSE otherwise.

--*/
Public BOOL KeyboardRemoveKeyFiltering(IN HANDLE driverHandle, IN PKEY_FILTER_DATA filterData);


/*++

Function Description:

	Sets the key modifying for the active device.

Arguments:

	driverHandle - Handle to the driver control object

	modifyRequest - Pointer to a 'KEY_MODIFY_REQUEST' structure that contains the key modification request.
					The 'ModifyData' member should point a location that contains 'ModifyCount' of 'KEY_Modify_DATA' structs.


Return Value:

	TRUE if successful,
	FALSE otherwise.

--*/
Public BOOL KeyboardSetKeyModifying(IN HANDLE driverHandle, IN PKEY_MODIFY_REQUEST modifyRequest);


/*++

Function Description:

	Gets the key modifying of the active device.

Arguments:

	driverHandle - Handle to the driver control object

	modifyBuffer - Pointer to a 'KEY_MODIFY_REQUEST' structure that will contains the active key modifications.
					The 'ModifyData' member should point a location that contains 'ModifyCount' of 'KEY_Modify_DATA' buffers.
					If the buffer is not large enough, the result will only contain 'ModifyCount' amount of modify datas.


Return Value:

	TRUE if successful,
	FALSE otherwise.

--*/
Public BOOL KeyboardGetKeyModifying(IN HANDLE driverHandle, IN OUT PKEY_MODIFY_REQUEST modifyBuffer);


/*++

Function Description:

	Adds a key modifying data to the collection of key modifications of the active device.

Arguments:

	driverHandle - Handle to the driver control object

	modifyData - Pointer to a 'PKEY_MODIFY_DATA' structure that contains the key modification data to be added.


Return Value:

	TRUE if successfully added or already existed,
	FALSE otherwise.

--*/
Public BOOL KeyboardAddKeyModifying(IN HANDLE driverHandle, IN PKEY_MODIFY_DATA modifyData);


/*++

Function Description:

	Removes a key modifying data from the collection of key modifications of the active device.

Arguments:

	driverHandle - Handle to the driver control object

	modifyData - Pointer to a 'PKEY_MODIFY_DATA' structure that contains the key modification data to be removed.



Return Value:

	TRUE if successfully removed or not found,
	FALSE otherwise.

--*/
Public BOOL KeyboardRemoveKeyModifying(IN HANDLE driverHandle, IN PKEY_MODIFY_DATA modifyData);


/*++

Function Description:

	Insert keys to the output from the context of the active device.

Arguments:

	driverHandle - Handle to the driver control object

	inputKeys - Pointer to 'KEYBOARD_INPUT_DATA' structures that contain the input data.

	inputCount - Number of keyboard input datas that 'inputKeys' points to.


Return Value:

	TRUE if successful,
	FALSE otherwise.

--*/
Public BOOL KeyboardInsertKeys(IN HANDLE driverHandle, IN PKEYBOARD_INPUT_DATA inputKeys, IN ULONG inputCount);


/*++

Function Description:

	Gets the attributes of the active keyboard device.

Arguments:

	driverHandle - Handle to the driver control object

	modifyBuffer - Pointer to a 'KEYBOARD_ATTRIBUTES' structure that will contains the keyboard attributes.


Return Value:

	TRUE if successful,
	FALSE otherwise.

--*/
Public BOOL KeyboardGetAttributes(IN HANDLE driverHandle, OUT PKEYBOARD_ATTRIBUTES attributes);

#ifdef __cplusplus
}
#endif