/*--

	THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF ANY
	KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
	IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A PARTICULAR
	PURPOSE.


Module Name:

	KeyboardEmu.c

--*/

#include "keyboardEmu.h"

//
// Collection object is used to store all the FilterDevice objects so
// that any event callback routine can easily walk thru the list and pick a
// specific instance of the device for filtering.
//
WDFCOLLECTION   FilterDeviceCollection;
WDFWAITLOCK     FilterDeviceCollectionLock;

#ifdef ALLOC_PRAGMA
#pragma alloc_text (INIT, DriverEntry)
#pragma alloc_text (PAGE, KbFilter_EvtDeviceAdd)
#pragma alloc_text (PAGE, KbFilter_EvtIoInternalDeviceControl)
#endif

//
// ControlDevice provides a sideband communication to the filter from
// usermode. This is required if the filter driver is sitting underneath
// another driver that fails custom ioctls defined by the Filter driver.
// Since there is one control-device for all instances of the device the
// filter is attached to, we will store the device handle in a global variable.
//

WDFDEVICE       ControlDevice = NULL;


NTSTATUS
DriverEntry(
	IN PDRIVER_OBJECT  DriverObject,
	IN PUNICODE_STRING RegistryPath
)
/*++

Routine Description:

	Installable driver initialization entry point.
	This entry point is called directly by the I/O system.

Arguments:

	DriverObject - pointer to the driver object

	RegistryPath - pointer to a unicode string representing the path,
				   to driver-specific key in the registry.

Return Value:

	STATUS_SUCCESS if successful,
	STATUS_UNSUCCESSFUL otherwise.

--*/
{
	WDF_DRIVER_CONFIG               config;
	NTSTATUS                        status;

	DebugPrint(("Keyboard Filter Driver Sample - Driver Framework Edition.\n"));
	DebugPrint(("Built %s %s\n", __DATE__, __TIME__));

	//
	// Initialize driver config to control the attributes that
	// are global to the driver. Note that framework by default
	// provides a driver unload routine. If you create any resources
	// in the DriverEntry and want to be cleaned in driver unload,
	// you can override that by manually setting the EvtDriverUnload in the
	// config structure. In general xxx_CONFIG_INIT macros are provided to
	// initialize most commonly used members.
	//

	WDF_DRIVER_CONFIG_INIT(
		&config,
		KbFilter_EvtDeviceAdd
	);

	//
	// Create a framework driver object to represent our driver.
	//
	status = WdfDriverCreate(DriverObject,
		RegistryPath,
		WDF_NO_OBJECT_ATTRIBUTES,
		&config,
		WDF_NO_HANDLE); // hDriver optional
	if (!NT_SUCCESS(status)) {
		DebugPrint(("WdfDriverCreate failed with status 0x%x\n", status));
	}

	//
	// Since there is only one control-device for all the instances
	// of the physical device, we need an ability to get to particular instance
	// of the device in our KbFilter_EvtIoDeviceControl. For that we
	// will create a collection object and store filter device objects.        
	// The collection object has the driver object as a default parent.
	//

	status = WdfCollectionCreate(WDF_NO_OBJECT_ATTRIBUTES,
		&FilterDeviceCollection);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("WdfCollectionCreate failed with status 0x%x\n", status));
		return status;
	}

	//
	// The wait-lock object has the driver object as a default parent.
	//

	status = WdfWaitLockCreate(WDF_NO_OBJECT_ATTRIBUTES,
		&FilterDeviceCollectionLock);
	if (!NT_SUCCESS(status))
	{
		KdPrint(("WdfWaitLockCreate failed with status 0x%x\n", status));
		return status;
	}

	return status;
}

NTSTATUS
KbFilter_EvtDeviceAdd(
	IN WDFDRIVER        Driver,
	IN PWDFDEVICE_INIT  DeviceInit
)
/*++
Routine Description:

	EvtDeviceAdd is called by the framework in response to AddDevice
	call from the PnP manager. Here you can query the device properties
	using WdfFdoInitWdmGetPhysicalDevice/IoGetDeviceProperty and based
	on that, decide to create a filter device object and attach to the
	function stack.

	If you are not interested in filtering this particular instance of the
	device, you can just return STATUS_SUCCESS without creating a framework
	device.

Arguments:

	Driver - Handle to a framework driver object created in DriverEntry

	DeviceInit - Pointer to a framework-allocated WDFDEVICE_INIT structure.

Return Value:

	NTSTATUS

--*/
{
	WDF_OBJECT_ATTRIBUTES		deviceAttributes;
	NTSTATUS					status;
	WDFDEVICE					hDevice;
	PFILTER_DEVICE_EXTENSION    filterExt;
	WDF_IO_QUEUE_CONFIG			ioQueueConfig;


	UNREFERENCED_PARAMETER(Driver);

	PAGED_CODE();

	DebugPrint(("Enter KbFilter_EvtDeviceAdd \n"));

	//
	// Tell the framework that you are filter driver. Framework
	// takes care of inherting all the device flags & characterstics
	// from the lower device you are attaching to.
	//
	WdfFdoInitSetFilter(DeviceInit);

	WdfDeviceInitSetDeviceType(DeviceInit, FILE_DEVICE_KEYBOARD);

	//
	// Specify the size of device extension where we track per device
	// context.
	//
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&deviceAttributes, FILTER_DEVICE_EXTENSION);

	//
	// We will just register for cleanup notification because we have to
	// delete the control-device when the last instance of the device goes
	// away. If we don't delete, the driver wouldn't get unloaded automatically
	// by the PNP subsystem.
	//
	deviceAttributes.EvtCleanupCallback = KbFilter_EvtDeviceContextCleanup;

	//
	// Create a framework device object.  This call will in turn create
	// a WDM deviceobject, attach to the lower stack and set the
	// appropriate flags and attributes.
	//
	status = WdfDeviceCreate(&DeviceInit, &deviceAttributes, &hDevice);
	if (!NT_SUCCESS(status)) {
		DebugPrint(("WdfDeviceCreate failed with status code 0x%x\n", status));
		return status;
	}

	filterExt = FilterGetData(hDevice);
	filterExt->FilterRequest.FilterMode = 0;
	filterExt->FilterRequest.FilterData = NULL;
	filterExt->ModifyRequest.ModifyCount = 0;
	filterExt->ModifyRequest.ModifyData = NULL;
	//
	// Configure the default queue to be Parallel. Do not use sequential queue
	// if this driver is going to be filtering PS2 ports because it can lead to
	// deadlock. The PS2 port driver sends a request to the top of the stack when it
	// receives an ioctl request and waits for it to be completed. If you use a
	// a sequential queue, this request will be stuck in the queue because of the 
	// outstanding ioctl request sent earlier to the port driver.
	//
	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
		WdfIoQueueDispatchParallel);

	//
	// Framework by default creates non-power managed queues for
	// filter drivers.
	//
	ioQueueConfig.EvtIoInternalDeviceControl = KbFilter_EvtIoInternalDeviceControl;

	status = WdfIoQueueCreate(hDevice,
		&ioQueueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		WDF_NO_HANDLE // pointer to default queue
	);
	if (!NT_SUCCESS(status)) {
		DebugPrint(("WdfIoQueueCreate failed 0x%x\n", status));
		return status;
	}

	//
	// Add this device to the FilterDevice collection.
	//
	WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);
	//
	// WdfCollectionAdd takes a reference on the item object and removes
	// it when you call WdfCollectionRemove.
	//
	status = WdfCollectionAdd(FilterDeviceCollection, hDevice);
	if (!NT_SUCCESS(status)) {
		DebugPrint(("WdfCollectionAdd failed with status code 0x%x\n", status));
		//
		// Let us not fail AddDevice just because we weren't able to add this device to the
		// FilterDeviceCollection.
		//
	}
	WdfWaitLockRelease(FilterDeviceCollectionLock);

	//
	// Create a control device
	//
	status = FilterCreateControlDevice(hDevice);
	if (!NT_SUCCESS(status)) {
		DebugPrint(("FilterCreateControlDevice failed with status 0x%x\n",
			status));
		//
		// Let us not fail AddDevice just because we weren't able to create the
		// control device.
		//
		status = STATUS_SUCCESS;
	}
	return status;
}

