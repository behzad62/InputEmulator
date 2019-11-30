/*++

Module Name:

	MouseEmu.h

Abstract:

	This module contains the common private declarations for the mouse
	packet filter

Environment:

	kernel mode only

Notes:


Revision History:


--*/

#ifndef MOUEMU_H
#define MOUEMU_H

#pragma warning(disable:4201)
#include <ntddk.h>
#include <kbdmou.h>
#include <ntddmou.h>
#include <ntdd8042.h>
#pragma warning(default:4201)

#include <wdf.h>
#define NTSTRSAFE_LIB
#include <ntstrsafe.h>
#include "public.h"

#define MOUSE_POOL_TAG (ULONG) 'memu'

#if DBG

#define TRAP()                      DbgBreakPoint()

#define DebugPrint(_x_) DbgPrint _x_

#else   // DBG

#define TRAP()

#define DebugPrint(_x_)

#endif


typedef struct _FILTER_DEVICE_EXTENSION
{
	//
	//Spin lock to synch input tempering
	//
	WDFSPINLOCK SpinLock;
	//
	// The real connect data that this driver reports to
	//
	CONNECT_DATA UpperConnectData;
	//
	// The mouse filtering request
	//
	USHORT FilterMode;
	//
	//The mouse modify request
	//
	MOUSE_MODIFY_REQUEST ModifyRequest;
	//
	// Cached Keyboard Attributes
	//
	MOUSE_ATTRIBUTES MouseAttributes;

} FILTER_DEVICE_EXTENSION, * PFILTER_DEVICE_EXTENSION;

typedef struct _CONTROL_DEVICE_EXTENSION {
	//
	//Spin lock to synch input tempering
	//
	WDFSPINLOCK SpinLock;
	//
	//Current input device target which receives filtering etc.
	//
	USHORT   ActiveMouseId;
	//
	//Is used to signal when user wants to detect the current input device
	//
	BOOLEAN InputRequired;
	//
	//Work item that sets the current input device when it is detected.
	//
	PIO_WORKITEM InputDeviceWorkItem;
	//
	//Queue to redirect pending IRPs for detecting current input deveice until user press any key
	//
	WDFQUEUE ManualQueue;

} CONTROL_DEVICE_EXTENSION, * PCONTROL_DEVICE_EXTENSION;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(FILTER_DEVICE_EXTENSION, FilterGetData)
WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(CONTROL_DEVICE_EXTENSION, ControlGetData)

#define NTDEVICE_NAME_STRING      L"\\Device\\MouseEmulator"
#define SYMBOLIC_NAME_STRING      L"\\DosDevices\\MouseEmulator"



//
// Prototypes
//
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD MouFilter_EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL MouFilter_EvtIoInternalDeviceControl;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MouFilter_EvtIoDeviceControl;
EVT_WDF_DEVICE_CONTEXT_CLEANUP MouFilter_EvtDeviceContextCleanup;
EVT_WDF_REQUEST_COMPLETION_ROUTINE MouFilter_RequestCompletionRoutine;

_Must_inspect_result_
_Success_(return == STATUS_SUCCESS)
_IRQL_requires_max_(PASSIVE_LEVEL)
NTSTATUS
FilterCreateControlDevice(
	_In_ WDFDEVICE Device
);

_IRQL_requires_max_(PASSIVE_LEVEL)
VOID
FilterDeleteControlDevice(
	_In_ WDFDEVICE Device
);

VOID
On_IOCTL_MOUSE_INSERT_KEY(
	IN PMOUSE_INPUT_DATA InputDataStart,
	IN size_t InputCount,
	IN PFILTER_DEVICE_EXTENSION FilterExtension);


VOID
MouFilter_ServiceCallback(
	IN PDEVICE_OBJECT DeviceObject,
	IN PMOUSE_INPUT_DATA InputDataStart,
	IN PMOUSE_INPUT_DATA InputDataEnd,
	IN OUT PULONG InputDataConsumed
);

_Function_class_(IO_WORKITEM_ROUTINE)
VOID
SetCurrentInputDevice(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_opt_ PVOID Context);

#endif  // MOUEMU_H


