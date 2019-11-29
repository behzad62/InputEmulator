#pragma once
#include "pch.h"

#ifdef MOUSEEMUAPI_EXPORTS
#define Public __declspec(dllexport)
#else 
#define Public __declspec(dllimport)
#endif

#ifdef __cplusplus
extern "C" {
#endif


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

		Queries the number of installed mouse devices and the current active device Id.


	Arguments:

		driverHandle - Handle to the driver control object

		devices - Pointer to a 'MOUSE_QUERY_RESULT' structure that will contain the result.
				  Pointer should not be NULL.

	Return Value:

		TRUE if successful,
		FALSE otherwise.

	--*/
	Public BOOL MouseGetDevices(IN HANDLE driverHandle, OUT PMOUSE_QUERY_RESULT devices);


	/*++

	Function Description:

		Sets the active device to the given Id.
		Active device is a device that this API operates on.

	Arguments:

		driverHandle - Handle to the driver control object

		deviceId - Target mouse device id.
				   This value should be in valied range returned by the 'MouseGetDevices'.

	Return Value:

		TRUE if successful,
		FALSE otherwise.

	--*/
	Public BOOL MouseSetActiveDevice(IN HANDLE driverHandle, IN USHORT deviceId);


	/*++

	Function Description:

		When called then next key press on any mouse will set the active device Id to
		the device Id of the detected mouse. The calling thread will be blocked until
		input detected.

	Arguments:

		driverHandle - Handle to the driver control object

		deviceId - Detected mouse device id.

	Return Value:

		TRUE if successful,
		FALSE otherwise.

	--*/
	Public BOOL MouseDetectDeviceId(IN HANDLE driverHandle, OUT PUSHORT deviceId);


	/*++

	Function Description:

		Sets the key filtering for the active device.

	Arguments:

		driverHandle - Handle to the driver control object

		filterRequest - 'MOUSE_FILTER_MODE' defining the filter mode

	Return Value:

		TRUE if successful,
		FALSE otherwise.

	--*/
	Public BOOL MouseSetFilterMode(IN HANDLE driverHandle, IN MOUSE_FILTER_MODE filterMode);


	/*++

	Function Description:

		Gets the filtering mode of the active device.

	Arguments:

		driverHandle - Handle to the driver control object

		filterMode - Pointer to a 'PMOUSE_FILTER_MODE' enum that will contains the filter mode.


	Return Value:

		TRUE if successful,
		FALSE otherwise.

	--*/
	Public BOOL MouseGetFilterMode(IN HANDLE driverHandle, IN OUT PMOUSE_FILTER_MODE filterMode);


	/*++

	Function Description:

		Sets the button modifications for the active device.

	Arguments:

		driverHandle - Handle to the driver control object

		modifyRequest - Pointer to a 'MOUSE_MODIFY_REQUEST' structure that contains the button modification request.
						The 'ModifyData' member should point a location that contains 'ModifyCount' of 'MOUSE_MODIFY_DATA' structs.


	Return Value:

		TRUE if successful,
		FALSE otherwise.

	--*/
	Public BOOL MouseSetModification(IN HANDLE driverHandle, IN PMOUSE_MODIFY_REQUEST modifyRequest);


	/*++

	Function Description:

		Gets the button modifications of the active device.

	Arguments:

		driverHandle - Handle to the driver control object

		modifyBuffer - Pointer to a 'MOUSE_MODIFY_REQUEST' structure that will contains the button modifications.
						The 'ModifyData' member should point a location that contains 'ModifyCount' of 'BUTTON_Modify_DATA' buffers.
						If the buffer is not large enough, the result will only contain 'ModifyCount' amount of modify datas.


	Return Value:

		TRUE if successful,
		FALSE otherwise.

	--*/
	Public BOOL MouseGetModifications(IN HANDLE driverHandle, IN OUT PMOUSE_MODIFY_REQUEST modifyBuffer);


	/*++

	Function Description:

		Adds a button modification data to the collection of key modifications of the active device.

	Arguments:

		driverHandle - Handle to the driver control object

		modifyData - Pointer to a 'MOUSE_MODIFY_DATA' structure that contains the key modification data to be added.


	Return Value:

		TRUE if successfully added or already existed,
		FALSE otherwise.

	--*/
	Public BOOL MouseAddButtonModification(IN HANDLE driverHandle, IN PMOUSE_MODIFY_DATA modifyData);


	/*++

	Function Description:

		Removes a button modification data from the collection of key modifications of the active device.

	Arguments:

		driverHandle - Handle to the driver control object

		modifyData - Pointer to a 'MOUSE_MODIFY_DATA' structure that contains the key modification data to be removed.



	Return Value:

		TRUE if successfully removed or not found,
		FALSE otherwise.

	--*/
	Public BOOL MouseRemoveButtonModification(IN HANDLE driverHandle, IN PMOUSE_MODIFY_DATA modifyData);


	/*++

	Function Description:

		Insert inputs to the output from the context of the active device.

	Arguments:

		driverHandle - Handle to the driver control object

		inputKeys - Pointer to 'MOUSE_INPUT_DATA' structures that contain the input data.

		inputCount - Number of mouse input datas that 'inputDatas' points to.


	Return Value:

		TRUE if successful,
		FALSE otherwise.

	--*/
	Public BOOL MouseInsertInputs(IN HANDLE driverHandle, IN PMOUSE_INPUT_DATA inputDatas, IN ULONG inputCount);


	/*++

	Function Description:

		Gets the attributes of the active mouse device.

	Arguments:

		driverHandle - Handle to the driver control object

		modifyBuffer - Pointer to a 'MOUSE_ATTRIBUTES' structure that will contains the mouse attributes.


	Return Value:

		TRUE if successful,
		FALSE otherwise.

	--*/
	Public BOOL MouseGetAttributes(IN HANDLE driverHandle, OUT PMOUSE_ATTRIBUTES attributes);

#ifdef __cplusplus
}
#endif