#pragma warning(push)
#pragma warning(disable:28118) // this callback will run at IRQL=PASSIVE_LEVEL
_Use_decl_annotations_
VOID
KbFilter_EvtDeviceContextCleanup(
	WDFOBJECT Device
)
/*++

Routine Description:

   EvtDeviceRemove event callback must perform any operations that are
   necessary before the specified device is removed. The framework calls
   the driver's EvtDeviceRemove callback when the PnP manager sends
   an IRP_MN_REMOVE_DEVICE request to the driver stack.

Arguments:

	Device - Handle to a framework device object.

Return Value:

	WDF status code

--*/
{
	ULONG						count;
	PFILTER_DEVICE_EXTENSION	filterExt;
	PAGED_CODE();

	DebugPrint(("Entered KbFilter_EvtDeviceContextCleanup\n"));

	WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);

	count = WdfCollectionGetCount(FilterDeviceCollection);

	if (count == 1)
	{
		//
		// We are the last instance. So let us delete the control-device
		// so that driver can unload when the FilterDevice is deleted.
		// We absolutely have to do the deletion of control device with
		// the collection lock acquired because we implicitly use this
		// lock to protect ControlDevice global variable. We need to make
		// sure another thread doesn't attempt to create while we are
		// deleting the device.
		//
		FilterDeleteControlDevice((WDFDEVICE)Device);
	}

	WdfCollectionRemove(FilterDeviceCollection, Device);
	WdfWaitLockRelease(FilterDeviceCollectionLock);
	filterExt = FilterGetData(Device);
	if (filterExt) {
		if (filterExt->FilterRequest.FilterData) {
			ExFreePoolWithTag(filterExt->FilterRequest.FilterData, KEYBOARD_POOL_TAG);
			filterExt->FilterRequest.FilterData = NULL;
		}
		if (filterExt->ModifyRequest.ModifyData) {
			ExFreePoolWithTag(filterExt->ModifyRequest.ModifyData, KEYBOARD_POOL_TAG);
			filterExt->ModifyRequest.ModifyData = NULL;
		}
	}
}
#pragma warning(pop) // enable 28118 again

_Use_decl_annotations_
NTSTATUS
FilterCreateControlDevice(
	WDFDEVICE Device
)
/*++

Routine Description:

	This routine is called to create a control deviceobject so that application
	can talk to the filter driver directly instead of going through the entire
	device stack. This kind of control device object is useful if the filter
	driver is underneath another driver which prevents ioctls not known to it
	or if the driver's dispatch routine is owned by some other (port/class)
	driver and it doesn't allow any custom ioctls.

	NOTE: Since the control device is global to the driver and accessible to
	all instances of the device this filter is attached to, we create only once
	when the first instance of the device is started and delete it when the
	last instance gets removed.

Arguments:

	Device - Handle to a filter device object.

Return Value:

	WDF status code

--*/
{
	PWDFDEVICE_INIT             pInit = NULL;
	WDFDEVICE                   controlDevice = NULL;
	WDF_OBJECT_ATTRIBUTES       controlAttributes;
	WDF_IO_QUEUE_CONFIG         ioQueueConfig;
	BOOLEAN                     bCreate = FALSE;
	NTSTATUS                    status;
	WDFQUEUE                    queue;
	PCONTROL_DEVICE_EXTENSION    controlExt;
	DECLARE_CONST_UNICODE_STRING(ntDeviceName, NTDEVICE_NAME_STRING);
	DECLARE_CONST_UNICODE_STRING(symbolicLinkName, SYMBOLIC_NAME_STRING);

	PAGED_CODE();

	//
	// First find out whether any ControlDevice has been created. If the
	// collection has more than one device then we know somebody has already
	// created or in the process of creating the device.
	//
	WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);

	if (WdfCollectionGetCount(FilterDeviceCollection) == 1) {
		bCreate = TRUE;
	}

	WdfWaitLockRelease(FilterDeviceCollectionLock);

	if (!bCreate) {
		//
		// Control device is already created. So return success.
		//
		return STATUS_SUCCESS;
	}

	DebugPrint(("Creating Control Device\n"));

	//
	//
	// In order to create a control device, we first need to allocate a
	// WDFDEVICE_INIT structure and set all properties.
	//
	pInit = WdfControlDeviceInitAllocate(
		WdfDeviceGetDriver(Device),
		&SDDL_DEVOBJ_SYS_ALL_ADM_RWX_WORLD_RW_RES_R
	);

	if (pInit == NULL) {
		status = STATUS_INSUFFICIENT_RESOURCES;
		goto Error;
	}

	//
	// Set exclusive to false so that more than one app can talk to the
	// control device simultaneously.
	//
	WdfDeviceInitSetExclusive(pInit, FALSE);

	status = WdfDeviceInitAssignName(pInit, &ntDeviceName);

	if (!NT_SUCCESS(status)) {
		goto Error;
	}

	//
	// Specify the size of device context
	//
	WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&controlAttributes, CONTROL_DEVICE_EXTENSION);

	status = WdfDeviceCreate(&pInit,
		&controlAttributes,
		&controlDevice);
	if (!NT_SUCCESS(status)) {
		goto Error;
	}

	controlExt = ControlGetData(controlDevice);
	controlExt->InputRequired = FALSE;
	status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &controlExt->SpinLock);

	if (!NT_SUCCESS(status)) {
		DebugPrint(("WdfSpinLockCreate failed %x\n", status));
		goto Error;
	}
	//
	// Create a symbolic link for the control object so that usermode can open
	// the device.
	//

	status = WdfDeviceCreateSymbolicLink(controlDevice,
		&symbolicLinkName);

	if (!NT_SUCCESS(status)) {
		goto Error;
	}

	//
	// Configure the default queue associated with the control device object
	// to be Serial so that request passed to EvtIoDeviceControl are serialized.
	//

	WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&ioQueueConfig,
		WdfIoQueueDispatchSequential);

	ioQueueConfig.EvtIoDeviceControl = KbFilter_EvtIoDeviceControl;

	//
	// Framework by default creates non-power managed queues for
	// filter drivers.
	//
	status = WdfIoQueueCreate(controlDevice,
		&ioQueueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&queue // pointer to default queue
	);
	if (!NT_SUCCESS(status)) {
		goto Error;
	}
	//
	//Creating a manual queue to redirect Detect_Device_Id Ioctls 
	//
	WDF_IO_QUEUE_CONFIG_INIT(&ioQueueConfig, WdfIoQueueDispatchManual);

	status = WdfIoQueueCreate(controlDevice,
		&ioQueueConfig,
		WDF_NO_OBJECT_ATTRIBUTES,
		&controlExt->ManualQueue // pointer to manual queue
	);
	if (!NT_SUCCESS(status)) {
		goto Error;
	}

	//
	// Control devices must notify WDF when they are done initializing.   I/O is
	// rejected until this call is made.
	//
	WdfControlFinishInitializing(controlDevice);

	ControlDevice = controlDevice;

	return STATUS_SUCCESS;

