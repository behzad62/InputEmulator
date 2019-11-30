
#include "pch.h"
#include "MouseEmuAPI.h"


HANDLE CreateDriverHandle(void) {
	HANDLE file = CreateFileW(L"\\\\.\\MouseEmulator",
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

BOOL MouseGetDevices(IN HANDLE driverHandle, OUT PMOUSE_QUERY_RESULT devices) {
	if (!devices || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_MOUSE_GET_DEVICE_ID,
		NULL, 0,
		devices, sizeof(MOUSE_QUERY_RESULT),
		&bytesReturned, NULL)) {
		return FALSE;
	}
	if (bytesReturned != sizeof(MOUSE_QUERY_RESULT))
		return FALSE;
	return TRUE;
}

BOOL MouseSetActiveDevice(IN HANDLE driverHandle, IN USHORT deviceId) {
	if (driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_MOUSE_SET_DEVICE_ID,
		&deviceId, sizeof(deviceId),
		NULL, 0,
		&bytesReturned, NULL)) {
		return FALSE;
	}

	return TRUE;
}

BOOL MouseSetFilterMode(IN HANDLE driverHandle, IN MOUSE_FILTER_MODE filterMode)
{
	if (driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	USHORT filterCode = (USHORT)filterMode;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_MOUSE_SET_FILTER,
		&filterCode, sizeof(USHORT),
		NULL, 0,
		&bytesReturned, NULL)) {
		return FALSE;
	}
	return TRUE;
}

