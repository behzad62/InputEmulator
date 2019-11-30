
#include "pch.h"
#include "KeyboardEmuAPI.h"


HANDLE CreateDriverHandle(void) {
	HANDLE file = CreateFileW(L"\\\\.\\KeyboardEmulator",
		GENERIC_READ | FILE_GENERIC_WRITE,
		FILE_SHARE_READ,
		NULL, // no SECURITY_ATTRIBUTES structure
		OPEN_EXISTING, // No special create flags
		0, // No special attributes
		NULL);
	if (INVALID_HANDLE_VALUE == file) {

		return NULL;
	}
	return file;
}

void DisposeHandle(HANDLE driverHandle) {
	if (driverHandle != INVALID_HANDLE_VALUE)
	{
		CloseHandle(driverHandle);
	}
}

BOOL KeyboardGetDevices(IN HANDLE driverHandle, OUT PKEYBOARD_QUERY_RESULT devices) {
	if (!devices || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_KEYBOARD_GET_DEVICE_ID,
		NULL, 0,
		devices, sizeof(KEYBOARD_QUERY_RESULT),
		&bytesReturned, NULL)) {
		return FALSE;
	}
	if (bytesReturned != sizeof(KEYBOARD_QUERY_RESULT))
		return FALSE;
	return TRUE;
}

BOOL KeyboardSetActiveDevice(IN HANDLE driverHandle, IN USHORT deviceId) {
	if (driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_KEYBOARD_SET_DEVICE_ID,
		&deviceId, sizeof(deviceId),
		NULL, 0,
		&bytesReturned, NULL)) {
		return FALSE;
	}

	return TRUE;
}