Error:

	if (pInit != NULL) {
		WdfDeviceInitFree(pInit);
	}

	if (controlDevice != NULL) {
		//
		// Release the reference on the newly created object, since
		// we couldn't initialize it.
		//
		WdfObjectDelete(controlDevice);
		controlDevice = NULL;
	}

	return status;
}

_Use_decl_annotations_
VOID
FilterDeleteControlDevice(
	WDFDEVICE Device
)
/*++

Routine Description:

	This routine deletes the control by doing a simple dereference.

Arguments:

	Device - Handle to a framework filter device object.

Return Value:

	WDF status code

--*/
{
	UNREFERENCED_PARAMETER(Device);

	PAGED_CODE();

	KdPrint(("Deleting Control Device\n"));

	if (ControlDevice) {
		WdfObjectDelete(ControlDevice);
		ControlDevice = NULL;
	}
}

VOID
KbFilter_EvtIoInternalDeviceControl(
	IN WDFQUEUE      Queue,
	IN WDFREQUEST    Request,
	IN size_t        OutputBufferLength,
	IN size_t        InputBufferLength,
	IN ULONG         IoControlCode
)
/*++

Routine Description:

	This routine is the dispatch routine for internal device control requests.
	There are two specific control codes that are of interest:

	IOCTL_INTERNAL_KEYBOARD_CONNECT:
		Store the old context and function pointer and replace it with our own.
		This makes life much simpler than intercepting IRPs sent by the RIT and
		modifying them on the way back up.

	IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
		Add in the necessary function pointers and context values so that we can
		alter how the ps/2 keyboard is initialized.

	NOTE:  Handling IOCTL_INTERNAL_I8042_HOOK_KEYBOARD is *NOT* necessary if
		   all you want to do is filter KEYBOARD_INPUT_DATAs.  You can remove
		   the handling code and all related device extension fields and
		   functions to conserve space.

Arguments:

	Queue - Handle to the framework queue object that is associated
			with the I/O request.
	Request - Handle to a framework request object.

	OutputBufferLength - length of the request's output buffer,
						if an output buffer is available.
	InputBufferLength - length of the request's input buffer,
						if an input buffer is available.

	IoControlCode - the driver-defined or system-defined I/O control code
					(IOCTL) that is associated with the request.

Return Value:

   VOID

--*/
{
	PFILTER_DEVICE_EXTENSION        filterExt;
	PCONNECT_DATA                   connectData = NULL;
	NTSTATUS                        status = STATUS_SUCCESS;
	size_t                          length;
	WDFDEVICE                       hDevice;
	BOOLEAN                         forwardWithCompletionRoutine = FALSE;
	BOOLEAN                         ret = TRUE;
	WDFCONTEXT                      completionContext = WDF_NO_CONTEXT;
	WDF_REQUEST_SEND_OPTIONS        options;
	WDFMEMORY                       outputMemory;
	UNREFERENCED_PARAMETER(OutputBufferLength);
	UNREFERENCED_PARAMETER(InputBufferLength);


	PAGED_CODE();

	DebugPrint(("Entered KbFilter_EvtIoInternalDeviceControl\n"));

	hDevice = WdfIoQueueGetDevice(Queue);
	filterExt = FilterGetData(hDevice);

	switch (IoControlCode) {

		//
		// Connect a keyboard class device driver to the port driver.
		//
	case IOCTL_INTERNAL_KEYBOARD_CONNECT:
		DebugPrint(("Received IOCTL_INTERNAL_KEYBOARD_CONNECT\n"));
		//
		// Only allow one connection.
		//
		if (filterExt->UpperConnectData.ClassService != NULL) {
			status = STATUS_SHARING_VIOLATION;
			break;
		}
		//
		// Get the input buffer from the request
		// (Parameters.DeviceIoControl.Type3InputBuffer).
		//
		status = WdfRequestRetrieveInputBuffer(Request,
			sizeof(CONNECT_DATA),
			&connectData,
			&length);
		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestRetrieveInputBuffer failed %x\n", status));
			break;
		}

		NT_ASSERT(length == InputBufferLength);

		status = WdfSpinLockCreate(WDF_NO_OBJECT_ATTRIBUTES, &filterExt->SpinLock);
		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfSpinLockCreate failed %x\n", status));
			break;
		}

		filterExt->UpperConnectData = *connectData;

		//
		// Hook into the report chain.  Everytime a keyboard packet is reported
		// to the system, KbFilter_ServiceCallback will be called
		//

		connectData->ClassDeviceObject = WdfDeviceWdmGetDeviceObject(hDevice);

#pragma warning(disable:4152)  //nonstandard extension, function/data pointer conversion

		connectData->ClassService = KbFilter_ServiceCallback;

#pragma warning(default:4152)

		break;

		//
		// Disconnect a keyboard class device driver from the port driver.
		//
	case IOCTL_INTERNAL_KEYBOARD_DISCONNECT:
		DebugPrint(("Received IOCTL_INTERNAL_KEYBOARD_DISCONNECT\n"));
		//
		// Clear the connection parameters in the device extension.
		//
		// devExt->UpperConnectData.ClassDeviceObject = NULL;
		// devExt->UpperConnectData.ClassService = NULL;

		status = STATUS_NOT_IMPLEMENTED;
		break;


	case IOCTL_KEYBOARD_QUERY_ATTRIBUTES:
		DebugPrint(("Received IOCTL_KEYBOARD_QUERY_ATTRIBUTES\n"));
		forwardWithCompletionRoutine = TRUE;
		completionContext = filterExt;
		break;

		//
		// Might want to capture these in the future.  For now, then pass them down
		// the stack.  These queries must be successful for the RIT to communicate
		// with the keyboard.
		//
	case IOCTL_INTERNAL_I8042_HOOK_KEYBOARD:
	case IOCTL_KEYBOARD_QUERY_INDICATOR_TRANSLATION:
	case IOCTL_KEYBOARD_QUERY_INDICATORS:
	case IOCTL_KEYBOARD_SET_INDICATORS:
	case IOCTL_KEYBOARD_QUERY_TYPEMATIC:
	case IOCTL_KEYBOARD_SET_TYPEMATIC:
		break;
	}

	if (!NT_SUCCESS(status)) {
		WdfRequestComplete(Request, status);
		return;
	}

	//
	// Forward the request down. WdfDeviceGetIoTarget returns
	// the default target, which represents the device attached to us below in
	// the stack.
	//

	if (forwardWithCompletionRoutine) {

		//
		// Format the request with the output memory so the completion routine
		// can access the return data in order to cache it into the context area
		//

		status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);

		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestRetrieveOutputMemory failed: 0x%x\n", status));
			WdfRequestComplete(Request, status);
			return;
		}

		status = WdfIoTargetFormatRequestForInternalIoctl(WdfDeviceGetIoTarget(hDevice),
			Request,
			IoControlCode,
			NULL,
			NULL,
			outputMemory,
			NULL);

		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfIoTargetFormatRequestForInternalIoctl failed: 0x%x\n", status));
			WdfRequestComplete(Request, status);
			return;
		}

		// 
		// Set our completion routine with a context area that we will save
		// the output data into
		//
		WdfRequestSetCompletionRoutine(Request,
			KbFilter_RequestCompletionRoutine,
			completionContext);

		ret = WdfRequestSend(Request,
			WdfDeviceGetIoTarget(hDevice),
			WDF_NO_SEND_OPTIONS);

		if (ret == FALSE) {
			status = WdfRequestGetStatus(Request);
			DebugPrint(("WdfRequestSend failed: 0x%x\n", status));
			WdfRequestComplete(Request, status);
		}

	}
	else
	{

		//
		// We are not interested in post processing the IRP so 
		// fire and forget.
		//
		WDF_REQUEST_SEND_OPTIONS_INIT(&options,
			WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

		ret = WdfRequestSend(Request, WdfDeviceGetIoTarget(hDevice), &options);

		if (ret == FALSE) {
			status = WdfRequestGetStatus(Request);
			DebugPrint(("WdfRequestSend failed: 0x%x\n", status));
			WdfRequestComplete(Request, status);
		}

	}

	return;
}

