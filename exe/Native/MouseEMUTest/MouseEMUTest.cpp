
#include <iostream>
#include "MouseEmuAPI.h"

int main()
{
	USHORT								currentDeviceIndex = 0;
	HANDLE                              file = NULL;

	file = CreateDriverHandle();
	if (file == INVALID_HANDLE_VALUE)
		return 0;

	printf("Getting mouse attributes\n");
	MOUSE_ATTRIBUTES attribs;
	if (MouseGetAttributes(file, &attribs))
	{
		printf("Attributes:\n"
			"	MouseIdentifier:				%u\n"
			"	InputDataQueueLength:			%u\n"
			"	NumberOfButtons:				%u\n"
			"	SampleRate:						%u\n",
			attribs.MouseIdentifier,
			attribs.InputDataQueueLength,
			attribs.NumberOfButtons,
			attribs.SampleRate);
	}
	else
	{
		DisposeHandle(file);
		return 0;
	}

	DisposeHandle(file);
}