BOOL MouseGetFilterMode(IN HANDLE driverHandle, IN OUT PMOUSE_FILTER_MODE filterMode)
{
	if (driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;

	DWORD bytesReturned = 0;

	if (!DeviceIoControl(
		driverHandle,
		IOCTL_MOUSE_GET_FILTER,
		NULL, 0,
		&filterMode, sizeof(USHORT),
		&bytesReturned, NULL)) {
		return FALSE;
	}

	return TRUE;
}

BOOL MouseSetModification(IN HANDLE driverHandle, IN PMOUSE_MODIFY_REQUEST modifyRequest)
{
	if (!modifyRequest || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	if (modifyRequest->ModifyCount == 0)
	{
		if (!DeviceIoControl(
			driverHandle,
			IOCTL_MOUSE_SET_MODIFY,
			&modifyRequest->ModifyCount, sizeof(USHORT),
			NULL, 0,
			&bytesReturned, NULL))
		{
			return FALSE;
		}
		return TRUE;
	}
	DWORD requiredBytes = sizeof(USHORT) + modifyRequest->ModifyCount * sizeof(MOUSE_MODIFY_DATA);
	HANDLE processHeap = GetProcessHeap();
	if (!processHeap)
		return FALSE;
	PUSHORT p = (PUSHORT)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
	if (!p)
		return FALSE;
	p[0] = modifyRequest->ModifyCount;
	PMOUSE_MODIFY_DATA modifyData = (PMOUSE_MODIFY_DATA)(&p[1]);
	for (USHORT i = 0; i < modifyRequest->ModifyCount; i++)
	{
		modifyData[i] = modifyRequest->ModifyData[i];
	}
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_MOUSE_SET_MODIFY,
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

BOOL MouseGetModifications(IN HANDLE driverHandle, IN OUT PMOUSE_MODIFY_REQUEST modifyBuffer)
{
	if (!modifyBuffer || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	DWORD requiredBytes = sizeof(USHORT) + modifyBuffer->ModifyCount * sizeof(MOUSE_MODIFY_DATA);
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
		IOCTL_MOUSE_GET_MODIFY,
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
		USHORT modifyCount = bytesReturned / sizeof(MOUSE_MODIFY_DATA);
		if (modifyCount > 0) {
			PMOUSE_MODIFY_DATA modifyData = (PMOUSE_MODIFY_DATA)(&buffer[1]);
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

BOOL MouseAddButtonModification(IN HANDLE driverHandle, IN PMOUSE_MODIFY_DATA modifyData)
{
	if (!modifyData || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	MOUSE_MODIFY_REQUEST modifyBuffer;
	modifyBuffer.ModifyCount = 0;
	if (!MouseGetModifications(driverHandle, &modifyBuffer))
		return FALSE;
	//modifyBuffer.ModifyCount now holds number of previously set modifications
	USHORT modifyCount = modifyBuffer.ModifyCount;//lets save it in the case another request changed it
	if (modifyCount == 0)
	{
		modifyBuffer.ModifyCount = 1;
		modifyBuffer.ModifyData = modifyData;
		return MouseSetModification(driverHandle, &modifyBuffer);
	}
	else
	{
		//add room for one more filter data
		DWORD requiredBytes = (modifyCount + 1) * sizeof(MOUSE_MODIFY_DATA);
		HANDLE processHeap = GetProcessHeap();
		if (!processHeap)
			return FALSE;
		PMOUSE_MODIFY_DATA buffer = (PMOUSE_MODIFY_DATA)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
		if (!buffer)
		{
			CloseHandle(processHeap);
			return FALSE;
		}
		modifyBuffer.ModifyData = buffer;
		//first get current filters
		if (!MouseGetModifications(driverHandle, &modifyBuffer))
			return FALSE;
		if (modifyBuffer.ModifyCount < modifyCount)
			modifyCount = modifyBuffer.ModifyCount;//reduce count if it changed meanwhile
		for (USHORT i = 0; i < modifyCount; i++)
		{
			if (modifyBuffer.ModifyData[i].FromState == modifyData->FromState &&
				modifyBuffer.ModifyData[i].ToState == modifyData->ToState)
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
		BOOL result = MouseSetModification(driverHandle, &modifyBuffer);
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return result;
	}
}

BOOL MouseRemoveButtonModification(IN HANDLE driverHandle, IN PMOUSE_MODIFY_DATA modifyData)
{
	if (!modifyData || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	MOUSE_MODIFY_REQUEST modifyBuffer;
	modifyBuffer.ModifyCount = 0;
	if (!MouseGetModifications(driverHandle, &modifyBuffer))
		return FALSE;
	//modifyBuffer.ModifyCount now holds number of previously set modifications
	USHORT modifyCount = modifyBuffer.ModifyCount;//lets save it in the case another request changed it
	if (modifyCount == 0)
	{
		return TRUE;
	}
	else
	{
		DWORD requiredBytes = modifyCount * sizeof(MOUSE_MODIFY_DATA);
		HANDLE processHeap = GetProcessHeap();
		if (!processHeap)
			return FALSE;
		PMOUSE_MODIFY_DATA buffer = (PMOUSE_MODIFY_DATA)HeapAlloc(processHeap, HEAP_ZERO_MEMORY, requiredBytes);
		if (!buffer)
		{
			CloseHandle(processHeap);
			return FALSE;
		}
		modifyBuffer.ModifyData = buffer;
		//first get current modifications
		if (!MouseGetModifications(driverHandle, &modifyBuffer))
			return FALSE;

		if (modifyBuffer.ModifyCount < modifyCount)
			modifyCount = modifyBuffer.ModifyCount;//reduce count if it changed meanwhile
		for (USHORT i = 0; i < modifyCount; i++)
		{
			if (modifyBuffer.ModifyData[i].FromState == modifyData->FromState &&
				modifyBuffer.ModifyData[i].ToState == modifyData->ToState)
			{
				for (USHORT j = i + 1; j < modifyCount; j++)
				{
					modifyBuffer.ModifyData[j - 1] = modifyBuffer.ModifyData[j];
				}
				modifyCount--;
			}
		}
		modifyBuffer.ModifyCount = modifyCount;
		BOOL result = MouseSetModification(driverHandle, &modifyBuffer);
		HeapFree(processHeap, 0, buffer);
		CloseHandle(processHeap);
		return result;
	}
}

BOOL MouseInsertInputs(IN HANDLE driverHandle, IN PMOUSE_INPUT_DATA inputDatas, IN ULONG inputCount) {
	if (!inputDatas || driverHandle == INVALID_HANDLE_VALUE || inputCount == 0)
		return FALSE;
	DWORD bytesReturned = 0;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_MOUSE_INSERT_KEY,
		inputDatas, inputCount * sizeof(MOUSE_INPUT_DATA),
		NULL, 0,
		&bytesReturned, NULL)) {
		return FALSE;
	}

	return TRUE;
}

BOOL MouseDetectDeviceId(IN HANDLE driverHandle, OUT PUSHORT deviceId) {
	if (driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_MOUSE_DETECT_DEVICE_ID,
		NULL, 0,
		deviceId, sizeof(USHORT),
		&bytesReturned, NULL)) {
		return FALSE;
	}
	if (bytesReturned != sizeof(USHORT))
		return FALSE;
	return TRUE;
}

BOOL MouseGetAttributes(IN HANDLE driverHandle, OUT PMOUSE_ATTRIBUTES attributes) {
	if (!attributes || driverHandle == INVALID_HANDLE_VALUE)
		return FALSE;
	DWORD bytesReturned = 0;
	if (!DeviceIoControl(
		driverHandle,
		IOCTL_MOUSE_GET_ATTRIBUTES,
		NULL, 0,
		attributes, sizeof(MOUSE_ATTRIBUTES),
		&bytesReturned, NULL)) {
		return FALSE;
	}
	if (bytesReturned != sizeof(MOUSE_ATTRIBUTES))
		return FALSE;
	return TRUE;
}