VOID
KbFilter_EvtIoDeviceControl(
	IN WDFQUEUE      Queue,
	IN WDFREQUEST    Request,
	IN size_t        OutputBufferLength,
	IN size_t        InputBufferLength,
	IN ULONG         IoControlCode
)
/*++

Routine Description:

	This routine is the dispatch routine for control device control requests.

Arguments:

	Queue - Handle to the framework queue object that is associated
			with the I/O request.
	Request - Handle to a framework request object.

	OutputBufferLength - length of the request's output buffer,
						if an output buffer is available.
	InputBufferLength - length of the request's input buffer,
						if an input buffer is available.

	IoControlCode - the driver-defined or system-defined I/O control code
					(IOCTL) that is associated with the request.

Return Value:

   VOID

--*/
{
	NTSTATUS status = STATUS_SUCCESS;
	PFILTER_DEVICE_EXTENSION	filterExt;
	PCONTROL_DEVICE_EXTENSION	controlExt;
	WDFMEMORY					outputMemory;
	WDFMEMORY					inputMemory;
	size_t						bytesTransferred = 0;
	size_t						inputCount;
	size_t						requiredBytes;
	USHORT						noItems;
	WDFDEVICE					hFilterDevice;
	KEYBOARD_QUERY_RESULT		keboardIds = { 0 };
	PUSHORT                     keyboardIdBuffer;
	PKEYBOARD_INPUT_DATA        inputData;
	size_t						bufferSize;
	UNREFERENCED_PARAMETER(Queue);

	PAGED_CODE();

	DebugPrint(("Entered KbFilter_EvtIoDeviceControl\n"));
	controlExt = ControlGetData(ControlDevice);

	//hDevice = WdfIoQueueGetDevice(Queue);
	//devExt = FilterGetData(hDevice);

	//
	// Process the ioctl and complete it when you are done.
	//

	switch (IoControlCode) {
	case IOCTL_KEYBOARD_SET_DEVICE_ID:
#pragma region IOCTL_KEYBOARD_SET_DEVICE_ID
		DebugPrint(("Received IOCTL_KEYBOARD_SET_DEVICE_ID\n"));
		if (InputBufferLength < sizeof(USHORT)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		//there is not much contention if any at all over FilterDeviceCollectionLock, hence lets use this lock to protect controlExt->ActiveKeyboardId too.
		status = WdfRequestRetrieveInputBuffer(Request, sizeof(USHORT), &keyboardIdBuffer, &bytesTransferred);
		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestRetrieveInputBuffer failed %x\n", status));
			break;
		}
		NT_ASSERT(bytesTransferred == InputBufferLength);

		WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);

		noItems = (USHORT)WdfCollectionGetCount(FilterDeviceCollection);
		if (noItems == 0) {
			status = STATUS_INVALID_PARAMETER;
			controlExt->ActiveMouseId = 0;
			DebugPrint(("Not any filter device found.\n"));
			WdfWaitLockRelease(FilterDeviceCollectionLock);
			break;
		}
		else if (*keyboardIdBuffer >= noItems)
		{
			status = STATUS_INVALID_PARAMETER;
			WdfWaitLockRelease(FilterDeviceCollectionLock);
			break;
		}
		controlExt->ActiveMouseId = *keyboardIdBuffer;
		WdfWaitLockRelease(FilterDeviceCollectionLock);
#pragma endregion
		break;
	case IOCTL_KEYBOARD_INSERT_KEY:
#pragma region IOCTL_KEYBOARD_INSERT_KEY
		DebugPrint(("Received IOCTL_KEYBOARD_INSERT_KEY\n"));
		//
		// Buffer is too small, fail the request
		//
		if (InputBufferLength < sizeof(KEYBOARD_INPUT_DATA)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		status = WdfRequestRetrieveInputMemory(Request, &inputMemory);

		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestRetrieveInputMemory failed %x\n", status));
			break;
		}
		inputData = WdfMemoryGetBuffer(inputMemory, &bytesTransferred);
		if (inputData == NULL) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint(("WdfMemoryGetBuffer failed.\n"));
			break;
		}
		NT_ASSERT(bytesTransferred == InputBufferLength);

		WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);

		noItems = (USHORT)WdfCollectionGetCount(FilterDeviceCollection);
		if (noItems == 0) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint(("Not any filter device found.\n"));
			WdfWaitLockRelease(FilterDeviceCollectionLock);
			break;
		}

		hFilterDevice = WdfCollectionGetItem(FilterDeviceCollection, controlExt->ActiveMouseId);

		WdfWaitLockRelease(FilterDeviceCollectionLock);

		filterExt = FilterGetData(hFilterDevice);
		inputCount = (bytesTransferred / sizeof(KEYBOARD_INPUT_DATA));

		On_IOCTL_KEYBOARD_INSERT_KEY(inputData, inputCount, filterExt);
#pragma endregion
		break;
	case IOCTL_KEYBOARD_SET_FILTER:
#pragma region IOCTL_KEYBOARD_SET_FILTER
		DebugPrint(("Received IOCTL_KEYBOARD_SET_FILTER\n"));
		//
		// Buffer is too small, fail the request
		//
		if (InputBufferLength < sizeof(USHORT)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		status = WdfRequestRetrieveInputMemory(Request, &inputMemory);

		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestRetrieveInputMemory failed %x\n", status));
			break;
		}

		WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);

		noItems = (USHORT)WdfCollectionGetCount(FilterDeviceCollection);
		if (noItems == 0) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint(("Not any filter device found.\n"));
			WdfWaitLockRelease(FilterDeviceCollectionLock);
			break;
		}

		hFilterDevice = WdfCollectionGetItem(FilterDeviceCollection, controlExt->ActiveMouseId);

		WdfWaitLockRelease(FilterDeviceCollectionLock);

		filterExt = FilterGetData(hFilterDevice);

		WdfSpinLockAcquire(filterExt->SpinLock);
		//first we should clear previously allocated buffer and reset filters
		filterExt->FilterRequest.FilterMode = FILTER_KEY_NONE;
		filterExt->FilterRequest.FilterCount = 0;
		if (filterExt->FilterRequest.FilterData) {
			ExFreePoolWithTag(filterExt->FilterRequest.FilterData, KEYBOARD_POOL_TAG);
			filterExt->FilterRequest.FilterData = NULL;
		}
		status = WdfMemoryCopyToBuffer(inputMemory, 0, &filterExt->FilterRequest.FilterMode, sizeof(filterExt->FilterRequest.FilterMode));
		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfMemoryCopyToBuffer failed %x\n", status));
			WdfSpinLockRelease(filterExt->SpinLock);
			break;
		}

		if (filterExt->FilterRequest.FilterMode == FILTER_KEY_FLAGS) {
			//checking input length
			if (InputBufferLength < sizeof(USHORT) * 2) {
				status = STATUS_BUFFER_TOO_SMALL;
				WdfSpinLockRelease(filterExt->SpinLock);
				break;
			}
			//In this filter mode we use FilterCount as flag
			status = WdfMemoryCopyToBuffer(inputMemory, sizeof(filterExt->FilterRequest.FilterMode), &filterExt->FilterRequest.FilterCount, sizeof(filterExt->FilterRequest.FilterCount));
			if (!NT_SUCCESS(status)) {
				DebugPrint(("WdfMemoryCopyToBuffer failed %x\n", status));
				WdfSpinLockRelease(filterExt->SpinLock);
				break;
			}
		}
		else if (filterExt->FilterRequest.FilterMode == FILTER_KEY_FLAG_AND_SCANCODE)
		{
			if (InputBufferLength < sizeof(USHORT) * 2) {
				status = STATUS_BUFFER_TOO_SMALL;
				WdfSpinLockRelease(filterExt->SpinLock);
				break;
			}
			status = WdfMemoryCopyToBuffer(inputMemory, sizeof(filterExt->FilterRequest.FilterMode), &filterExt->FilterRequest.FilterCount, sizeof(filterExt->FilterRequest.FilterCount));
			if (!NT_SUCCESS(status)) {
				DebugPrint(("WdfMemoryCopyToBuffer failed %x\n", status));
				WdfSpinLockRelease(filterExt->SpinLock);
				break;
			}

			if (filterExt->FilterRequest.FilterCount > 0) {
				requiredBytes = filterExt->FilterRequest.FilterCount * sizeof(KEY_FILTER_DATA);
				if (InputBufferLength < requiredBytes + sizeof(USHORT) * 2) {
					status = STATUS_BUFFER_TOO_SMALL;
					WdfSpinLockRelease(filterExt->SpinLock);
					break;
				}

				filterExt->FilterRequest.FilterData = (PKEY_FILTER_DATA)ExAllocatePoolWithTag(NonPagedPool, requiredBytes, KEYBOARD_POOL_TAG);
				if (filterExt->FilterRequest.FilterData == NULL) {
					status = STATUS_INSUFFICIENT_RESOURCES;
					WdfSpinLockRelease(filterExt->SpinLock);
					break;
				}

				status = WdfMemoryCopyToBuffer(inputMemory, sizeof(USHORT) * 2, filterExt->FilterRequest.FilterData, requiredBytes);
				if (!NT_SUCCESS(status)) {
					DebugPrint(("WdfMemoryCopyToBuffer failed %x\n", status));
					WdfSpinLockRelease(filterExt->SpinLock);
					break;
				}
			}
		}
		WdfSpinLockRelease(filterExt->SpinLock);
#pragma endregion
		break;
	case IOCTL_KEYBOARD_SET_MODIFY:
#pragma region IOCTL_KEYBOARD_SET_MODIFY
		DebugPrint(("Received IOCTL_KEYBOARD_SET_MODIFY\n"));
		//
		// Buffer is too small, fail the request
		//
		if (InputBufferLength < sizeof(USHORT)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		status = WdfRequestRetrieveInputMemory(Request, &inputMemory);

		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestRetrieveInputMemory failed %x\n", status));
			break;
		}

		WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);

		noItems = (USHORT)WdfCollectionGetCount(FilterDeviceCollection);
		if (noItems == 0) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint(("Not any filter device found.\n"));
			WdfWaitLockRelease(FilterDeviceCollectionLock);
			break;
		}

		hFilterDevice = WdfCollectionGetItem(FilterDeviceCollection, controlExt->ActiveMouseId);

		WdfWaitLockRelease(FilterDeviceCollectionLock);

		filterExt = FilterGetData(hFilterDevice);

		WdfSpinLockAcquire(filterExt->SpinLock);
		//first we should clear previously allocated buffer and reset mofify count
		filterExt->ModifyRequest.ModifyCount = 0;
		if (filterExt->ModifyRequest.ModifyData) {
			ExFreePoolWithTag(filterExt->ModifyRequest.ModifyData, KEYBOARD_POOL_TAG);
			filterExt->ModifyRequest.ModifyData = NULL;
		}
		status = WdfMemoryCopyToBuffer(inputMemory, 0, &filterExt->ModifyRequest.ModifyCount, sizeof(filterExt->ModifyRequest.ModifyCount));
		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfMemoryCopyToBuffer failed %x\n", status));
			WdfSpinLockRelease(filterExt->SpinLock);
			break;
		}

		if (filterExt->ModifyRequest.ModifyCount > 0) {
			requiredBytes = filterExt->ModifyRequest.ModifyCount * sizeof(KEY_MODIFY_DATA);
			if (InputBufferLength < requiredBytes + sizeof(USHORT))//buffer size does not match
			{
				status = STATUS_BUFFER_TOO_SMALL;
				WdfSpinLockRelease(filterExt->SpinLock);
				break;
			}
			filterExt->ModifyRequest.ModifyData = (PKEY_MODIFY_DATA)ExAllocatePoolWithTag(NonPagedPool, requiredBytes, KEYBOARD_POOL_TAG);
			if (filterExt->ModifyRequest.ModifyData == NULL) {
				status = STATUS_INSUFFICIENT_RESOURCES;
				WdfSpinLockRelease(filterExt->SpinLock);
				break;
			}
			status = WdfMemoryCopyToBuffer(inputMemory, sizeof(filterExt->ModifyRequest.ModifyCount), filterExt->ModifyRequest.ModifyData, requiredBytes);
			if (!NT_SUCCESS(status)) {
				DebugPrint(("WdfMemoryCopyToBuffer failed %x\n", status));
				WdfSpinLockRelease(filterExt->SpinLock);
				break;
			}
		}
		WdfSpinLockRelease(filterExt->SpinLock);
#pragma endregion
		break;
	case IOCTL_KEYBOARD_GET_FILTER:
#pragma region IOCTL_KEYBOARD_GET_FILTER
		DebugPrint(("Received IOCTL_KEYBOARD_GET_FILTER\n"));
		//
		// Buffer is too small, fail the request
		//
		if (OutputBufferLength < sizeof(USHORT)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);

		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestRetrieveOutputMemory failed %x\n", status));
			break;
		}

		PUSHORT filterQueryBuffer = (PUSHORT)WdfMemoryGetBuffer(outputMemory, &bufferSize);
		if (filterQueryBuffer == NULL) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint(("WdfMemoryGetBuffer failed.\n"));
			break;
		}
		NT_ASSERT(bufferSize == OutputBufferLength);
		WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);

		noItems = (USHORT)WdfCollectionGetCount(FilterDeviceCollection);
		if (noItems == 0) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint(("Not any filter device found.\n"));
			WdfWaitLockRelease(FilterDeviceCollectionLock);
			break;
		}

		hFilterDevice = WdfCollectionGetItem(FilterDeviceCollection, controlExt->ActiveMouseId);

		WdfWaitLockRelease(FilterDeviceCollectionLock);

		filterExt = FilterGetData(hFilterDevice);

		WdfSpinLockAcquire(filterExt->SpinLock);
		*filterQueryBuffer = filterExt->FilterRequest.FilterMode;
		filterQueryBuffer++;
		bytesTransferred += sizeof(filterExt->FilterRequest.FilterMode);
		if (bufferSize - bytesTransferred >= sizeof(USHORT)) {
			*filterQueryBuffer = filterExt->FilterRequest.FilterCount;
			filterQueryBuffer++;
			bytesTransferred += sizeof(filterExt->FilterRequest.FilterCount);
			USHORT i = 0;
			PKEY_FILTER_DATA filterData = (PKEY_FILTER_DATA)filterQueryBuffer;
			while (i < filterExt->FilterRequest.FilterCount && bytesTransferred < bufferSize && bufferSize - bytesTransferred >= sizeof(KEY_FILTER_DATA))
			{
				*filterData = filterExt->FilterRequest.FilterData[i];
				filterData++;
				bytesTransferred += sizeof(KEY_FILTER_DATA);
				i++;
			}
		}
		WdfSpinLockRelease(filterExt->SpinLock);
#pragma endregion
		break;
	case IOCTL_KEYBOARD_GET_MODIFY:
#pragma region IOCTL_KEYBOARD_GET_MODIFY
		DebugPrint(("Received IOCTL_KEYBOARD_GET_MODIFY\n"));
		//
		// Buffer is too small, fail the request
		//
		if (OutputBufferLength < sizeof(USHORT)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);

		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestRetrieveOutputMemory failed %x\n", status));
			break;
		}

		PUSHORT modifyQueryBuffer = (PUSHORT)WdfMemoryGetBuffer(outputMemory, &bufferSize);
		if (modifyQueryBuffer == NULL) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint(("WdfMemoryGetBuffer failed.\n"));
			break;
		}
		NT_ASSERT(bufferSize == OutputBufferLength);

		WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);

		noItems = (USHORT)WdfCollectionGetCount(FilterDeviceCollection);
		if (noItems == 0) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint(("Not any filter device found.\n"));
			WdfWaitLockRelease(FilterDeviceCollectionLock);
			break;
		}

		hFilterDevice = WdfCollectionGetItem(FilterDeviceCollection, controlExt->ActiveMouseId);

		WdfWaitLockRelease(FilterDeviceCollectionLock);

		filterExt = FilterGetData(hFilterDevice);

		WdfSpinLockAcquire(filterExt->SpinLock);

		*modifyQueryBuffer = filterExt->ModifyRequest.ModifyCount;
		modifyQueryBuffer++;
		bytesTransferred += sizeof(filterExt->ModifyRequest.ModifyCount);
		PKEY_MODIFY_DATA modifyData = (PKEY_MODIFY_DATA)modifyQueryBuffer;
		USHORT i = 0;
		while (i < filterExt->ModifyRequest.ModifyCount && bytesTransferred < bufferSize && bufferSize - bytesTransferred >= sizeof(KEY_MODIFY_DATA))
		{
			*modifyData = filterExt->ModifyRequest.ModifyData[i];
			modifyData++;
			bytesTransferred += sizeof(KEY_MODIFY_DATA);
			i++;
		}
		WdfSpinLockRelease(filterExt->SpinLock);
#pragma endregion
		break;
	case IOCTL_KEYBOARD_GET_ATTRIBUTES:
#pragma region IOCTL_KEYBOARD_GET_ATTRIBUTES
		DebugPrint(("Received IOCTL_KEYBOARD_GET_ATTRIBUTES\n"));
		//
		// Buffer is too small, fail the request
		//
		if (OutputBufferLength < sizeof(KEYBOARD_ATTRIBUTES)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);

		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestRetrieveOutputMemory failed %x\n", status));
			break;
		}

		WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);
		noItems = (USHORT)WdfCollectionGetCount(FilterDeviceCollection);
		if (noItems == 0) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint(("Not any filter device found.\n"));
			WdfWaitLockRelease(FilterDeviceCollectionLock);
			break;
		}
		hFilterDevice = WdfCollectionGetItem(FilterDeviceCollection, controlExt->ActiveMouseId);
		WdfWaitLockRelease(FilterDeviceCollectionLock);

		filterExt = FilterGetData(hFilterDevice);
		status = WdfMemoryCopyFromBuffer(outputMemory,
			0,
			&filterExt->KeyboardAttributes,
			sizeof(KEYBOARD_ATTRIBUTES));

		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfMemoryCopyFromBuffer failed %x\n", status));
			break;
		}

		bytesTransferred = sizeof(KEYBOARD_ATTRIBUTES);
#pragma endregion
		break;

	case IOCTL_KEYBOARD_DETECT_DEVICE_ID:
#pragma region IOCTL_KEYBOARD_DETECT_DEVICE_ID
		DebugPrint(("Received IOCTL_KEYBOARD_DETECT_DEVICE_ID\n"));
		if (OutputBufferLength < sizeof(USHORT)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}

		WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);
		noItems = (USHORT)WdfCollectionGetCount(FilterDeviceCollection);
		if (noItems == 0) {
			status = STATUS_INVALID_DEVICE_REQUEST;
			DebugPrint(("Not any filter device found.\n"));
			WdfWaitLockRelease(FilterDeviceCollectionLock);
			break;
		}
		WdfWaitLockRelease(FilterDeviceCollectionLock);

		if (controlExt->InputDeviceWorkItem != NULL) {
			status = STATUS_OPERATION_IN_PROGRESS;
			DebugPrint(("Another request is in progress\n"));
			break;
		}
		controlExt->InputDeviceWorkItem = IoAllocateWorkItem(WdfDeviceWdmGetDeviceObject(ControlDevice));
		if (controlExt->InputDeviceWorkItem == NULL)
		{
			status = STATUS_INSUFFICIENT_RESOURCES;
			DebugPrint(("Failed to allocate work item:  %x\n", status));
			break;
		}
		status = WdfRequestForwardToIoQueue(Request, controlExt->ManualQueue);
		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestForwardToIoQueue failed %x\n", status));
			break;
		}

		WdfSpinLockAcquire(controlExt->SpinLock);
		controlExt->InputRequired = TRUE;//signalling the KbFilter_ServiceCallback to queue the workitem on key entered
		WdfSpinLockRelease(controlExt->SpinLock);
		return;//important to return from function here
