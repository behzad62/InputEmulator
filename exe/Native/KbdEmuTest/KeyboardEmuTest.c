

#include "KeyboardEmuAPI.h"
#include <stdio.h>


int
_cdecl
main(
	_In_ int argc,
	_In_ char* argv[]
)
{
	USHORT								currentDeviceIndex = 0;
	HANDLE                              file = NULL;


	UNREFERENCED_PARAMETER(argc);
	UNREFERENCED_PARAMETER(argv);


	if (argc == 2) {
		sscanf_s(argv[1], "%hu", &currentDeviceIndex);

	}

	file = CreateDriverHandle();
	if (file == INVALID_HANDLE_VALUE)
		return 0;

	KEYBOARD_QUERY_RESULT devices;

	printf("Setting active device to %u\n", currentDeviceIndex);
	if (!KeyboardSetActiveDevice(file, currentDeviceIndex))
	{
		DisposeHandle(file);
		return 0;
	}

	printf("Getting keyboard devices\n");
	if (KeyboardGetDevices(file, &devices))
	{
		printf("No. of devices: %u, Active device: %u\n", devices.NumberOfDevices, devices.ActiveDeviceId);
	}
	else
	{
		DisposeHandle(file);
		return 0;
	}

	printf("Detecting device id of keyboard. Press a key\n");
	if (KeyboardDetectDeviceId(file, &currentDeviceIndex))
	{
		printf("Device id is: %u\n", currentDeviceIndex);
	}
	else
	{
		DisposeHandle(file);
		return 0;
	}
	printf("Getting keyboard devices again\n");
	if (KeyboardGetDevices(file, &devices))
	{
		printf("No. of devices: %u, Active device: %u\n", devices.NumberOfDevices, devices.ActiveDeviceId);
	}
	else
	{
		DisposeHandle(file);
		return 0;
	}
	printf("Getting keyboard attributes\n");
	KEYBOARD_ATTRIBUTES attribs;
	if (KeyboardGetAttributes(file, &attribs)) 
	{
		printf("Attributes:\n"
			"	KeyboardIdentifier-Type:			%c\n"
			"	KeyboardIdentifier-Subtype:			%c\n"
			"	KeyboardMode:						%u\n"
			"	InputDataQueueLength:				%u\n"
			"	NumberOfFunctionKeys:				%u\n"
			"	NumberOfIndicators:					%u\n"
			"	NumberOfKeysTotal:					%u\n"
			"	KeyRepeatMaximum-UnitId:			%u\n"
			"	KeyRepeatMaximum-Delay:				%u\n"
			"	KeyRepeatMaximum-Rate:				%u\n"
			"	KeyRepeatMinimum-UnitId:			%u\n"
			"	KeyRepeatMinimum-Delay:				%u\n"
			"	KeyRepeatMinimum-Rate:				%u\n",
			attribs.KeyboardIdentifier.Type,
			attribs.KeyboardIdentifier.Subtype,
			attribs.KeyboardMode,
			attribs.InputDataQueueLength,
			attribs.NumberOfFunctionKeys,
			attribs.NumberOfIndicators,
			attribs.NumberOfKeysTotal,
			attribs.KeyRepeatMaximum.UnitId,
			attribs.KeyRepeatMaximum.Delay, 
			attribs.KeyRepeatMaximum.Rate,
			attribs.KeyRepeatMinimum.UnitId,
			attribs.KeyRepeatMinimum.Delay, 
			attribs.KeyRepeatMinimum.Rate);
	}
	else
	{
		DisposeHandle(file);
		return 0;
	}
	printf("Setting keyboard filters\n");
	KEY_FILTER_REQUEST filterReq;
	filterReq.FilterMode = FILTER_KEY_FLAG_AND_SCANCODE;
	filterReq.FilterCount = 2;
	KEY_FILTER_DATA filterData[3];
	filterData[0].FlagPredicates = FLAG_KEY_ALL;
	filterData[0].ScanCode = 1;
	filterData[1].FlagPredicates = FLAG_KEY_DOWN;
	filterData[1].ScanCode = 2;
	filterReq.FilterData = filterData;
	if (KeyboardSetKeyFiltering(file, &filterReq)) {
		printf("Getting keyboard filters\n");
		if (KeyboardGetKeyFiltering(file, &filterReq))
		{
			printf("Filter mode: %u\n", filterReq.FilterMode);
			for (size_t i = 0; i < filterReq.FilterCount; i++)
			{
				printf("Filter Flag: %u, Filter Scan code: %u\n", filterReq.FilterData[i].FlagPredicates, filterReq.FilterData[i].ScanCode);
			}
		}

		printf("Adding a keyboard filter\n");
		KEY_FILTER_DATA addData;
		addData.ScanCode = 3;
		addData.FlagPredicates = FLAG_KEY_PRESS;
		if (KeyboardAddKeyFiltering(file, &addData)) {
			printf("Getting keyboard filters\n");
			filterReq.FilterCount = 3;
			if (KeyboardGetKeyFiltering(file, &filterReq))
			{
				printf("Filter mode: %u\n", filterReq.FilterMode);
				for (size_t i = 0; i < filterReq.FilterCount; i++)
				{
					printf("Filter Flag: %u, Filter Scan code: %u\n", filterReq.FilterData[i].FlagPredicates, filterReq.FilterData[i].ScanCode);
				}
			}
		}
		printf("Removing a keyboard filter #0\n");
		if (KeyboardRemoveKeyFiltering(file, &filterData[0])) {
			printf("Getting keyboard filters\n");
			if (KeyboardGetKeyFiltering(file, &filterReq))
			{
				printf("Filter mode: %u\n", filterReq.FilterMode);
				for (size_t i = 0; i < filterReq.FilterCount; i++)
				{
					printf("Filter Flag: %u, Filter Scan code: %u\n", filterReq.FilterData[i].FlagPredicates, filterReq.FilterData[i].ScanCode);
				}
			}
		}
	}


	printf("Setting keyboard modifications\n");
	KEY_MODIFY_REQUEST modifyReq;
	modifyReq.ModifyCount = 2;
	KEY_MODIFY_DATA modifyData[3];
	modifyData[0].FlagPredicates = FLAG_KEY_ALL;
	modifyData[0].FromScanCode = 3;
	modifyData[0].ToScanCode = 4;
	modifyData[1].FlagPredicates = FLAG_KEY_ALL;
	modifyData[1].FromScanCode = 4;
	modifyData[1].ToScanCode = 5;
	modifyReq.ModifyData = modifyData;
	if (KeyboardSetKeyModifying(file, &modifyReq)) {
		printf("Getting keyboard modifications\n");
		if (KeyboardGetKeyModifying(file, &modifyReq))
		{
			for (size_t i = 0; i < modifyReq.ModifyCount; i++)
			{
				printf("Modify Flag: %u, From Scan code: %u, To Scan code: %u\n", modifyReq.ModifyData[i].FlagPredicates, modifyReq.ModifyData[i].FromScanCode, modifyReq.ModifyData[i].ToScanCode);
			}
		}
		printf("Adding a keyboard modification\n");
		KEY_MODIFY_DATA addData;
		addData.FromScanCode = 5;
		addData.ToScanCode = 6;
		addData.FlagPredicates = FLAG_KEY_PRESS;
		if (KeyboardAddKeyModifying(file, &addData)) {
			printf("Getting keyboard modifications\n");
			modifyReq.ModifyCount = 3;
			if (KeyboardGetKeyModifying(file, &modifyReq))
			{
				for (size_t i = 0; i < modifyReq.ModifyCount; i++)
				{
					printf("Modify Flag: %u, From Scan code: %u, To Scan code: %u\n", modifyReq.ModifyData[i].FlagPredicates, modifyReq.ModifyData[i].FromScanCode, modifyReq.ModifyData[i].ToScanCode);
				}
			}
		}
		printf("Removing a keyboard modification #1\n");
		if (KeyboardRemoveKeyModifying(file, &modifyData[1])) {
			printf("Getting keyboard modifications\n");
			if (KeyboardGetKeyModifying(file, &modifyReq))
			{
				for (size_t i = 0; i < modifyReq.ModifyCount; i++)
				{
					printf("Modify Flag: %u, From Scan code: %u, To Scan code: %u\n", modifyReq.ModifyData[i].FlagPredicates, modifyReq.ModifyData[i].FromScanCode, modifyReq.ModifyData[i].ToScanCode);
				}
			}
		}
	}

	Sleep(1000);
	KEYBOARD_INPUT_DATA                 keyInput2[2] = { 0 };
	keyInput2[0].UnitId = 0;
	keyInput2[0].MakeCode = 0x3;
	keyInput2[0].Flags = KBD_KEY_DOWN;//pressed
	keyInput2[1].UnitId = 0;
	keyInput2[1].MakeCode = 0x3;
	keyInput2[1].Flags = KBD_KEY_UP;//release
	KeyboardInsertKeys(file, keyInput2, 2);


	CloseHandle(file);
	return 0;
}