BOOL KeyboardSetKeyFiltering(IN HANDLE driverHandle, IN PKEY_FILTER_REQUEST filterRequest)
{
	if (!filterRequest || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;

	if (filterRequest->FilterMode == FILTER_KEY_NONE || filterRequest->FilterMode == FILTER_KEY_ALL)
	{

		if (!DeviceIoControl(
			driverHandle,
			IOCTL_KEYBOARD_SET_FILTER,
			&filterRequest->FilterMode, sizeof(USHORT),
			NULL, 0,
			&bytesReturned, NULL)) {
			return FALSE;
		}
		return TRUE;
	}
	else if (filterRequest->FilterMode == FILTER_KEY_FLAGS)
	{
		if (!DeviceIoControl(
			driverHandle,
			IOCTL_KEYBOARD_SET_FILTER,
			filterRequest, 2 * sizeof(USHORT),
			NULL, 0,
			&bytesReturned, NULL))
		{
			return FALSE;
		}
		return TRUE;
	}
	else if (filterRequest->FilterMode == FILTER_KEY_FLAG_AND_SCANCODE)
	{
		HANDLE processHeap = GetProcessHeap();
		if (!processHeap)
		{
			return FALSE;
		}
		DWORD requiredBytes = 2 * sizeof(USHORT) + filterRequest->FilterCount * sizeof(KEY_FILTER_DATA);
		PUSHORT buffer = (PUSHORT)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
		if (!buffer)
		{
			CloseHandle(processHeap);
			return FALSE;
		}
		buffer[0] = filterRequest->FilterMode;
		buffer[1] = filterRequest->FilterCount;

		PKEY_FILTER_DATA fData = (PKEY_FILTER_DATA)(&buffer[2]);
		for (USHORT i = 0; i < filterRequest->FilterCount; i++)
		{
			fData[i] = filterRequest->FilterData[i];
		}
		if (!DeviceIoControl(
			driverHandle,
			IOCTL_KEYBOARD_SET_FILTER,
			buffer, requiredBytes,
			NULL, 0,
			&bytesReturned, NULL))
		{
			HeapFree(processHeap, 0, buffer);
			CloseHandle(processHeap);
			return FALSE;
		}
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return TRUE;
	}
	return FALSE;
}

BOOL KeyboardGetKeyFiltering(IN HANDLE driverHandle, IN OUT PKEY_FILTER_REQUEST filterBuffer)
{
	if (!filterBuffer || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	DWORD requiredBytes = 2 * sizeof(USHORT) + filterBuffer->FilterCount * sizeof(KEY_FILTER_DATA);
	HANDLE processHeap = GetProcessHeap();
	if (!processHeap)
		return FALSE;
	PUSHORT buffer = (PUSHORT)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
	if (!buffer)
	{
		CloseHandle(processHeap);
		return FALSE;
	}

	if (!DeviceIoControl(
		driverHandle,
		IOCTL_KEYBOARD_GET_FILTER,
		NULL, 0,
		buffer, requiredBytes,
		&bytesReturned, NULL))
	{
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return FALSE;
	}
	if (bytesReturned >= sizeof(USHORT))
	{
		filterBuffer->FilterMode = buffer[0];
		bytesReturned -= sizeof(USHORT);
		if (bytesReturned >= sizeof(USHORT)) {
			filterBuffer->FilterCount = buffer[1];
			bytesReturned -= sizeof(USHORT);
			USHORT filterCount = bytesReturned / sizeof(KEY_FILTER_DATA);
			if (filterCount > 0) {
				PKEY_FILTER_DATA filterData = (PKEY_FILTER_DATA)(&buffer[2]);
				for (USHORT i = 0; i < filterCount; i++)
				{
					filterBuffer->FilterData[i] = filterData[i];
				}
			}
		}
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return TRUE;
	}
	else {
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return FALSE;
	}
}

BOOL KeyboardAddKeyFiltering(IN HANDLE driverHandle, IN PKEY_FILTER_DATA filterData)
{
	if (!filterData || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	KEY_FILTER_REQUEST filterBuffer;
	filterBuffer.FilterCount = 0;
	if (!KeyboardGetKeyFiltering(driverHandle, &filterBuffer))
		return FALSE;
	//filterBuffer.FilterCount now holds number of previously set filter datas
	USHORT filterCount = filterBuffer.FilterCount;//lets save it in the case another request changed it
	if (filterCount == 0 || filterBuffer.FilterMode != FILTER_KEY_FLAG_AND_SCANCODE)
	{
		filterBuffer.FilterMode = FILTER_KEY_FLAG_AND_SCANCODE;
		filterBuffer.FilterCount = 1;
		filterBuffer.FilterData = filterData;
		return KeyboardSetKeyFiltering(driverHandle, &filterBuffer);
	}
	else
	{
		//add room for one more filter data
		DWORD requiredBytes = (filterCount + 1) * sizeof(KEY_FILTER_DATA);
		HANDLE processHeap = GetProcessHeap();
		if (!processHeap)
			return FALSE;
		PKEY_FILTER_DATA buffer = (PKEY_FILTER_DATA)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
		if (!buffer)
		{
			CloseHandle(processHeap);
			return FALSE;
		}
		filterBuffer.FilterData = buffer;
		//first get current filters
		if (!KeyboardGetKeyFiltering(driverHandle, &filterBuffer))
			return FALSE;
		if (filterBuffer.FilterCount < filterCount)
			filterCount = filterBuffer.FilterCount;//reduce count if it changed meanwhile
		for (USHORT i = 0; i < filterCount; i++)
		{
			if (filterBuffer.FilterData[i].FlagPredicates == filterData->FlagPredicates &&
				filterBuffer.FilterData[i].ScanCode == filterData->ScanCode)
			{
				//already exists
				HeapFree(processHeap, 0, buffer);
				CloseHandle(processHeap);
				return TRUE;
			}
		}
		filterBuffer.FilterMode = FILTER_KEY_FLAG_AND_SCANCODE;
		//add the new one to the end
		filterBuffer.FilterData[filterCount] = *filterData;
		filterBuffer.FilterCount = filterCount + 1;
		BOOL result = KeyboardSetKeyFiltering(driverHandle, &filterBuffer);
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return result;
	}
}

BOOL KeyboardRemoveKeyFiltering(IN HANDLE driverHandle, IN PKEY_FILTER_DATA filterData)
{
	if (!filterData || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	KEY_FILTER_REQUEST filterBuffer;
	filterBuffer.FilterCount = 0;
	if (!KeyboardGetKeyFiltering(driverHandle, &filterBuffer))
		return FALSE;
	//filterBuffer.FilterCount now holds number of previously set filter datas
	USHORT filterCount = filterBuffer.FilterCount;//lets save it in the case another request changed it
	if (filterCount == 0 || filterBuffer.FilterMode != FILTER_KEY_FLAG_AND_SCANCODE)
	{
		return TRUE;
	}
	else
	{
		DWORD requiredBytes = filterCount * sizeof(KEY_FILTER_DATA);
		HANDLE processHeap = GetProcessHeap();
		if (!processHeap)
			return FALSE;
		PKEY_FILTER_DATA buffer = (PKEY_FILTER_DATA)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
		if (!buffer)
		{
			CloseHandle(processHeap);
			return FALSE;
		}
		filterBuffer.FilterData = buffer;
		//first get current filters
		if (!KeyboardGetKeyFiltering(driverHandle, &filterBuffer))
			return FALSE;

		if (filterBuffer.FilterCount < filterCount)
			filterCount = filterBuffer.FilterCount;//reduce count if it changed meanwhile
		for (USHORT i = 0; i < filterCount; i++)
		{
			if (filterBuffer.FilterData[i].FlagPredicates == filterData->FlagPredicates &&
				filterBuffer.FilterData[i].ScanCode == filterData->ScanCode)
			{
				for (USHORT j = i + 1; j < filterCount; j++)
				{
					filterBuffer.FilterData[j - 1] = filterBuffer.FilterData[j];
				}
				filterCount--;
			}
		}
		filterBuffer.FilterMode = FILTER_KEY_FLAG_AND_SCANCODE;
		filterBuffer.FilterCount = filterCount;
		BOOL result = KeyboardSetKeyFiltering(driverHandle, &filterBuffer);
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return result;
	}
}

BOOL KeyboardSetKeyModifying(IN HANDLE driverHandle, IN PKEY_MODIFY_REQUEST modifyRequest)
{
	if (!modifyRequest || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	if (modifyRequest->ModifyCount == 0)
	{
		if (!DeviceIoControl(
			driverHandle,
			IOCTL_KEYBOARD_SET_MODIFY,
			&modifyRequest->ModifyCount, sizeof(USHORT),
			NULL, 0,
			&bytesReturned, NULL))
		{
			return FALSE;
		}
		return TRUE;
	}
	DWORD requiredBytes = sizeof(USHORT) + modifyRequest->ModifyCount * sizeof(KEY_MODIFY_DATA);
	HANDLE processHeap = GetProcessHeap();
	if (!processHeap)
		return FALSE;
	PUSHORT p = (PUSHORT)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
	if (!p)
		return FALSE;
	p[0] = modifyRequest->ModifyCount;
	PKEY_MODIFY_DATA modifyData = (PKEY_MODIFY_DATA)(&p[1]);
	for (USHORT i = 0; i < modifyRequest->ModifyCount; i++)
	{
		modifyData[i] = modifyRequest->ModifyData[i];
	}
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_KEYBOARD_SET_MODIFY,
		p, requiredBytes,
		NULL, 0,
		&bytesReturned, NULL))
	{
		HeapFree(processHeap, HEAP_ZERO_MEMORY, p);
		CloseHandle(processHeap);
		return FALSE;
	}
	HeapFree(processHeap, HEAP_ZERO_MEMORY, p);
	CloseHandle(processHeap);
	return TRUE;
}

BOOL KeyboardGetKeyModifying(IN HANDLE driverHandle, IN OUT PKEY_MODIFY_REQUEST modifyBuffer)
{
	if (!modifyBuffer || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	DWORD requiredBytes = sizeof(USHORT) + modifyBuffer->ModifyCount * sizeof(KEY_MODIFY_DATA);
	HANDLE processHeap = GetProcessHeap();
	if (!processHeap)
		return FALSE;
	PUSHORT buffer = (PUSHORT)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
	if (!buffer)
	{
		CloseHandle(processHeap);
		return FALSE;
	}
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_KEYBOARD_GET_MODIFY,
		NULL, 0,
		buffer, requiredBytes,
		&bytesReturned, NULL))
	{
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return FALSE;
	}
	if (bytesReturned >= sizeof(USHORT)) {
		modifyBuffer->ModifyCount = buffer[0];
		bytesReturned -= sizeof(USHORT);
		USHORT modifyCount = bytesReturned / sizeof(KEY_MODIFY_DATA);
		if (modifyCount > 0) {
			PKEY_MODIFY_DATA modifyData = (PKEY_MODIFY_DATA)(&buffer[1]);
			for (USHORT i = 0; i < modifyCount; i++)
			{
				modifyBuffer->ModifyData[i] = modifyData[i];
			}
		}
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return TRUE;
	}
	else {
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return FALSE;
	}
}

BOOL KeyboardAddKeyModifying(IN HANDLE driverHandle, IN PKEY_MODIFY_DATA modifyData)
{
	if (!modifyData || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	KEY_MODIFY_REQUEST modifyBuffer;
	modifyBuffer.ModifyCount = 0;
	if (!KeyboardGetKeyModifying(driverHandle, &modifyBuffer))
		return FALSE;
	//modifyBuffer.ModifyCount now holds number of previously set modifications
	USHORT modifyCount = modifyBuffer.ModifyCount;//lets save it in the case another request changed it
	if (modifyCount == 0)
	{
		modifyBuffer.ModifyCount = 1;
		modifyBuffer.ModifyData = modifyData;
		return KeyboardSetKeyModifying(driverHandle, &modifyBuffer);
	}
	else
	{
		//add room for one more filter data
		DWORD requiredBytes = (modifyCount + 1) * sizeof(KEY_MODIFY_DATA);
		HANDLE processHeap = GetProcessHeap();
		if (!processHeap)
			return FALSE;
		PKEY_MODIFY_DATA buffer = (PKEY_MODIFY_DATA)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
		if (!buffer)
		{
			CloseHandle(processHeap);
			return FALSE;
		}
		modifyBuffer.ModifyData = buffer;
		//first get current filters
		if (!KeyboardGetKeyModifying(driverHandle, &modifyBuffer))
			return FALSE;
		if (modifyBuffer.ModifyCount < modifyCount)
			modifyCount = modifyBuffer.ModifyCount;//reduce count if it changed meanwhile
		for (USHORT i = 0; i < modifyCount; i++)
		{
			if (modifyBuffer.ModifyData[i].FlagPredicates == modifyData->FlagPredicates &&
				modifyBuffer.ModifyData[i].FromScanCode == modifyData->FromScanCode &&
				modifyBuffer.ModifyData[i].ToScanCode == modifyData->ToScanCode)
			{
				//already exists
				HeapFree(processHeap, 0, buffer);
				CloseHandle(processHeap);
				return TRUE;
			}
		}
		//add the new one to the end
		modifyBuffer.ModifyData[modifyCount] = *modifyData;
		modifyBuffer.ModifyCount = modifyCount + 1;
		BOOL result = KeyboardSetKeyModifying(driverHandle, &modifyBuffer);
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return result;
	}
}

BOOL KeyboardRemoveKeyModifying(IN HANDLE driverHandle, IN PKEY_MODIFY_DATA modifyData)
{
	if (!modifyData || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	KEY_MODIFY_REQUEST modifyBuffer;
	modifyBuffer.ModifyCount = 0;
	if (!KeyboardGetKeyModifying(driverHandle, &modifyBuffer))
		return FALSE;
	//modifyBuffer.ModifyCount now holds number of previously set modifications
	USHORT modifyCount = modifyBuffer.ModifyCount;//lets save it in the case another request changed it
	if (modifyCount == 0)
	{
		return TRUE;
	}
	else
	{
		DWORD requiredBytes = modifyCount * sizeof(KEY_MODIFY_DATA);
		HANDLE processHeap = GetProcessHeap();
		if (!processHeap)
			return FALSE;
		PKEY_MODIFY_DATA buffer = (PKEY_MODIFY_DATA)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
		if (!buffer)
		{
			CloseHandle(processHeap);
			return FALSE;
		}
		modifyBuffer.ModifyData = buffer;
		//first get current modifications
		if (!KeyboardGetKeyModifying(driverHandle, &modifyBuffer))
			return FALSE;

		if (modifyBuffer.ModifyCount < modifyCount)
			modifyCount = modifyBuffer.ModifyCount;//reduce count if it changed meanwhile
		for (USHORT i = 0; i < modifyCount; i++)
		{
			if (modifyBuffer.ModifyData[i].FlagPredicates == modifyData->FlagPredicates &&
				modifyBuffer.ModifyData[i].FromScanCode == modifyData->FromScanCode &&
				modifyBuffer.ModifyData[i].ToScanCode == modifyData->ToScanCode)
			{
				for (USHORT j = i + 1; j < modifyCount; j++)
				{
					modifyBuffer.ModifyData[j - 1] = modifyBuffer.ModifyData[j];
				}
				modifyCount--;
			}
		}
		modifyBuffer.ModifyCount = modifyCount;
		BOOL result = KeyboardSetKeyModifying(driverHandle, &modifyBuffer);
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return result;
	}
}

BOOL KeyboardInsertKeys(IN HANDLE driverHandle, IN PKEYBOARD_INPUT_DATA inputKeys, IN ULONG inputCount) {
	if (!inputKeys || driverHandle == INVALID_HANDLE_VALUE || inputCount == 0)
		return FALSE;
	DWORD bytesReturned = 0;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_KEYBOARD_INSERT_KEY,
		inputKeys, inputCount * sizeof(KEYBOARD_INPUT_DATA),
		NULL, 0,
		&bytesReturned, NULL)) {
		return FALSE;
	}

	return TRUE;
}

BOOL KeyboardDetectDeviceId(IN HANDLE driverHandle, OUT PUSHORT deviceId) {
	if (driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_KEYBOARD_DETECT_DEVICE_ID,
		NULL, 0,
		deviceId, sizeof(USHORT),
		&bytesReturned, NULL)) {
		return FALSE;
	}
	if (bytesReturned != sizeof(USHORT))
		return FALSE;
	return TRUE;
}

BOOL KeyboardGetAttributes(IN HANDLE driverHandle, OUT PKEYBOARD_ATTRIBUTES attributes) {
	if (!attributes || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_KEYBOARD_GET_ATTRIBUTES,
		NULL, 0,
		attributes, sizeof(KEYBOARD_ATTRIBUTES),
		&bytesReturned, NULL)) {
		return FALSE;
	}
	if (bytesReturned != sizeof(KEYBOARD_ATTRIBUTES))
		return FALSE;
	return TRUE;
}