#pragma endregion

	case IOCTL_KEYBOARD_GET_DEVICE_ID:
#pragma region IOCTL_KEYBOARD_GET_DEVICE_ID
		DebugPrint(("Received IOCTL_KEYBOARD_GET_DEVICE_ID\n"));
		if (OutputBufferLength < sizeof(KEYBOARD_QUERY_RESULT)) {
			status = STATUS_BUFFER_TOO_SMALL;
			break;
		}
		status = WdfRequestRetrieveOutputMemory(Request, &outputMemory);
		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfRequestRetrieveOutputMemory failed %x\n", status));
			break;
		}
		WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);
		noItems = (USHORT)WdfCollectionGetCount(FilterDeviceCollection);
		keboardIds.ActiveDeviceId = controlExt->ActiveMouseId;
		WdfWaitLockRelease(FilterDeviceCollectionLock);
		keboardIds.NumberOfDevices = noItems;
		status = WdfMemoryCopyFromBuffer(outputMemory,
			0,
			&keboardIds,
			sizeof(KEYBOARD_QUERY_RESULT));
		if (!NT_SUCCESS(status)) {
			DebugPrint(("WdfMemoryCopyFromBuffer failed %x\n", status));
			break;
		}
		bytesTransferred = sizeof(KEYBOARD_QUERY_RESULT);
#pragma endregion
		break;

	default:
		status = STATUS_NOT_IMPLEMENTED;
		break;
	}
	//this will free the user bufferd input
	WdfRequestCompleteWithInformation(Request, status, bytesTransferred);

	return;
}

VOID
On_IOCTL_KEYBOARD_INSERT_KEY(
	IN PKEYBOARD_INPUT_DATA InputDataStart,
	IN size_t InputCount,
	IN PFILTER_DEVICE_EXTENSION FilterExtension) {
	/*++

Routine Description:

	Forwards our artificial inputs to the kbdclass service callback

Arguments:

	InputDataStart - Keyboard input structure pointer.

	InputCount - Number of inputs the pointer points to.

	FilterExtension - Filter device extension which holds the hooked service call back of the kbdclass.

Return Value:

		Void.

--*/
	DebugPrint(("Entered On_IOCTL_KEYBOARD_INSERT_KEY\n"));
	KIRQL oldIrql = KeGetCurrentIrql();
	if (oldIrql < DISPATCH_LEVEL)
		KeRaiseIrql(DISPATCH_LEVEL, &oldIrql);
	NT_ASSERT(KeGetCurrentIrql() == DISPATCH_LEVEL);
	ULONG InputDataConsumed = 0;
	PKEYBOARD_INPUT_DATA end = InputDataStart + InputCount;
	__try {
		//calling the kbdclass service callback
		(*(PSERVICE_CALLBACK_ROUTINE)(ULONG_PTR)FilterExtension->UpperConnectData.ClassService)(
			FilterExtension->UpperConnectData.ClassDeviceObject,
			InputDataStart,
			end,
			&InputDataConsumed);
	}
	__finally {
		if (oldIrql < DISPATCH_LEVEL)
			KeLowerIrql(oldIrql);
	}
}


VOID
KbFilter_ServiceCallback(
	IN PDEVICE_OBJECT  DeviceObject,
	IN PKEYBOARD_INPUT_DATA InputDataStart,
	IN PKEYBOARD_INPUT_DATA InputDataEnd,
	IN OUT PULONG InputDataConsumed
)
/*++

Routine Description:

	Called when there are keyboard packets to report to the Win32 subsystem.
	You can do anything you like to the packets.  For instance:

	o Drop a packet altogether
	o Mutate the contents of a packet
	o Insert packets into the stream

Arguments:

	DeviceObject - Context passed during the connect IOCTL

	InputDataStart - First packet to be reported

	InputDataEnd - One past the last packet to be reported.  Total number of
				   packets is equal to InputDataEnd - InputDataStart

	InputDataConsumed - Set to the total number of packets consumed by the RIT
						(via the function pointer we replaced in the connect
						IOCTL)

Return Value:

	void.

--*/
{
	PFILTER_DEVICE_EXTENSION	filterExt;
	PCONTROL_DEVICE_EXTENSION	controlExt;
	WDFDEVICE					filterDevice;
	BOOLEAN						setInputRequested = FALSE;

	DebugPrint(("Entered KbFilter_ServiceCallback\n"));
	controlExt = ControlGetData(ControlDevice);
	filterDevice = WdfWdmDeviceGetWdfDeviceHandle(DeviceObject);
	filterExt = FilterGetData(filterDevice);
	WdfSpinLockAcquire(controlExt->SpinLock);
	setInputRequested = controlExt->InputRequired;
	controlExt->InputRequired = FALSE;//important to set it here else another key input will cause bsod since controlExt->InputDeviceWorkItem might be cleared or in use.
	WdfSpinLockRelease(controlExt->SpinLock);
	if (setInputRequested)
		IoQueueWorkItem(controlExt->InputDeviceWorkItem, SetCurrentInputDevice, DelayedWorkQueue, filterDevice);

	if (InputDataStart) {

		DebugPrint(("Kbd input - Flags: %x, Scan code: %x, Count: %i\n", InputDataStart->Flags, InputDataStart->MakeCode, InputDataEnd - InputDataStart));

#pragma region Filtering keys
		WdfSpinLockAcquire(filterExt->SpinLock);

		switch (filterExt->FilterRequest.FilterMode) {
		case FILTER_KEY_ALL:
			WdfSpinLockRelease(filterExt->SpinLock);
			(*InputDataConsumed) += (ULONG)(InputDataEnd - InputDataStart);//Every filtered key needs to be consumed.
			DebugPrint(("Key filtered flag: %x ,Scan code: %x\n", InputDataStart->Flags, InputDataStart->MakeCode));
			return; //drop the input
		case FILTER_KEY_FLAGS:
			for (LONG64 i = 0; i < InputDataEnd - InputDataStart; i++)
			{
				USHORT checkFlag = InputDataStart[i].Flags == 0 ? 1 : (InputDataStart[i].Flags << 1);
				//In this filter mode, filterExt->FilterRequest.FilterCount is where our flag predicate stored.
				if ((checkFlag & filterExt->FilterRequest.FilterCount) != 0)
				{
					//filter this key

					(*InputDataConsumed) += 1; //Every filtered key needs to be consumed.
					DebugPrint(("Key filtered flag: %x ,Scan code: %x\n", InputDataStart[i].Flags, InputDataStart[i].MakeCode));
					if (i == 0 && InputDataEnd - InputDataStart == 1)
					{
						WdfSpinLockRelease(filterExt->SpinLock);
						return;	//this is the only input, drop it. (most probabely there are always just one input at a time from a keyboard device)
					}
					LONG64 j = i;
					//In the case there are more than one input, replace this one with the next and so on.
					while (j + 1 < InputDataEnd - InputDataStart) {
						InputDataStart[j] = InputDataStart[j + 1];
						j++;
					}
					InputDataEnd--;
				}
			}
			break;
		case FILTER_KEY_FLAG_AND_SCANCODE:
			for (LONG64 i = 0; i < InputDataEnd - InputDataStart; i++)
			{
				BOOLEAN shouldFilter = FALSE;
				USHORT checkFlag = InputDataStart[i].Flags == 0 ? 1 : (InputDataStart[i].Flags << 1);
				for (USHORT j = 0; j < filterExt->FilterRequest.FilterCount; j++)
				{
					if (InputDataStart[i].MakeCode == filterExt->FilterRequest.FilterData[j].ScanCode && (checkFlag & filterExt->FilterRequest.FilterData[j].FlagPredicates) != 0) {
						shouldFilter = TRUE;
						break;
					}
				}
				if (shouldFilter)
				{
					//filter this key

					(*InputDataConsumed) += 1; //Every filtered key needs to be consumed.
					DebugPrint(("Key filtered flag: %x ,Scan code: %x\n", InputDataStart[i].Flags, InputDataStart[i].MakeCode));
					if (i == 0 && (InputDataEnd - InputDataStart) == 1)
					{
						WdfSpinLockRelease(filterExt->SpinLock);
						return;	//this is the only input, drop it. (most probabely there are always just one input at a time from a keyboard device)
					}
					LONG64 j = i;
					//In the case there are more than one input, replace this one with the next and so on.
					while (j + 1 < InputDataEnd - InputDataStart) {
						InputDataStart[j] = InputDataStart[j + 1];
						j++;
					}
					InputDataEnd--;
				}
			}
			break;
		}
#pragma endregion

#pragma region Modifing Keys
		if (filterExt->ModifyRequest.ModifyCount > 0)
			for (LONG64 i = 0; i < InputDataEnd - InputDataStart; i++)
			{
				USHORT checkFlag = InputDataStart[i].Flags == 0 ? 1 : (InputDataStart[i].Flags << 1);
				for (USHORT j = 0; j < filterExt->ModifyRequest.ModifyCount; j++)
				{
					if (InputDataStart[i].MakeCode == filterExt->ModifyRequest.ModifyData[j].FromScanCode && (checkFlag & filterExt->ModifyRequest.ModifyData[j].FlagPredicates) != 0) {
						InputDataStart[i].MakeCode = filterExt->ModifyRequest.ModifyData[j].ToScanCode;
						DebugPrint(("Key modified from: %x to: %x\n", filterExt->ModifyRequest.ModifyData[j].FromScanCode, filterExt->ModifyRequest.ModifyData[j].ToScanCode));
						break;
					}
				}
			}
		WdfSpinLockRelease(filterExt->SpinLock);
#pragma endregion

		//forwarding input to the kbdclass service callback.
		(*(PSERVICE_CALLBACK_ROUTINE)(ULONG_PTR)filterExt->UpperConnectData.ClassService)(
			filterExt->UpperConnectData.ClassDeviceObject,
			InputDataStart,
			InputDataEnd,
			InputDataConsumed);
	}
}

