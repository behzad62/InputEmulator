/*++

Module Name:

    KeyboardEmu.h

Abstract:

    This module contains the common private declarations for the keyboard
    packet filter

Environment:

    kernel mode only

--*/

#ifndef KBEMU_H
#define KBEMU_H

#pragma warning(disable:4201)

#include "ntddk.h"
#include "kbdmou.h"
#include <ntddkbd.h>
#include <ntdd8042.h>

#pragma warning(default:4201)

#include <wdf.h>

#define NTSTRSAFE_LIB
#include <ntstrsafe.h>

#include "public.h"

#define KEYBOARD_POOL_TAG (ULONG) 'kemu'

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
	// The keyboard key filtering request
	//
	KEY_FILTER_REQUEST FilterRequest;
	//
	//The keyboard key modify request
	//
	KEY_MODIFY_REQUEST ModifyRequest;
    //
    // Cached Keyboard Attributes
    //
    KEYBOARD_ATTRIBUTES KeyboardAttributes;

} FILTER_DEVICE_EXTENSION, *PFILTER_DEVICE_EXTENSION;

typedef struct _CONTROL_DEVICE_EXTENSION {
	//
	//Spin lock to synch input tempering
	//
	WDFSPINLOCK SpinLock;
	//
	//Current input device target which receives filtering etc.
	//
	USHORT   ActiveKeyboardId;
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

#define NTDEVICE_NAME_STRING      L"\\Device\\KeyboardEmulator"
#define SYMBOLIC_NAME_STRING      L"\\DosDevices\\KeyboardEmulator"





//
// Prototypes
//
DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_DEVICE_ADD KbFilter_EvtDeviceAdd;
EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL KbFilter_EvtIoDeviceControl;
EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL KbFilter_EvtIoInternalDeviceControl;
EVT_WDF_DEVICE_CONTEXT_CLEANUP KbFilter_EvtDeviceContextCleanup;
EVT_WDF_REQUEST_COMPLETION_ROUTINE KbFilter_RequestCompletionRoutine;

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
On_IOCTL_KEYBOARD_INSERT_KEY(
	IN PKEYBOARD_INPUT_DATA InputDataStart, 
	IN size_t InputCount, 
	IN PFILTER_DEVICE_EXTENSION FilterExtension);

VOID
KbFilter_ServiceCallback(
    IN PDEVICE_OBJECT DeviceObject,
    IN PKEYBOARD_INPUT_DATA InputDataStart,
    IN PKEYBOARD_INPUT_DATA InputDataEnd,
    IN OUT PULONG InputDataConsumed
    );

_Function_class_(IO_WORKITEM_ROUTINE)
VOID
SetCurrentInputDevice(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_opt_ PVOID Context);

#endif  // KBEMU_H