_Function_class_(IO_WORKITEM_ROUTINE)
VOID
SetCurrentInputDevice(
	_In_ PDEVICE_OBJECT DeviceObject,
	_In_opt_ PVOID Context) {

	/*++

Routine Description:

	Work item WorkerRoutine queued from KbFilter_ServiceCallback in response to the user
	request to detect the input device.

Arguments:

	DeviceObject - Device object of the framework filter device.

	Context - Context passed which is the filter device corresponding to the
	keyboard device which generated the input.

Return Value:

		Void.

--*/

	UNREFERENCED_PARAMETER(DeviceObject);

	PAGED_CODE();

	DebugPrint(("Entered SetCurrentInputDevice\n"));
	WDFDEVICE					filterDevice;
	ULONG                       noItems;
	PCONTROL_DEVICE_EXTENSION   controlExt;
	WDFREQUEST                  request;
	WDFMEMORY					outputMemory;
	NTSTATUS                    status;
	USHORT						deviceId;

	controlExt = ControlGetData(ControlDevice);
	filterDevice = (WDFDEVICE)Context;
	WdfWaitLockAcquire(FilterDeviceCollectionLock, NULL);
	noItems = WdfCollectionGetCount(FilterDeviceCollection);
	deviceId = controlExt->ActiveMouseId;
	for (USHORT i = 0; i < noItems; i++)
	{
		if (filterDevice == WdfCollectionGetItem(FilterDeviceCollection, i)) {
			deviceId = i;
			break;
		}
	}
	controlExt->ActiveMouseId = deviceId;
	WdfWaitLockRelease(FilterDeviceCollectionLock);


	IoFreeWorkItem(controlExt->InputDeviceWorkItem);
	controlExt->InputDeviceWorkItem = NULL;
	status = WdfIoQueueRetrieveNextRequest(controlExt->ManualQueue, &request);//fetching the pending request from our manual queue.
	if (!NT_SUCCESS(status)) {
		NT_ASSERT(status != STATUS_NO_MORE_ENTRIES);//should not get this
		DebugPrint(("WdfIoQueueRetrieveNextRequest failed %x\n", status));
		return;
	}
	status = WdfRequestRetrieveOutputMemory(request, &outputMemory);
	if (!NT_SUCCESS(status)) {
		DebugPrint(("WdfRequestRetrieveOutputMemory failed %x\n", status));
		WdfRequestComplete(request, status);
		return;
	}
	status = WdfMemoryCopyFromBuffer(
		outputMemory,
		0,
		&deviceId,
		sizeof(deviceId));
	if (!NT_SUCCESS(status)) {
		DebugPrint(("WdfMemoryCopyFromBuffer failed %x\n", status));
		WdfRequestComplete(request, status);
		return;
	}
	WdfRequestCompleteWithInformation(request, status, sizeof(deviceId));
}

VOID
KbFilter_RequestCompletionRoutine(
	WDFREQUEST                  Request,
	WDFIOTARGET                 Target,
	PWDF_REQUEST_COMPLETION_PARAMS CompletionParams,
	WDFCONTEXT                  Context
)
/*++

Routine Description:

	Completion Routine

Arguments:

	Target - Target handle
	Request - Request handle
	Params - request completion params
	Context - Driver supplied context


Return Value:

	VOID

--*/
{
	WDFMEMORY   buffer = CompletionParams->Parameters.Ioctl.Output.Buffer;
	NTSTATUS    status = CompletionParams->IoStatus.Status;

	UNREFERENCED_PARAMETER(Target);

	//
	// Save the keyboard attributes in our context area so that we can return
	// them to the app later.
	//
	if (NT_SUCCESS(status) &&
		CompletionParams->Type == WdfRequestTypeDeviceControlInternal &&
		CompletionParams->Parameters.Ioctl.IoControlCode == IOCTL_KEYBOARD_QUERY_ATTRIBUTES) {

		if (CompletionParams->Parameters.Ioctl.Output.Length >= sizeof(KEYBOARD_ATTRIBUTES)) {

			status = WdfMemoryCopyToBuffer(buffer,
				CompletionParams->Parameters.Ioctl.Output.Offset,
				&((PFILTER_DEVICE_EXTENSION)Context)->KeyboardAttributes,
				sizeof(KEYBOARD_ATTRIBUTES)
			);
		}
	}

	WdfRequestComplete(Request, status);
	return;
}


